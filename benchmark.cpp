// SPDX-FileCopyrightText: Steven Ward
// SPDX-License-Identifier: OSL-3.0

// https://github.com/google/benchmark

#include <benchmark/benchmark.h>
#include <functional>

void BM_rdtsc_jitter_entropy(benchmark::State& BM_state,
        const std::function<uint64_t(const unsigned int, const bool)>& fn,
        const unsigned int k, const bool use_pause)
{
    // Perform setup here

    for (auto _ : BM_state)
    {
        // This code gets timed

        benchmark::DoNotOptimize(fn(k, use_pause));
    }
}

template <std::unsigned_integral T>
void BM_rd_rand_seed(benchmark::State& BM_state, const std::function<T()>& fn)
{
    // Perform setup here

    for (auto _ : BM_state)
    {
        // This code gets timed

        benchmark::DoNotOptimize(fn());
    }
}

#include "rdrand.h"
#include "rdtsc_jitter_entropy.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>
#include <thread>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    using namespace std::literals;

    // Copied from benchmark.h
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    // {{{ determine num_threads

    constexpr unsigned int min_threads = 1;
    const unsigned int max_threads = std::max(min_threads, std::thread::hardware_concurrency());
    // https://en.wikipedia.org/wiki/Elvis_operator
    //const unsigned int max_threads = std::thread::hardware_concurrency() ?: min_threads;

    unsigned int num_threads;

    try
    {
        num_threads = static_cast<unsigned int>(std::stoi(std::getenv("NUM_THREADS")));
    }
    catch (...)
    {
        num_threads = max_threads;
    }

    num_threads = std::clamp(num_threads, min_threads, max_threads);

    /*
    if (num_threads > min_threads)
        // Don't use all the cores
        --num_threads;
    */

    // }}}

    // {{{ accuracy testing

    // }}}

    // {{{ speed

    if (num_threads == 1)
    {
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=1,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 1, false);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=3,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 3, false);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=5,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 5, false);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=7,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 7, false);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=9,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 9, false);

        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=1,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 1, false);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=3,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 3, false);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=5,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 5, false);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=7,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 7, false);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=9,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 9, false);

        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=1,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 1, true);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=3,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 3, true);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=5,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 5, true);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=7,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 7, true);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=9,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 9, true);

        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=1,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 1, true);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=3,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 3, true);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=5,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 5, true);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=7,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 7, true);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=9,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 9, true);

        benchmark::RegisterBenchmark("rdrand64", BM_rd_rand_seed<uint64_t>, rdrand64);
        benchmark::RegisterBenchmark("rdseed64", BM_rd_rand_seed<uint64_t>, rdseed64);
    }
    else
    {
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=1,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 1, false)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=3,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 3, false)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=5,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 5, false)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=7,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 7, false)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=9,use_pause=false)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 9, false)         ->Threads(num_threads);

        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=1,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 1, false)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=3,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 3, false)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=5,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 5, false)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=7,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 7, false)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=9,use_pause=false)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 9, false)        ->Threads(num_threads);

        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=1,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 1, true)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=3,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 3, true)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=5,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 5, true)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=7,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 7, true)         ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtsc_jitter_entropy(k=9,use_pause=true)",         BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, 9, true)         ->Threads(num_threads);

        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=1,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 1, true)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=3,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 3, true)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=5,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 5, true)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=7,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 7, true)        ->Threads(num_threads);
        benchmark::RegisterBenchmark("rdtscp_jitter_entropy(k=9,use_pause=true)",        BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, 9, true)        ->Threads(num_threads);

        benchmark::RegisterBenchmark("rdrand64", BM_rd_rand_seed<uint64_t>, rdrand64)->Threads(num_threads);
        benchmark::RegisterBenchmark("rdseed64", BM_rd_rand_seed<uint64_t>, rdseed64)->Threads(num_threads);
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    // }}}

    return 0;
}
