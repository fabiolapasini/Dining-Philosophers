// https://doc.rust-lang.org/stable/book/title-page.html

use parking_lot::{Mutex, Condvar};
use std::sync::Arc;
use std::thread;
use rand::Rng;
use std::time::Duration;

const N: usize = 5;

#[derive(Clone, Copy, PartialEq)]
enum State {
    Thinking,
    Hungry,
    Eating,
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
        *ready = false; // consuma il segnale
    }

    // wake up who is sleeping (like release())
    fn signal(&self) {
        let mut ready = self.ready.lock();
        *ready = true;
        self.condition_variable.notify_one();
    }
}





fn main() {
    println!("Hello, world!");
}