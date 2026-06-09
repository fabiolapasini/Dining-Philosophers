#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>

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
		: index(i), left(l), right(r), 
		rng(std::random_device{}()), dist(100, 200)  
	{}

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

	// reference: cannot copy a mutex!
	Fork& left;		
	Fork& right;

	// release mutex if excetion occurs in eat()
	std::unique_lock<std::mutex> lk_left;
	std::unique_lock<std::mutex> lk_right;

	// avoid rece condition and local for each philosopher
	std::mt19937 rng;
	std::uniform_int_distribution<int> dist;

	void think(){
		print("is thinking");
		int duration = dist(rng);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration));

	}

	void hungry(){
		print("is hungry");
	}

	void pick_up_forks(){
		// avoid deadlock!
		if (index % 2 == 0) {
			lk_left = std::unique_lock<std::mutex>(left.mtx);
			print("took left fork");
			lk_right = std::unique_lock<std::mutex>(right.mtx);
			print("took right fork");
		}
		else{
			lk_right = std::unique_lock<std::mutex>(right.mtx);
			print("took right fork");
			lk_left = std::unique_lock<std::mutex>(left.mtx);
			print("took left fork");
		}
	}

	void eat(){
		print("is eating");
		int duration = dist(rng);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration));
	}

	void put_down_forks(){
		lk_left.unlock();
		lk_right.unlock();
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
			Philosopher( i, forks[i], forks[(i + 1)%N])
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
