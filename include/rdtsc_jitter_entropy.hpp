// SPDX-FileCopyrightText: Steven Ward
// SPDX-License-Identifier: OSL-3.0

/// Get entropy from TSC jitter
/**
* \file
* \author Steven Ward
*
* \verbatim
No Time to Hash:
On Super-Efficient Entropy Accumulation
\endverbatim
* \sa https://cs.nyu.edu/~dodis/ps/no-time.pdf
*
* \verbatim
The Windows 10 random number generation infrastructure
Niels Ferguson
Published: October 2019
For the latest information, please see
https://aka.ms/win10rng
\endverbatim
* \sa https://aka.ms/win10rng
*/

#pragma once

#include "rdtsc.hpp"

#if defined(DEBUG)
#include <cassert>
#include <numeric> // std::gcd
#endif
#include <bit> // std::rotl
#include <immintrin.h>
#include <limits>

#if !defined(__SSE2__)
#error "SSE2 instruction set required"
#endif

/// Get entropy from TSC jitter
/**
* \param k estimated minimum entropy bits per sample
* \pre \a k > 0
* \pre \a k < 64
*/
[[gnu::optimize("O1")]]
static uint64_t
rdtsc_jitter_entropy(const unsigned int k)
{
    constexpr unsigned int L = std::numeric_limits<uint64_t>::digits;
#if defined(DEBUG)
    assert(k > 0);
    assert(k < L);
#endif

    const unsigned int r = k - ((k % 2) == 0); // the largest odd number ≤ k
#if defined(DEBUG)
    assert(r > 0);
    assert(r <= k);
    assert((r % 2) != 0); // r must be odd
    assert(std::gcd(L, r) == 1); // r must be co-prime to L
#endif

    // number of samples to take
    // (ceiling of the quotient of the integer division)
    const unsigned int N = (L / r) + ((L % r) != 0);
#if defined(DEBUG)
    assert(N > 0);
#endif

    uint64_t entropy = rdtsc();

    // N-1 times
    for (unsigned int i = 1; i < N; ++i)
    {
        //_mm_clflush(&entropy);

        _mm_pause();

        const uint64_t tsc = rdtsc();
        entropy = std::rotl(entropy, static_cast<int>(r)) ^ tsc;
    }

    return entropy;
}

/// Get entropy from TSC jitter
/**
* \param k estimated minimum entropy bits per sample
* \pre \a k > 0
* \pre \a k < 64
*/
[[gnu::optimize("O1")]]
static uint64_t
rdtscp_jitter_entropy(const unsigned int k)
{
    constexpr unsigned int L = std::numeric_limits<uint64_t>::digits;
#if defined(DEBUG)
    assert(k > 0);
    assert(k < L);
#endif

    const unsigned int r = k - ((k % 2) == 0); // the largest odd number ≤ k
#if defined(DEBUG)
    assert(r > 0);
    assert(r <= k);
    assert((r % 2) != 0); // r must be odd
    assert(std::gcd(L, r) == 1); // r must be co-prime to L
#endif

    // number of samples to take
    // (ceiling of the quotient of the integer division)
    const unsigned int N = (L / r) + ((L % r) != 0);
#if defined(DEBUG)
    assert(N > 0);
#endif

    uint64_t entropy = rdtscp();

    // N-1 times
    for (unsigned int i = 1; i < N; ++i)
    {
        //_mm_clflush(&entropy);

        _mm_pause();

        const uint64_t tsc = rdtscp();
        entropy = std::rotl(entropy, static_cast<int>(r)) ^ tsc;
    }

    return entropy;
}
