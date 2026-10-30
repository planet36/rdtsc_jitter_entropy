# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Purpose

This repo demonstrates the feasibility of using RDTSC jitter as a source of
entropy compared to RDSEED.  It targets x86-64 Linux.  There is no test suite.
Claims are backed by PractRand statistical runs and Google Benchmark timings,
whose output is committed under `results/`.

## Commands

The scripts are not executable, so run them with `sh` from the repo root.

```sh
make                    # build one binary per top-level *.cpp (dump, benchmark)
make clean
make lint               # clang-tidy; make ignores its exit status
sh run-dump.sh          # full PractRand sweep; writes new files to results/
sh run-benchmark.sh     # full benchmark run; writes a new file to results/
```

These commands run a single case and write nothing to `results/`:

```sh
./dump -f rdtsc -k 3 | RNG_test stdin64 -tf 0 -te 1 -tlmin 64KB -tlmax 1MB -multithreaded
./benchmark --benchmark_filter='^rdtsc_jitter_entropy\(k=9\)'
```

The build needs GCC with C++26 support (`std::saturating_cast`), fmt, Google
Benchmark, and a CPU with RDSEED, because `-march=native` is what defines
`__RDSEED__` for `include/rdrand.h`.  `RNG_test` comes from PractRand and must be
on `PATH`.

## Architecture

**Entropy functions.**  `include/rdtsc_jitter_entropy.hpp` holds the accumulator,
which is the rotate-and-XOR construction from the "No Time to Hash" paper cited
in its header.  The caller passes `k`, the estimated minimum entropy bits per
sample.  The rotation `r` is the largest odd number not above `k`, which keeps it
co-prime to 64, so an even `k` behaves exactly like `k - 1`.  The function takes
`ceil(64 / r)` samples and yields the thread between them.

`rdtsc_jitter_entropy` and `rdtscp_jitter_entropy` are copies that differ only in
which read they call, so a change to one belongs in both.  Each is pinned to
`[[gnu::optimize("O1")]]` despite the global `-O3`, and committed results were
measured that way.  Their assertions compile only under `DEBUG`, which the
Makefile does not define.

The reads in `include/rdtsc.hpp` carry their own `MFENCE` and `LFENCE`
instructions, and the Intel SDM excerpts at the top of that file explain why.
Those fences affect the timing being measured, so changing them invalidates
comparisons with committed results.

**Adding an entropy source.**  A new source touches three places, and all of them
share the signature `uint64_t(unsigned int k)`:

- the `-f` option in `dump.cpp`, where `rdseed64_wrapper` shows how to adapt a
  function that ignores `k`
- the sweep in `run-dump.sh`
- the registrations in `benchmark.cpp`

**Experiment pipelines.**  `run-dump.sh` pipes `dump` into `RNG_test` for RDSEED as
a baseline, then for `rdtsc` and `rdtscp` at each odd `k` from 1 through 31.  It
writes `results/RNG_test.<func>[.k-<K>].<UTC>.txt` and lists the files that lack
the word `FAIL`, which is the pass criterion.  In the committed run, only RDSEED
and `k` of 1 and 3 pass, at the 1 MB test limit.

`run-benchmark.sh` runs `benchmark` on N-1 threads through the `NUM_THREADS`
environment variable, with 5 repetitions.  It writes `results/benchmark.<UTC>.txt`,
and `process-benchmark-result.sh` then prints the medians sorted by time.
`benchmark.cpp` registers only odd `k` from 1 through 9, a narrower range than the
PractRand sweep.

**Behavior of `dump`.**  It exits 0 without writing anything when stdout is a
terminal, so a bare `./dump` looks like a no-op.  Without `-l` it runs until the
reader closes the pipe.  It exits with an error unless `-k` is an integer from 1 through 63.

## Conventions

@COMMENT-STYLE.md governs code comments, Doxygen blocks, commit messages, and
documentation files.  It overrides any other style, prose, vocabulary, or spelling
guidance.

Every source file and script starts with an SPDX header (`MPL-2.0`).

`include/rdrand.h` and `include/rdtsc.hpp` also live in the owner's dotfiles repo,
as the "Sync with dotfiles repo" commits show.  An edit to either one may need to
be made there as well.

`include/parse_int.hpp` and `include/get_num_threads.hpp` are shared with other
projects, so keep anything specific to this repo out of them.
