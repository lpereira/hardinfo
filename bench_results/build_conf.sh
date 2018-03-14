#!/bin/bash

CRUDINI=`which crudini`
if [ -z $CRUDINI ]; then
    echo "Needs crudini; can't continue."
    exit 1
fi

add_conf() {
    echo "Adding $1 to benchmark.conf ..."
    "$CRUDINI" --merge benchmark.conf < "$1"
}

echo "Clearing benchmark.conf ..."
echo "" >benchmark.conf

#These are new results verified by bp ~2017/2018
add_conf raspberry_pi_3b_r1p2.conf
add_conf raspberry_pi_1b_r1p0.conf
add_conf amd_ryzen5_1600.conf
add_conf amd_phenom2_x4.conf
add_conf toshiba_satellite_c675_psc3uu-00n00g.conf

# Some fun historical results for the flops.c benchmark from the flops.c readme
add_conf historical_flops_results.conf

# This is the old results file from the 2009-era hardinfo, they contain less information
# because they use the old format. They are unverified.
#add_conf old_benchmark.conf

# This is a strange CPU Zlib result that was in the old benchmark.conf
#add_conf old_ppc_zlib.conf

echo "Replacing benchmark.conf with the newly generated version"
mv benchmark.conf ..
