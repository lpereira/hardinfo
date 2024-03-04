#!/bin/bash
VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
cd ..
rm -rf build
sudo yum -y remove hardinfo
rm -rf ~/rpmbuild

mkdir build
cd build
cmake -DDISTRO=src ..
cp ../tools/hardinfo.spec .
make package_source
#cp _CPack_Packages/Linux-Source/RPM/SPECS/hardinfo.spec .

echo "Fedora Source Package Files ready in build:"
ls -l hardinfo-$VERSION*.src.rpm
ls -l hardinfo.spec

#exit
sleep 3

#checking
#fedpkg --release f39 lint

#install src package
sudo rpm --nomd5 -i ./hardinfo-$VERSION-1.src.rpm
#cp -r ../tools/hardinfo.spec ~/rpmbuild/SPECS/

#create package from srpm
cd ~/rpmbuild/SPECS
rpmbuild -ba hardinfo.spec

echo "Fedora binary build from Source Package Files ready:"
ls -l ~/rpmbuild/RPMS/*
sudo yum -y install ~/rpmbuild/RPMS/hardinfo-$VERSION*
yum info hardinfo
