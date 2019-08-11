#!/bin/sh

# bash just_bench.sh >whatever.conf
# crudini --merge data/benchmark.conf < whatever.conf

do_hi_bench() {
    sleep 1
    echo "[$1]"
    hardinfo -b "$1" -g conf
}

do_hi_bench "CPU Blowfish (Single-thread)"
do_hi_bench "CPU Blowfish (Multi-thread)"
do_hi_bench "CPU Blowfish (Multi-core)"
do_hi_bench "CPU CryptoHash"
do_hi_bench "CPU Fibonacci"
do_hi_bench "CPU N-Queens"
do_hi_bench "CPU Zlib"
do_hi_bench "FPU FFT"
do_hi_bench "FPU Raytracing"
#do_hi_bench "GPU Drawing"
