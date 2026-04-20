// https://doc.rust-lang.org/stable/book/title-page.html

use parking_lot::{Mutex, Condvar};
use std::sync::Arc;
use std::thread;
use rand::Rng;
use std::time::Duration;

const N: usize = 5;     // size_t 

#[derive(Clone, Copy, PartialEq)]
enum State {
    Thinking,
    Hungry,
    Eating,
}

// Shared state among th
struct Table {
    state: Mutex<[State; N]>,
    both_forks_available: [Semaphore; N],
    output: Mutex<()>,  
}

/* 
# withoug attribute "copy" this wouldn't be possible, a would be passed and no more accessible
let a = State::Thinking;
let b = a; 

if state[i] == State::Hungry { ... }  // without PartialEq it does not compile
it makes it possible to compare (== or !=)
*/


// create a Semaphore as a mutex and condition variable
// mutex in a container
struct Semaphore {
    ready: Mutex<bool>,
    condition_variable: Condvar,
}

impl Semaphore {
    fn new() -> Self {
        Semaphore {
            ready: Mutex::new(false),   // init to 0 like Semaphore(0)
            condition_variable: Condvar::new(),
        }
    }

    // wait until ready (like acquire())
    fn wait(&self) {
        let mut ready = self.ready.lock();
        while !*ready {
            self.condition_variable.wait(&mut ready); // release il lock and sleep
        }
        *ready = false; // use singal
    }

    // wake up who is sleeping (like release())
    fn signal(&self) {
        let mut ready = self.ready.lock();
        *ready = true;
        self.condition_variable.notify_one();
    }
}


fn left(i: usize) -> usize {
    (i + N - 1) % N
}

fn right(i: usize) -> usize {
    (i + 1) % N
}

/*
take_forks(i)
    → state[i] = Hungry
    → test(i): are neighbors eating?
        → YES: does nothing, wait() blocks the thread
        → NO: state[i] = Eating, signal() → wait() passes immediately

put_forks(neighbor)
    → state[neighbor] = Thinking
    → test(i): can i eat now?
        → YES: state[i] = Eating, signal()
            → thread i wakes up
            → exits wait()
            → returns to take_forks()
            → returns to philosopher()
            → executes eat()
*/

// test() receives &Table instead of accessing globals.
// Note: it receives the state already locked, because it is always called inside a critical section.
fn can_i_eat(i: usize, state: &mut [State; N], both_forks_available: &[Semaphore; N]) {
    if state[i] == State::Hungry && state[left(i)] != State::Eating && state[right(i)] != State::Eating
    {
        state[i] = State::Eating;
        both_forks_available[i].signal();  // wake up philosopher i
    }
}

fn think(i: usize, table: &Table) {
    let duration = rand::thread_rng().gen_range(400..800);
    {
        let _lock = table.output.lock();
        println!("{i} is thinking");
    }
    thread::sleep(Duration::from_millis(duration));
}

fn take_forks(i: usize, table: &Table) {
    {
        let mut state = table.state.lock();
        state[i] = State::Hungry;
        {

            let _lock = table.output.lock();
            println!("{i:>20} is HUNGRY");
        } // When the guard goes out of scope, the lock is released - Python's "with" statement.
        
        // state still locked and mutable
        can_i_eat(i, &mut state, &table.both_forks_available);
    } // state MutexGuard is dropped here, lock released
    
    // blocks if can_i_eat() did not call signal()
    table.both_forks_available[i].wait();   // outside the lock!               
}

fn eat(i: usize, table: &Table) {
    let duration = rand::thread_rng().gen_range(400..800);
    {
        let _lock = table.output.lock();
        println!("{i:>40} is eating");
    }
    thread::sleep(Duration::from_millis(duration));
}

fn put_forks(i: usize, table: &Table) {
    let mut state = table.state.lock();  // acquire lock
    state[i] = State::Thinking;          // done eating, go back to thinking
    can_i_eat(left(i), &mut state, &table.both_forks_available);   // can left neighbor eat now?
    can_i_eat(right(i), &mut state, &table.both_forks_available);  // can right neighbor eat now?
}                                        // lock released here — MutexGuard dropped

fn philosopher(i: usize, table: Arc<Table>) {
    // while true{}
    loop {
        think(i, &table);
        take_forks(i, &table);
        eat(i, &table);
        put_forks(i, &table);
    }
}

fn main() {
    println!("Dining Philosophers Solution (Rust)");

    // create Table on the heap, Arc starts the reference counter at 1
    // Arc: Atomically Reference Counted, std::shared_ptr<T> — Arc<T> but arc is trhead safe for def
    let table = Arc::new(Table {
        state: Mutex::new([State::Thinking; N]),                                // array of N State::Thinking - [State.THINKING] * 5
        both_forks_available: std::array::from_fn(|_| Semaphore::new()),        // array of N semaphores initialized to 0 - [Semaphore() for _ in range(N)] - |_| : closure, stands for indexes
        output: Mutex::new(()),                                                 // mutex protecting println! - (): None
    });


    /*
    let handles: Vec<_> = (0..N).map(|i| {
            // .clone() does not copy Table, it only increments the reference counter
            // each thread gets its own Arc pointing to the same Table in memory
            let table = Arc::clone(&table);

            // move: the closure takes ownership of `table` and `i`
            // without move, the compiler refuses to compile because it cannot
            // guarantee that the variables are still alive when the thread uses them
            thread::spawn(move || philosopher(i, table))
        })
        .collect(); // consumes the iterator and builds the Vec of JoinHandles
    */


    let mut handles = Vec::new();
    for i in 0..N {
        let table = Arc::clone(&table);
        let t = thread::spawn(move || philosopher(i, table));
        handles.push(t);
    }


    for handle in handles {
        // wait for each thread to finish
        // unwrap() propagates the error if the thread panicked
        handle.join().unwrap();
    }
}
