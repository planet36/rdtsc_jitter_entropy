// SPDX-FileCopyrightText: Steven Ward
// SPDX-License-Identifier: OSL-3.0

/**
* This program will dump bytes to stdout (as long as stdout does not refer to a
* terminal), which could be analyzed by PractRand.
*
* "How to Test with PractRand"
* https://www.pcg-random.org/posts/how-to-test-with-practrand.html
*/

#include "rdrand.h"
#include "rdtsc_jitter_entropy.hpp"

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <err.h>
#include <fmt/format.h>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>

/// The raison d'etre of this wrapper is to have the same signature as
/// rdtsc_jitter_entropy.
static inline uint64_t
rdseed64_wrapper([[maybe_unused]] const unsigned int k,
        [[maybe_unused]] const bool use_pause = false)
{
    return rdseed64();
}

#define nl (void)putchar('\n')

inline constexpr std::string_view program_author = "Steven Ward";
inline constexpr std::string_view program_version = "2026-01-07";
inline constexpr std::string_view program_license = "OSL-3.0";

// Globals

inline constexpr unsigned long long bytes_per_gigabyte = 1000ULL * 1000ULL * 1000ULL;
inline constexpr unsigned long long bytes_per_gibibyte = 1024ULL * 1024ULL * 1024ULL;

inline constexpr unsigned int default_k = 3;
inline constexpr unsigned int min_k = 1;
inline constexpr unsigned int max_k = 63;
static_assert(min_k > 0);
static_assert(max_k < 64);
static_assert(min_k <= max_k);


/// Print the version information.
void
print_version()
{
    fmt::println("{} {}", program_invocation_short_name, program_version);
    fmt::println("License: {}", program_license);
    fmt::println("Written by {}", program_author);
}

/// Print the help message.
void
print_usage()
{
    fmt::println("Usage: {} [OPTION]...", program_invocation_short_name);
    fmt::println("Dump RDTSC jitter entropy to stdout.");
    nl;

    fmt::println("OPTIONS");
    nl;

    fmt::println("-V");
    fmt::println("    Print the version information, then exit.");
    nl;

    fmt::println("-h");
    fmt::println("    Print this message, then exit.");
    nl;

    fmt::println("-l  MAX");
    fmt::println("    Limit the output to no more than MAX gibibytes.");
    nl;

    fmt::println("-f  FUNC");
    fmt::println("    Specify which entropy function should be used.");
    fmt::println("    FUNC is one of: rdtsc, rdtscp, rdseed");
    fmt::println("    (default: {:?})", "rdtsc");
    nl;

    fmt::println("-k  K");
    fmt::println("    Specify the estimated minimum entropy bits per sample.");
    fmt::println("    (default: {})", default_k);
    nl;

    fmt::println("-p");
    fmt::println("    Call _mm_pause() between RDTSC calls.");
    nl;
}

int
main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    using namespace std::literals;

    // {{{ options
    unsigned long long limit_bytes = 0;
    std::function<uint64_t(const unsigned int, const bool)> func{rdtsc_jitter_entropy};
    unsigned int k = default_k;
    bool use_pause = false;
    // }}}

    // {{{ process options
    {
        const char* short_options = "+Vhl:f:k:p";
        int c;
        while ((c = getopt(argc, argv, short_options)) != -1)
        {
            switch (c)
            {
            case 'h':
                print_usage();
                std::exit(EXIT_SUCCESS);
                break;

            case 'V':
                print_version();
                std::exit(EXIT_SUCCESS);
                break;

            case 'l':
                try
                {
                    const auto tmp = std::stoull(optarg);
                    limit_bytes = std::saturate_cast<decltype(limit_bytes)>(tmp);
                }
                catch (const std::invalid_argument& ex)
                {
                    errx(EXIT_FAILURE, "invalid argument: %s: \"%s\"", ex.what(), optarg);
                }
                catch (const std::out_of_range& ex)
                {
                    errx(EXIT_FAILURE, "out of range: %s: \"%s\"", ex.what(), optarg);
                }

                //if (__builtin_umulll_overflow(limit_bytes, bytes_per_gibibyte, &limit_bytes))
                if (limit_bytes > std::numeric_limits<decltype(limit_bytes)>::max() / bytes_per_gibibyte)
                {
                    errx(EXIT_FAILURE, "Arithmetic overflow: %s * %llu", optarg, bytes_per_gibibyte);
                }
                limit_bytes *= bytes_per_gibibyte;
                break;

            case 'f':
                if (optarg == "rdtsc"s)
                {
                    func = rdtsc_jitter_entropy;
                }
                else if (optarg == "rdtscp"s)
                {
                    func = rdtscp_jitter_entropy;
                }
                else if (optarg == "rdseed"s)
                {
                    func = rdseed64_wrapper;
                }
                else
                {
                    errx(EXIT_FAILURE, "Invalid option value: \"%s\"", optarg);
                }
                break;

            case 'k':
                try
                {
                    auto tmp = std::stol(optarg);
                    if (tmp < min_k)
                        tmp = min_k;
                    else if (tmp > max_k)
                        tmp = max_k;
                    k = std::saturate_cast<decltype(k)>(tmp);
                }
                catch (const std::invalid_argument& ex)
                {
                    errx(EXIT_FAILURE, "invalid argument: %s: \"%s\"", ex.what(), optarg);
                }
                catch (const std::out_of_range& ex)
                {
                    errx(EXIT_FAILURE, "out of range: %s: \"%s\"", ex.what(), optarg);
                }
                break;

            case 'p':
                use_pause = true;
                break;

            default:
                std::exit(EXIT_FAILURE);
            }
        }
    }
    // }}}

    if (::isatty(STDOUT_FILENO)) // stdout is open and refers to a terminal.
    {
        // Do not write to a terminal.
        return 0;
    }

    // {{{ dump
    {
        using result_type = uint64_t;

        // /proc/sys/fs/pipe-max-size = 1048576
        // fcntl(STDOUT_FILENO, F_GETPIPE_SZ) = 65536
        // BUFSIZ = 8192
        // PractRand uses a buffer of size 32768 bytes for reading from stdin.
        constexpr size_t buf_size_bytes = 32768;
        constexpr size_t buf_num_elems = buf_size_bytes / sizeof(result_type);
        static_assert(buf_size_bytes % sizeof(result_type) == 0);

        result_type buf[buf_num_elems] = {0};

        if (limit_bytes == 0)
        {
            while (true)
            {
                for (size_t i = 0; i < buf_num_elems; ++i)
                {
                    buf[i] = func(k, use_pause);
                }

                (void)::write(STDOUT_FILENO, &buf[0], sizeof(buf));
            }
        }
        else // limit_bytes > 0
        {
            const size_t num_writes = limit_bytes / buf_size_bytes;
            assert(limit_bytes % buf_size_bytes == 0);

            for (size_t j = 0; j < num_writes; ++j)
            {
                for (size_t i = 0; i < buf_num_elems; ++i)
                {
                    buf[i] = func(k, use_pause);
                }

                (void)::write(STDOUT_FILENO, &buf[0], sizeof(buf));
            }
        }
    }
    // }}}

    return 0;
}
