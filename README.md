# rdtsc\_jitter\_entropy

[RDTSC](https://en.wikipedia.org/wiki/Time_Stamp_Counter) Jitter Entropy

The purpose of this repo is to demonstrate the feasibility of using RDTSC jitter as a source of entropy compared to RDSEED.

The goal is to get entropy from RDTSC calls, even if it is biased, rather than cryptographic random numbers.  Therefore the PractRand runs in `results/` stop at 1 MB by design.  PractRand looks for statistical flaws and does not measure how many bits of entropy each sample holds.
