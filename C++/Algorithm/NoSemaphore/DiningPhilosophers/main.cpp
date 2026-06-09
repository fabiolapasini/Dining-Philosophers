#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>

static const int N = 5;
static const int NUM_OF_MEAL = 4;


struct Fork
{
	std::mutex mtx;
	int id;
};


Fork forks[5];
std::mutex print_mtx;		// protects output from race condition


class Philosopher
{
public:
	Philosopher(int i, Fork& l, Fork& r)
		: index(i), left(l), right(r) {}

	void run(){
		for (int i = 0; i < NUM_OF_MEAL; i++){
			think();
			hungry();
			pick_up_forks();
			eat();
			put_down_forks();
		}
	}

private:
	int index;
	Fork& left;		// reference: cannot copy a mutex!
	Fork& right;

	void think(){
		print("is thinking");
		int time = 100 + (std::rand() % 100);
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

	}

	void hungry(){
		print("is hungry");
	}

	void pick_up_forks(){
		// avoid deadlock!
		if (index % 2 == 0) {
			left.mtx.lock();
			print("took left fork");
			right.mtx.lock();
			print("took right fork");
		}
		else{
			right.mtx.lock();
			print("took right fork");
			left.mtx.lock();
			print("took left fork");
		}
	}

	void eat(){
		print("is eating");
		int time = 100 + (std::rand() % 100);
		std::this_thread::sleep_for(std::chrono::milliseconds(time));
	}

	void put_down_forks(){
		left.mtx.unlock();
		right.mtx.unlock();
		print("put down forks");
	}

	void print(const std::string &msg){
		std::lock_guard<std::mutex> lock(print_mtx);
		std::cout << "Philosopher " << index
			<< " [" << std::this_thread::get_id() << "] "
			<< msg << "\n";
	}
};


int main()
{
	std::srand((unsigned)std::time(nullptr));

	std::vector <Philosopher> phils;

	/*
		Philosopher holds references to Forks, since std::mutex is not copyable.
		If too many elements are added and the vector runs out of capacity,
		it reallocates to a new contiguous memory block, moving all elements —
		leaving any existing references dangling (pointing to freed memory).
		Accessing a dangling reference is undefined behaviour: the program may
		crash, silently corrupt data, or appear to work correctly by accident.
	*/
	phils.reserve(N);

	for (int i = 0; i < N; i++){
		phils.emplace_back(
			Philosopher( i, forks[i], forks[(i + 1)%N] )
		);
	}

	std::vector <std::thread> ths;
	for (int i = 0; i < N; i++) {
		ths.emplace_back(
			[& phils, i]{
				phils[i].run();
			}
		);
	}

	for (auto& t : ths)
		t.join();

	return 0;
}
