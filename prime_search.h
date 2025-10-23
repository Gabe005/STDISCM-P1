#pragma once

#include <vector>
#include <thread>
#include <string>
#include <mutex>


struct Config {
    int threads;
    int max_n;
    int variant;
};

class PrimeSearcher {
public:
    PrimeSearcher(const Config& cfg);
    ~PrimeSearcher();

    // Run chosen variant. This blocks until completion.
    void run();

private:
    Config cfg_;

    std::mutex coutMutex;

    // utilities
    static bool isPrimeSimple(int n); // simple single-thread trial division
    static std::string timestampNow();

    // variants:
    void variant1_range_immediate();
    void variant2_range_buffered();
    void variant3_cooperative_immediate();
    void variant4_cooperative_divisor_testing();

    // helpers used by variant implementations
    void worker_range_immediate(int idx, int start, int end);
    void worker_range_buffered(int idx, int start, int end, std::vector<int>& out);
    void worker_round_robin_immediate(int idx);
    bool is_Prime_multi_threaded(int n, int numThreads);

    // For variant 4 cooperative:
    void variant4_worker_loop(int workerId);
    int current_candidate_; // protected by mutex in variant 4
    bool stop_workers_;
};

