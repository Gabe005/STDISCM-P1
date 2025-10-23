#include "prime_search.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>


PrimeSearcher::PrimeSearcher(const Config& cfg) : cfg_(cfg), current_candidate_(2), stop_workers_(false) {}
PrimeSearcher::~PrimeSearcher() {}

std::string PrimeSearcher::timestampNow() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

bool PrimeSearcher::isPrimeSimple(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0) return n == 2;
    int r = static_cast<int>(std::sqrt(n));
    for (int d = 3; d <= r; d += 2) {
        if (n % d == 0) return false;
    }
    return true;
}

void PrimeSearcher::run() {
    std::cout << "Run start: " << timestampNow() << "  (threads=" << cfg_.threads << " max=" << cfg_.max_n << " variant=" << cfg_.variant << ")\n";
    switch (cfg_.variant) {
        case 1: variant1_range_immediate(); break;
        case 2: variant2_range_buffered();  break;
        case 3: variant3_cooperative_immediate(); break;
        case 4: variant4_cooperative_divisor_testing(); break;
        default:
            std::cerr << "Unknown variant: " << cfg_.variant << "\n";
    }
    std::cout << "Run end:   " << timestampNow() << "\n";
}

// Variant 1
// Range division; immediate print
void PrimeSearcher::variant1_range_immediate() {
    auto start = std::chrono::system_clock::now();
    int T = cfg_.threads;
    int N = cfg_.max_n;
    if (T <= 0) T = 1;
    if (N < 2) return;

    std::vector<std::thread> threads;
    int total = N - 1; // numbers from 2..N inclusive
    int base = total / T;
    int rem = total % T;

    int cur = 2;
    for (int i = 0; i < T; ++i) {
        int block = base + (i < rem ? 1 : 0);
        int start = cur;
        int end = (block > 0) ? (cur + block - 1) : (cur - 1);
        cur = end + 1;
        threads.emplace_back(&PrimeSearcher::worker_range_immediate, this, i, start, end);
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << timestampNow() << " [Variant 1] Done in " << elapsed.count() << "s" << std::endl;
    }

    for (auto& th : threads) th.join();
}

void PrimeSearcher::worker_range_immediate(int idx, int start, int end) {
    for (int n = start; n <= end; ++n) {
        if (isPrimeSimple(n)) {
            std::ostringstream oss;
            oss << "[" << timestampNow() << "] (thread " << (idx + 1)
                << ") prime: " << n;
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << oss.str() << std::endl;
            }
        }
    }
}

// Variant 2 
// Range division; buffer per-thread results, print after join
void PrimeSearcher::variant2_range_buffered() {
    int T = cfg_.threads;
    int N = cfg_.max_n;
    if (T <= 0) T = 1;
    if (N < 2) return;

    auto start  = std::chrono::system_clock::now();

    std::vector<std::thread> threads;
    std::vector<std::vector<int>> results(T);

    int total = N - 1;
    int base = total / T;
    int rem = total % T;
    int cur = 2;
    for (int i = 0; i < T; ++i) {
        int block = base + (i < rem ? 1 : 0);
        int start = cur;
        int end = (block > 0) ? (cur + block - 1) : (cur - 1);
        cur = end + 1;
        threads.emplace_back(&PrimeSearcher::worker_range_buffered, this, i, start, end, std::ref(results[i]));
    }
    for (auto& th : threads) th.join();

    // After all threads done: print everything
    std::cout << "=== Buffered primes (all threads finished) ===\n";
    std::string timeNow = timestampNow();
    for (int i = 0; i < T; ++i) {
        for (int p : results[i]) {
            std::ostringstream oss;
            oss << "[" << timeNow << "] (thread " << i << ") prime: " << p;
            std::cout << oss.str() << std::endl;
        }
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << timestampNow() << " [Variant 2] Done in " << elapsed.count() << "s" << std::endl;
    }
    std::cout << "============================================\n";
}

void PrimeSearcher::worker_range_buffered(int idx, int start, int end, std::vector<int>& out) {
    if (start > end) return;
    for (int n = start; n <= end; ++n) {
        if (isPrimeSimple(n)) out.push_back(n);
    }
}

// Variant 3
// cooperative assignment; immediate print

void PrimeSearcher::variant3_cooperative_immediate() {
    auto start = std::chrono::system_clock::now();
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << timestampNow() << " [Variant 3] Start searching up to " << cfg_.max_n << std::endl;
    }

    for (int n = 2; n <= cfg_.max_n; ++n) {
        if (is_Prime_multi_threaded(n, cfg_.threads)) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << timestampNow() << " prime: " << n << std::endl;
        }
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << timestampNow() << " [Variant 3] Done in " << elapsed.count() << "s" << std::endl;
    }
}


// Variant 4
// Cooperative divisibility testing using cfg_.threads worker threads.
// For each candidate n in [2..max], workers split divisor range [2..sqrt(n)] among themselves.
// manager (main run thread) coordinates worker rounds.

void PrimeSearcher::variant4_cooperative_divisor_testing() {
    auto start = std::chrono::system_clock::now();
    std::vector<int> primes;
    primes.reserve(cfg_.max_n / 10);

    for (int n = 2; n <= cfg_.max_n; ++n) {
        if (is_Prime_multi_threaded(n, cfg_.threads)) {
            primes.push_back(n);
        }
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << timestampNow() << " [Variant 4] Printing buffered primes..." << std::endl;
        for (int p : primes) {
            std::cout << "prime: " << p << std::endl;
        }
        std::cout << timestampNow() << " [Variant 4] Done in " << elapsed.count() << "s" << std::endl;
    }
}


bool PrimeSearcher::is_Prime_multi_threaded(int n, int numThreads) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    int sqrtN = static_cast<int>(std::sqrt(n));
    int range = (sqrtN - 3) / numThreads + 1;
    std::atomic<bool> isPrime(true);
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        int start = 3 + t * range;
        int end = std::min(sqrtN, start + range - 1);

        threads.emplace_back([&, start, end]() {
            for (int i = start; i <= end && isPrime; i += 2) {
                if (n % i == 0) {
                    isPrime = false;
                    break;
                }
            }
        });
    }

    for (auto &th : threads)
        th.join();

    return isPrime;
}