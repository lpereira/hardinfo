#!/bin/bash
echo ""
echo "This script updates sysbench 1.0.20 with riscv beta"
echo ""
ARCH=$( uname -m )
echo $ARCH;
if [ "$ARCH" != 'riscv64' ]; then
    echo "Install sysbench 1.0.20 from distro"
    exit
fi
echo "needs additional packages to hardinfo2: libtool automake"
echo ""
read -p "Press Ctrl+C to cancel build, press Enter to Continue"

#Latest release
git clone https://github.com/akopytov/sysbench --branch 1.0

#update luajit for riscv
cd sysbench/third_party/luajit
rm -rf luajit
git clone https://github.com/plctlab/LuaJIT/ luajit

#update ck for riscv
cd ../concurrency_kit
rm -rf ck
git clone https://github.com/concurrencykit/ck ck
cd ../../

#build
./autogen.sh
./configure --without-mysql
make -j

#only local install sysbench
cp src/sysbench /usr/local/bin/

#cleanup
cd ..
rm -rf sysbench

echo "Done - Sysbench is installed in /usr/local/bin"
