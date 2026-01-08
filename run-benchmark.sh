#!/usr/bin/sh
# SPDX-FileCopyrightText: Steven Ward
# SPDX-License-Identifier: OSL-3.0

export LC_ALL=C

# Use N-1 threads
NUM_THREADS="$(nproc --ignore=1)"
export NUM_THREADS

# Should be an odd number for simpler median
BENCHMARK_REPS=7

OUTPUT_DIR=results
DATETIME=$(date -u +'%Y%m%dT%H%M%S')

mkdir --verbose --parents -- "$OUTPUT_DIR" || exit

./benchmark \
    --benchmark_enable_random_interleaving=true \
    --benchmark_repetitions=${BENCHMARK_REPS} \
    --benchmark_report_aggregates_only=true \
    --benchmark_out_format=console \
    --benchmark_out="${OUTPUT_DIR}/benchmark.${DATETIME}.txt" || exit

sh process-benchmark-result.sh "${OUTPUT_DIR}/benchmark.${DATETIME}.txt"
