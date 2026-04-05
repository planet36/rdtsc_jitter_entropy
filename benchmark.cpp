// SPDX-FileCopyrightText: Steven Ward
// SPDX-License-Identifier: MPL-2.0

#include <benchmark/benchmark.h> // https://github.com/google/benchmark

using func_u64_u_t = uint64_t (&)(const unsigned int);

void BM_rdtsc_jitter_entropy(benchmark::State& BM_state,
        func_u64_u_t& fn,
        const unsigned int k)
{
    // Perform setup here

    for (auto _ : BM_state)
    {
        // This code gets timed

        benchmark::DoNotOptimize(fn(k));
    }
}

template <std::unsigned_integral T>
using func_T_void_t = T (&)();

template <std::unsigned_integral T>
void BM_rd_rand_seed(benchmark::State& BM_state,
        func_T_void_t<T>& fn)
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
#include <fmt/format.h>
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

    constexpr int min_threads = 1;
    const auto max_threads = std::max(min_threads, static_cast<int>(std::thread::hardware_concurrency()));
    // https://en.wikipedia.org/wiki/Elvis_operator
    //const auto max_threads = static_cast<int>(std::thread::hardware_concurrency()) ?: min_threads;

    auto num_threads = min_threads;

    try
    {
        num_threads = std::stoi(std::getenv("NUM_THREADS"));
    }
    catch (...)
    {
        num_threads = min_threads;
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
        benchmark::RegisterBenchmark("rdseed64", BM_rd_rand_seed<uint64_t>, rdseed64);

        for (unsigned int k = 1; k <= 9; k += 2)
        {
            benchmark::RegisterBenchmark(
                    fmt::format("rdtsc_jitter_entropy(k={})", k),
                    BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, k);
            benchmark::RegisterBenchmark(
                    fmt::format("rdtscp_jitter_entropy(k={})", k),
                    BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, k);
        }
    }
    else
    {
        benchmark::RegisterBenchmark("rdseed64", BM_rd_rand_seed<uint64_t>, rdseed64)->Threads(num_threads);

        for (unsigned int k = 1; k <= 9; k += 2)
        {
            benchmark::RegisterBenchmark(
                    fmt::format("rdtsc_jitter_entropy(k={})", k),
                    BM_rdtsc_jitter_entropy, rdtsc_jitter_entropy, k)->Threads(num_threads);
            benchmark::RegisterBenchmark(
                    fmt::format("rdtscp_jitter_entropy(k={})", k),
                    BM_rdtsc_jitter_entropy, rdtscp_jitter_entropy, k)->Threads(num_threads);
        }
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    // }}}

    return 0;
}
