#!/bin/bash
#tool script used by project maintainer to test fedora/redhat releases

VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
ARCH=$(uname -m)

#clean and prep
yum -y install ninja-build
cd ..
rm -rf build
sudo yum -y remove hardinfo2
rm -rf ~/rpmbuild
mkdir build

#create source package
cd ..
tar -czf hardinfo2-release-$VERSION.tar.gz hardinfo2 --transform s/hardinfo2/hardinfo2-release-$VERSION/
mv hardinfo2-release-$VERSION.tar.gz hardinfo2/build/
cd hardinfo2/build
wget https://src.fedoraproject.org/rpms/hardinfo2/raw/rawhide/f/hardinfo2.spec
cp hardinfo2.spec ../tools/
cat hardinfo2.spec |grep -v Patch|sed '/URL:/c\URL:            ./'|sed '/Source0:/c\Source0:        hardinfo2-release-%{version}.tar.gz' |sed '/Version:/c\Version: '"$VERSION"'' >./hardinfo2.spec

echo "Fedora/redhat Source Package Files ready in build:"
ls -l hardinfo2-release-$VERSION*.tar.gz
ls -l hardinfo2.spec

sleep 3

#install src package
mkdir ~/rpmbuild
mkdir ~/rpmbuild/SPECS
mkdir ~/rpmbuild/SOURCES
cp hardinfo2.spec ~/rpmbuild/SPECS/
cp hardinfo2-release-$VERSION.tar.gz ~/rpmbuild/SOURCES/

#create package from srpm
cd ~/rpmbuild/SPECS
rpmbuild -ba hardinfo2.spec

echo "Fedora binary build from Source Package Files ready:"
ls -l ~/rpmbuild/RPMS/$ARCH/*
sudo yum -y install ~/rpmbuild/RPMS/$ARCH/hardinfo2-$VERSION*
yum info hardinfo2
