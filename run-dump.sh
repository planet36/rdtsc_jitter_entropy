#!/usr/bin/sh
# SPDX-FileCopyrightText: Steven Ward
# SPDX-License-Identifier: OSL-3.0

renice --priority 19 --pid $$ > /dev/null

export LC_ALL=C

TF=0
TE=1
TLMIN=64KB
TLMAX=1MB

OUTPUT_DIR=results
DATETIME=$(date -u +'%Y%m%dT%H%M%S')

mkdir --verbose --parents -- "$OUTPUT_DIR" || exit

kill_jobs()
{
    # https://www.gnu.org/software/bash/manual/html_node/Job-Control-Builtins.html
    # https://www.man7.org/linux/man-pages/man1/pkill.1.html

    pkill --parent $$
    sleep 0.5s
    pkill --parent $$ --signal SIGTERM
    sleep 0.5s
    pkill --parent $$ --signal SIGKILL

    exit
}

trap 'kill_jobs' INT

for K in $(seq 1 2 31)
do
    echo "K=$K"

    ./dump -f rdtsc  -k "$K"    | RNG_test stdin64 -tf $TF -te $TE -tlmin $TLMIN -tlmax $TLMAX -multithreaded > "${OUTPUT_DIR}/RNG_test.rdtsc.k-$K.pause-no.${DATETIME}.txt" &
    ./dump -f rdtscp -k "$K"    | RNG_test stdin64 -tf $TF -te $TE -tlmin $TLMIN -tlmax $TLMAX -multithreaded > "${OUTPUT_DIR}/RNG_test.rdtscp.k-$K.pause-no.${DATETIME}.txt" &
    ./dump -f rdtsc  -k "$K" -p | RNG_test stdin64 -tf $TF -te $TE -tlmin $TLMIN -tlmax $TLMAX -multithreaded > "${OUTPUT_DIR}/RNG_test.rdtsc.k-$K.pause-yes.${DATETIME}.txt" &
    ./dump -f rdtscp -k "$K" -p | RNG_test stdin64 -tf $TF -te $TE -tlmin $TLMIN -tlmax $TLMAX -multithreaded > "${OUTPUT_DIR}/RNG_test.rdtscp.k-$K.pause-yes.${DATETIME}.txt" &

    wait
done

./dump -f rdseed | RNG_test stdin64 -tf $TF -te $TE -tlmin $TLMIN -tlmax $TLMAX -multithreaded > "${OUTPUT_DIR}/RNG_test.rdseed.${DATETIME}.txt"

echo
echo "Result files which do NOT contain \"FAIL\":"

command grep -L -F -w FAIL -- "${OUTPUT_DIR}/RNG_test."*".${DATETIME}.txt"
