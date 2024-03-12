#!/bin/bash
#tool script used by project maintainer to test debian releases
# WIP - needs maintainer to create .dsc and debian directory - current takes from CPack

VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
ARCH=$(uname -m)
if [ $ARCH=="x86_64" ]; then ARCH=amd64; fi

cd ..
rm -rf build
sudo apt -y remove hardinfo
sudo apt -y remove hardinfo2

mkdir build
cd build
cmake ..
make package_source
#rename cpack files
mv hardinfo2_$VERSION*.deb hardinfo2-$VERSION.src.deb
rm hardinfo2_$VERSION*.tar.gz

#extract CPack source package
mkdir cpacksrc
dpkg-deb -R hardinfo2-$VERSION.src.deb cpacksrc

#create source package (NOTE: We use github tags as release-$VERSION)
cd ../..
tar -czf hardinfo2-$VERSION.tar.gz hardinfo2 --transform s/hardinfo2/hardinfo2-$VERSION/
mv hardinfo2-$VERSION.tar.gz hardinfo2/build/
cd hardinfo2/build

#extract source
tar -xzf hardinfo2-$VERSION.tar.gz
cd hardinfo2-$VERSION
debmake
#fixup source from cpack - FIXME
cd debian
grep Maintainer ../../cpacksrc/DEBIAN/control >control.fixed
grep -v Homepage control |grep -v Description|grep -v auto-gen|grep -v Section|grep -v debmake |grep -v Maintainer >>control.fixed
echo "Homepage: https://hardinfo2.org">>control.fixed
echo "Description: Hardinfo2 - System Information and Benchmark" >>control.fixed
grep Recommends ../../cpacksrc/DEBIAN/control >>control.fixed
grep Section ../../cpacksrc/DEBIAN/control >>control.fixed
rm -f control
mv control.fixed control
cd ..

#create debian tar.gz
tar -czf ../hardinfo2-$VERSION.debian.tar.gz debian
cd ..
#rename source file
mv hardinfo2-$VERSION.tar.gz hardinfo2-$VERSION.orig.tar.gz

#create dsc
echo "Format: 3.0 (quilt)
Source: hardinfo2
Binary: hardinfo2
Architecture: any
Version: $VERSION">./hardinfo2-$VERSION.dsc
grep Maintainer ./cpacksrc/DEBIAN/control >>./hardinfo2-$VERSION.dsc
echo "Homepage: https://hardinfo2.org
Standards-Version: 4.1.3
Vcs-Browser: https://salsa.debian.org/hwspeedy/hardinfo2
Vcs-Git: https://salsa.debian.org/hwspeedy/hardinfo2.git
Build-Depends: cmake, debhelper (>= 11)
Package-List:
 hardinfo2 deb x11 optional arch=any
Checksums-Sha1:" >./hardinfo2-$VERSION.dsc
sha1sum hardinfo2-$VERSION.*.tar.gz >>./hardinfo2-$VERSION.dsc
echo "Checksums-Sha256:">>./hardinfo2-$VERSION.dsc
sha256sum hardinfo2-$VERSION.*.tar.gz >>./hardinfo2-$VERSION.dsc
echo "Files:">>./hardinfo2-$VERSION.dsc
md5sum hardinfo2-$VERSION.*.tar.gz >>./hardinfo2-$VERSION.dsc

echo "Debian Source Package Files ready in build:"
ls -l hardinfo2-$VERSION*.tar.gz
ls -l hardinfo2-$VERSION.dsc

#build from source
sudo apt install debhelper
cd hardinfo2-$VERSION
debuild -b -uc -us

#test package
ls ../hardinfo2_*.deb
sudo apt -y install ../hardinfo2_$VERSION-1_$ARCH.deb
apt info hardinfo2
