#!/bin/bash
VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
ARCH=$(uname -m)
DIST=$(uname -r|cut -d '.' -f 4)

#clean and prep
yum -y install ninja-build
cd ..
rm -rf build
sudo yum -y remove hardinfo2
rm -rf ~/rpmbuild

#build source
mkdir build
cd build
cmake -DDISTRO=src ..
#fix for local build
cat ../tools/hardinfo2.spec |grep -v Patch|sed '/URL:/c\URL:            ./'|sed '/Source0:/c\Source0:        hardinfo2-%{version}.tar.gz' |sed 's/hardinfo2-release/hardinfo2/g' >./hardinfo2.spec
make package_source

cp -f ../tools/hardinfo2.spec .

echo "Fedora Source Package Files ready in build:"
ls -l hardinfo2-$VERSION*.src.rpm
ls -l hardinfo2.spec

sleep 3

#install src package
sudo rpm --nomd5 -i ./hardinfo2-$VERSION-1.$DIST.src.rpm

#create package from srpm
cd ~/rpmbuild/SPECS
rpmbuild -ba hardinfo2.spec

echo "Fedora binary build from Source Package Files ready:"
ls -l ~/rpmbuild/RPMS/$ARCH/*
sudo yum -y install ~/rpmbuild/RPMS/$ARCH/hardinfo2-$VERSION*
yum info hardinfo2
