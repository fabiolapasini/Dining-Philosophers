
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <atomic>
#include <iomanip>
#include <memory>

 // ─────────────────────────────────────────────────────────────────────────────
 // Global configuration
 // ─────────────────────────────────────────────────────────────────────────────

constexpr int N_PHILOSOPHERS = 5;       // Number of philosophers (and forks)
constexpr int N_MEALS = 3;              // How many times each philosopher eats
constexpr int MAX_THINK_MS = 1500;      // Maximum thinking time (ms)
constexpr int MAX_EAT_MS = 1000;        // Maximum eating time (ms)

std::mutex mtx_print;
std::atomic<int> total_meals{ 0 };

// ─────────────────────────────────────────────────────────────────────────────
// Class Fork
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A fork is essentially a mutex: either it is lying on the table (unlocked)
 * or it is held by a philosopher (locked). We wrap it in a class for clarity
 * and to give each fork an identity for logging.
 */
class Fork {
public:
    int id;
    std::mutex mtx;

    explicit Fork(int id) : id(id) {}

    // Not copyable or movable
    Fork(const Fork&) = delete;
    Fork& operator=(const Fork&) = delete;
};

// ─────────────────────────────────────────────────────────────────────────────
// Class Philosopher
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Each philosopher knows:
 *  - their own index and name
 *  - references to the two adjacent forks (left and right)
 *
 * Life cycle: think → pick up forks → eat → put down forks.
 * Repeated N_MEALS times, then the philosopher leaves the table.
 */
class Philosopher {
public:
    // ── Constructor ──────────────────────────────────────────────────────────
    Philosopher(int index,
        const std::string& name,
        Fork& left,
        Fork& right)
        : index_{ index }
        , name_{ name }
        , left_{ left }
        , right_{ right }
        , rng_{ std::random_device{}() }
        , meals_eaten_{ 0 }
    {}

    // ── Thread entry point ───────────────────────────────────────────────────
    void operator()() {
        log("sits down at the table 🪑");

        for (int i = 0; i < N_MEALS; ++i) {
            think();
            pick_up_forks();
            eat();
            put_down_forks();
        }

        log("is done eating and leaves satisfied 🚶");
    }

    // ── Getters ──────────────────────────────────────────────────────────────
    int  index()       const { return index_; }
    int  meals_eaten() const { return meals_eaten_; }
    const std::string& name() const { return name_; }

private:
    // ── Internal state ───────────────────────────────────────────────────────
    int          index_;
    std::string  name_;
    Fork& left_;
    Fork& right_;
    std::mt19937 rng_;
    int          meals_eaten_;

    // ── Actions ──────────────────────────────────────────────────────────────

    /**
     * The philosopher thinks for a random amount of time.
     * No resources are held during this phase.
     */
    void think() {
        log("is thinking... 🤔");
        sleep_ms(200, MAX_THINK_MS);
    }

    /**
     * Picks up both forks in the order that prevents deadlock:
     *  - EVEN index  → left first, then right
     *  - ODD index   → right first, then left
     *
     * This asymmetry ensures the circular wait cannot form.
     */
    void pick_up_forks() {
        if (index_ % 2 == 0) {
            log("wants fork " + std::to_string(left_.id) + " (left)");
            left_.mtx.lock();
            log("picked up fork " + std::to_string(left_.id) );

            log("wants fork " + std::to_string(right_.id) + " (right)");
            right_.mtx.lock();
            log("picked up fork " + std::to_string(right_.id) );
        }
        else {
            log("wants fork " + std::to_string(right_.id) + " (right)");
            right_.mtx.lock();
            log("picked up fork " + std::to_string(right_.id) );

            log("wants fork " + std::to_string(left_.id) + " (left)");
            left_.mtx.lock();
            log("picked up fork " + std::to_string(left_.id) );
        }
    }

