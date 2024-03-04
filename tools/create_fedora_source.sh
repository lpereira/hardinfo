#!/bin/bash
VERSION=2.0.12
cd ..
rm -rf build
mkdir build
cd build
cmake ..
make package_source
cp ./_CPack_Packages/Linux-Source/RPM/SPECS/hardinfo.spec .
mv hardinfo-$VERSION.tar.gz hardinfo-$VERSION.orig.tar.gz
mv hardinfo-$VERSION-1.src.rpm hardinfo-$VERSION.src.rpm
mv hardinfo.spec hardinfo-$VERSION.spec

echo "Fedora Source Package Files ready in build:"
ls -l hardinfo-$VERSION.*.tar.gz
ls -l hardinfo-$VERSION.src.rpm
ls -l hardinfo-$VERSION.spec
