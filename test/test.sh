
DATADER=`pwd`

do_test() {
    ARCH="$1"
    CPUINFO="$DATADER/$2"
    cmake .. -DOVRARCH=$ARCH -DOVRCPUINFO=\"$CPUINFO\"
    make
}

#do_test x86 data/x86_pent_cpuinfo
#do_test x86 data/x86_amdr7_cpuinfo
#do_test ppc data/ppc_g5_cpuinfo
#do_test ppc data/power8_cpuinfo
#do_test arm data/arm_rpi3_aarch64_cpuinfo
#do_test arm data/arm_jetsontx2_cpuinfo
#do_test sparc data/sparc_m7_cpuinfo
#do_test mips data/mips_loongson_cpuinfo
#do_test alpha data/alpha_as_cpuinfo
#do_test parisc data/parisc_hppa_fortex4_cpuinfo
#do_test ia64 data/ia64_x2_cpuinfo
do_test m68k data/m68k_sun3_cpuinfo