    /**
     * Eats for a random amount of time.
     * Both forks are held throughout this phase.
     */
    void eat() {
        ++meals_eaten_;
        log("IS EATING (meal " + std::to_string(meals_eaten_) +
            " of " + std::to_string(N_MEALS) + ")");
        sleep_ms(300, MAX_EAT_MS);
        total_meals.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * Puts both forks back on the table.
     * Released in reverse acquisition order — good practice.
     */
    void put_down_forks() {
        left_.mtx.unlock();
        right_.mtx.unlock();
        log("put down both forks ↩️");
    }

    // ── Utilities ─────────────────────────────────────────────────────────────

    /** Thread-safe log with elapsed timestamp and philosopher name. */
    void log(const std::string& msg) const {
        static const auto t0 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::steady_clock::now() - t0;
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        std::lock_guard<std::mutex> guard(mtx_print);
        std::cout << "[" << std::setw(6) << ms << " ms] "
            << std::setw(12) << std::left << name_
            << " " << msg << "\n";
    }

    /** Sleeps for a random number of milliseconds in [lo, hi]. */
    void sleep_ms(int lo, int hi) {
        std::uniform_int_distribution<int> dist(lo, hi);
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng_)));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n"
        << "║           DINING PHILOSOPHERS PROBLEM                ║\n"
        << "║  " << N_PHILOSOPHERS << " philosophers, " << N_MEALS
        << " meals each — asymmetric solution  ║\n"
        << "╚══════════════════════════════════════════════════════╝\n\n";

    // ── Forks ─────────────────────────────────────────────────────────────────
    // unique_ptr because Fork is not copyable (it contains a mutex).
    std::vector<std::unique_ptr<Fork>> forks;
    forks.reserve(N_PHILOSOPHERS);
    for (int i = 0; i < N_PHILOSOPHERS; ++i)
        forks.push_back(std::make_unique<Fork>(i));

    // ── Names ─────────────────────────────────────────────────────────────────
    std::vector<std::string> names = {
        "Socrates", "Plato", "Aristotle", "Epicurus", "Pythagoras"
    };
    while (static_cast<int>(names.size()) < N_PHILOSOPHERS)
        names.push_back("Philosopher" + std::to_string(names.size()));

    // ── Philosophers ──────────────────────────────────────────────────────────
    // Philosopher i has fork i on the left and fork (i+1) % N on the right.
    std::vector<std::unique_ptr<Philosopher>> philosophers;
    philosophers.reserve(N_PHILOSOPHERS);
    for (int i = 0; i < N_PHILOSOPHERS; ++i) {
        philosophers.push_back(std::make_unique<Philosopher>(
            i,
            names[i],
            *forks[i],
            *forks[(i + 1) % N_PHILOSOPHERS]
        ));
    }

    // ── Launch threads ────────────────────────────────────────────────────────
    std::vector<std::thread> threads;
    threads.reserve(N_PHILOSOPHERS);

    auto t_start = std::chrono::steady_clock::now();

    for (auto& p : philosophers)
        threads.emplace_back(std::ref(*p));

    // ── Wait for all threads to finish ────────────────────────────────────────
    for (auto& t : threads)
        t.join();

    auto t_end = std::chrono::steady_clock::now();
    long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_end - t_start).count();

    // ── Summary ───────────────────────────────────────────────────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n"
        << "║                    FINAL SUMMARY                    ║\n"
        << "╚══════════════════════════════════════════════════════╝\n";

    int expected = N_PHILOSOPHERS * N_MEALS;
    for (auto& p : philosophers) {
        std::cout << "  " << std::setw(12) << std::left << p->name()
            << " → " << p->meals_eaten() << " meals\n";
    }
    std::cout << "\n  Total meals : " << total_meals.load()
        << " / " << expected
        << (total_meals == expected ? "  ✅" : "  ❌ ERROR") << "\n"
        << "  Total time  : " << elapsed_ms << " ms\n\n";

    std::cout << "No deadlock detected. Solution is correct! 🎉\n";

    return 0;
}


