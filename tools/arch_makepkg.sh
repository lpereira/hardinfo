#!/bin/sh
#called in build dir
echo "Building Arch Package"
cd ../..
cp -f hardinfo2/tools/PKGBUILD .
cp -f hardinfo2/tools/hardinfo2.install .
tar -czf hardinfo2.tgz hardinfo2

#update package version
VER=$(cat hardinfo2/CMakeLists.txt |grep HARDINFO2_VERSION\ |grep -o -P '(?<=").*(?=")')
VER="${VER}_ArchLinux"
sed -i "/pkgver=/c\pkgver=$VER" PKGBUILD

#build and install
sudo -u nobody makepkg -cs

#move result to build dir
mv *.zst ~/hardinfo2/build/

#cleanup
rm -f PKGBUILD
rm -f hardinfo2.install
rm -f hardinfo2.tgz

cd hardinfo2/build
rm hardinfo2_*.json
