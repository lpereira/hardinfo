#!/bin/bash

#
# bash just_bench.sh >whatever.conf
#
# hardinfo PR instructions:
# crudini --merge ../data/benchmark.conf < whatever.conf
# git checkout -b br_whatever
# git add ../data/benchmark.conf
# git commit -m "bench_result: whatever"
# git push -u <github_fork_origin> br_whatever
# https://github.com/lpereira/hardinfo/pulls

USER_NOTE="$1"

do_hi_bench() {
    sleep 1
    echo "[$1]"
    if [ "$USER_NOTE" != "" ]; then
        hardinfo -b "$1" -g conf -u "$USER_NOTE"
    else
        hardinfo -b "$1" -g conf
    fi
}

if [ ! -z `which sysbench` ]; then
    do_hi_bench "SysBench CPU (Single-thread)"
    do_hi_bench "SysBench CPU (Multi-thread)"
    #do_hi_bench "SysBench CPU (Four threads)"
    do_hi_bench "SysBench Memory (Single-thread)"
    #do_hi_bench "SysBench Memory (Two threads)"
    do_hi_bench "SysBench Memory"
fi

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
