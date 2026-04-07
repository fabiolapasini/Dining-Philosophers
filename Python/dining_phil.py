import threading
import time
import random
from enum import Enum

N = 5

class State(Enum):
    THINKING = 0
    HUNGRY = 1
    EATING = 2

# global states
state = [State.THINKING] * N

# protects state
state_critical_region_mtx  = threading.Lock()

# protects print()
output_mtx = threading.Lock()

# Binary semaphores initialized to 0
# it stops on acquire, and wake up on release
both_forks_available = [threading.Semaphore(0) for _ in range(N)]


def left(i):
    return (i - 1 + N) % N

def right(i):
    return (i + 1) % N

# check if neighbors are eating and wake up
def test(i):
    # if neighbors are not eating, grant permission to eat
    if state[i] == State.HUNGRY and state[left(i)] != State.EATING and state[right(i)] != State.EATING:
        state[i] = State.EATING
        both_forks_available[i].release()

def think(i):
    duration = random.randint(400, 800) / 1000.0
    with output_mtx:
        print(f"{i} is thinking")
    time.sleep(duration)

# wait for the forks
def take_forks(i):
    with state_critical_region_mtx :
        state[i] = State.HUNGRY
        with output_mtx:
            print(f"{i:>20} is HUNGRY")
        test(i) 
    # Blocks if test(i) didn't release and didn't change state to EATING 
    both_forks_available[i].acquire()

def eat(i):
    duration = random.randint(400, 800) / 1000.0
    with output_mtx:
        print(f"{i:>40} is eating")
    time.sleep(duration)

# put down forks when u'r done
def put_forks(i):
    with state_critical_region_mtx :
        state[i] = State.THINKING
        # Check if neighbors can eat now that I'm done
        # I release neighbors if they are hungry
        test(left(i))
        test(right(i))


def philosopher(i):
    while True:
        think(i)
        take_forks(i)
        eat(i)
        put_forks(i)


if __name__ == "__main__":
    print("Dining Philosophers Solution")

    threads = []
    for i in range(N):
        t = threading.Thread(target=philosopher, args=(i,))
        t.daemon = True # Allows script to exit on Ctrl+C
        threads.append(t)
        t.start()
    
    # Keep main thread alive
    try:
        while True: time.sleep(1)
    except KeyboardInterrupt:
        print("\nSimulation stopped.")