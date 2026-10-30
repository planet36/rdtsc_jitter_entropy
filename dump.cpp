// SPDX-FileCopyrightText: Steven Ward
// SPDX-License-Identifier: MPL-2.0

/**
* This program will dump bytes to stdout (as long as stdout does not refer to a
* terminal), which could be analyzed by PractRand.
*
* "How to Test with PractRand"
* https://www.pcg-random.org/posts/how-to-test-with-practrand.html
*/

#include "parse_int.hpp"
#include "rdrand.h"
#include "rdtsc_jitter_entropy.hpp"

#include <array>
#include <cerrno> // program_invocation_short_name
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <err.h>
#include <exception>
#include <functional>
#include <numeric>
#include <print>
#include <string_view>
#include <unistd.h>

/// The raison d'etre of this wrapper is to have the same signature as
/// rdtsc_jitter_entropy.
static inline uint64_t
rdseed64_wrapper([[maybe_unused]] const unsigned int k)
{
    return rdseed64();
}

#define nl (void)putchar('\n')

inline constexpr std::string_view program_author = "Steven Ward";
inline constexpr std::string_view program_version = "2026-02-17";
inline constexpr std::string_view program_license = "MPL-2.0";

// Globals

inline constexpr unsigned long long bytes_per_gigabyte = 1000ULL * 1000ULL * 1000ULL;
inline constexpr unsigned long long bytes_per_gibibyte = 1024ULL * 1024ULL * 1024ULL;

inline constexpr unsigned long long max_limit_gibibytes = 1024ULL * 1024ULL; // 1 pebibyte (PiB)

inline constexpr unsigned int default_k = 3;
inline constexpr unsigned int min_k = 1;
inline constexpr unsigned int max_k = 63;
static_assert(min_k > 0);
static_assert(max_k < 64);
static_assert(min_k <= max_k);


/// Write all \a count bytes of \a buf to \a fd
/**
* Retry after a partial write or \c EINTR.  Exit on any other error.
*/
void
write_all(const int fd, const void* buf, size_t count)
{
    const auto* p = static_cast<const char*>(buf);

    while (count > 0)
    {
        const ssize_t num_bytes_written = ::write(fd, p, count);

        if (num_bytes_written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            err(EXIT_FAILURE, "write");
        }

        p += num_bytes_written;
        count -= static_cast<size_t>(num_bytes_written);
    }
}

/// Print the version information.
void
print_version()
{
    std::println("{} {}", program_invocation_short_name, program_version);
    std::println("License: {}", program_license);
    std::println("Written by {}", program_author);
}

/// Print the help message.
void
print_usage()
{
    std::println("Usage: {} [OPTION]...", program_invocation_short_name);
    std::println("Dump RDTSC jitter entropy to stdout.");
    nl;

    std::println("OPTIONS");
    nl;

    std::println("-V");
    std::println("    Print the version information, then exit.");
    nl;

    std::println("-h");
    std::println("    Print this message, then exit.");
    nl;

    std::println("-l  MAX");
    std::println("    Limit the output to no more than MAX gibibytes.");
    std::println("    If MAX is 0, the output is unlimited.");
    std::println("    (maximum: {})", max_limit_gibibytes);
    nl;

    std::println("-f  FUNC");
    std::println("    Specify which entropy function should be used.");
    std::println("    FUNC is one of: rdtsc, rdtscp, rdseed");
    std::println("    (default: {:?})", "rdtsc");
    nl;

    std::println("-k  K");
    std::println("    Specify the estimated minimum entropy bits per sample.");
    std::println("    K is an integer from {} to {}.", min_k, max_k);
    std::println("    (default: {})", default_k);
    nl;
}

// NOLINTNEXTLINE(bugprone-exception-escape)
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    using namespace std::literals;

    // {{{ options
    unsigned long long limit_bytes = 0;
    std::function<uint64_t(const unsigned int)> func{rdtsc_jitter_entropy};
    unsigned int k = default_k;
    // }}}

    // {{{ process options
    {
        const char* short_options = "+Vhl:f:k:";
        int c = 0;
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
                    // the value of "-l" is gibibytes (GiB)
                    const unsigned long long limit_gibibytes =
                        parse_option_int(optarg, 0, max_limit_gibibytes, "-l");

                    // convert GiB to B
                    limit_bytes = limit_gibibytes * bytes_per_gibibyte;
                }
                catch (const std::exception& ex)
                {
                    errx(EXIT_FAILURE, "%s", ex.what());
                }
                break;

            case 'f':
                if (optarg == "rdseed"sv)
                {
                    func = rdseed64_wrapper;
                }
                else if (optarg == "rdtsc"sv)
                {
                    func = rdtsc_jitter_entropy;
                }
                else if (optarg == "rdtscp"sv)
                {
                    func = rdtscp_jitter_entropy;
                }
                else
                {
                    errx(EXIT_FAILURE, "Invalid option value: \"%s\"", optarg);
                }
                break;

            case 'k':
                try
                {
                    const int tmp = parse_option_int(optarg, min_k, max_k, "-k");
                    k = std::saturating_cast<decltype(k)>(tmp);
                }
                catch (const std::exception& ex)
                {
                    errx(EXIT_FAILURE, "%s", ex.what());
                }
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
        static_assert(bytes_per_gibibyte % buf_size_bytes == 0);

        std::array<result_type, buf_num_elems> buf{};

        const size_t num_writes = limit_bytes / buf_size_bytes;

        for (size_t j = 0; (limit_bytes == 0) || (j < num_writes); ++j)
        {
            for (size_t i = 0; i < buf_num_elems; ++i)
            {
                buf[i] = func(k);
            }

            write_all(STDOUT_FILENO, std::data(buf), sizeof(buf));
        }
    }
    // }}}

    return 0;
}
