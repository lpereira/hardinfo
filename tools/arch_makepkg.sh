#!/bin/sh
#"CPACK" - called in build dir
echo "Building Arch Package"
cd ..
cp -f ./tools/PKGBUILD .
cp -f ./tools/hardinfo2.install .

#update package version
VER=$(cat CMakeLists.txt |grep HARDINFO2_VERSION\ |grep -o -P '(?<=").*(?=")')
VER="${VER}_ArchLinux"
sed -i "/pkgver=/c\pkgver=$VER" PKGBUILD

#build and install
if [ $(id -u) -ne 0 ]; then
    makepkg -cs --noextract
else
    chown -R nobody ../hardinfo2
    sudo -u nobody makepkg -cs --noextract
    chown -R root ../hardinfo2
fi

#move result to build dir
mv *.zst ./build/

#cleanup
rm -f PKGBUILD
rm -f hardinfo2.install

cd build
