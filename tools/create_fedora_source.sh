#!/bin/bash
VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
cd ..
rm -rf build
yum -y remove hardinfo
rm -rf ~/rpmbuild

mkdir build
cd build
cmake -DDISTRO=src ..
make package_source
cp _CPackage_Packages/Linux-Source/RPM/SPECS/hardinfo.spec .

echo "Fedora Source Package Files ready in build:"
ls -l hardinfo-$VERSION*.src.rpm
ls -l hardinfo.spec

sleep 3

#checking
fedpkg --release f39 lint

#install src package
rpm --nomd5 -i ./hardinfo-$VERSION-1.src.rpm

#create package from srpm
cd ~/rpmbuild/SPECS
rpmbuild -ba hardinfo.spec

echo "Fedora binary build from Source Package Files ready:"
ls -l ~/rpmbuild/RPMS/*
yum -y install ~/rpmbuild/RPMS/hardinfo-$VERSION*
