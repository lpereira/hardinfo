#!/bin/bash
VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
cd ..
rm -rf build
sudo apt -y remove hardinfo

mkdir build
cd build
cmake -DDISTRO=src ..
make package_source
#rename cpack file
mv hardinfo-$VERSION.deb hardinfo-$VERSION.src.deb

#extract CPack source package
mkdir cpacksrc
dpkg-deb -R hardinfo-$VERSION.src.deb cpacksrc

#extract source
tar -xzf hardinfo-$VERSION.tar.gz
cd hardinfo-$VERSION
debmake
#fixup
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
tar -czf ../hardinfo-$VERSION.debian.tar.gz debian
cd ..
#rename cpack file
mv hardinfo-$VERSION.tar.gz hardinfo-$VERSION.orig.tar.gz

#create dsc
echo "Format: 3.0 (quilt)
Source: hardinfo
Binary: hardinfo
Architecture: any
Version: $VERSION">./hardinfo-$VERSION.dsc
grep Maintainer ./cpacksrc/DEBIAN/control >>./hardinfo-$VERSION.dsc
echo "Homepage: https://hardinfo2.org
Standards-Version: 4.1.3
Vcs-Browser: https://salsa.debian.org/hwspeedy/hardinfo2
Vcs-Git: https://salsa.debian.org/hwspeedy/hardinfo2.git
Build-Depends: cmake, debhelper (>= 11)
Package-List:
 hardinfo deb x11 optional arch=any
Checksums-Sha1:" >./hardinfo-$VERSION.dsc
sha1sum hardinfo-$VERSION.*.tar.gz >>./hardinfo-$VERSION.dsc
echo "Checksums-Sha256:">>./hardinfo-$VERSION.dsc
sha256sum hardinfo-$VERSION.*.tar.gz >>./hardinfo-$VERSION.dsc
echo "Files:">>./hardinfo-$VERSION.dsc
md5sum hardinfo-$VERSION.*.tar.gz >>./hardinfo-$VERSION.dsc

echo "Debian Source Package Files ready in build:"
ls -l hardinfo-$VERSION.src.deb
ls -l hardinfo-$VERSION.*.tar.gz
ls -l hardinfo-$VERSION.dsc

#build from source
sudo apt install debhelper
cd hardinfo-$VERSION
debuild -b -uc -us

#test package
ls ../hardinfo_*.deb
sudo apt -y install ../hardinfo_*.deb
apt info hardinfo
