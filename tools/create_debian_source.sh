#!/bin/bash
VERSION=2.0.12
cd ..
rm -rf build
mkdir build
cd build
cmake ..
make package_source
tar -xzf hardinfo-$VERSION.tar.gz
cd hardinfo-$VERSION
debmake
tar -czf ../hardinfo-$VERSION.debian.tar.gz debian
cd ..
mv hardinfo-$VERSION.tar.gz hardinfo-$VERSION.orig.tar.gz
mv hardinfo-$VERSION.deb hardinfo-$VERSION.src.deb

echo "Format: 3.0 (quilt)
Source: hardinfo
Binary: hardinfo
Architecture: any
Version: $VERSION
Maintainer: hwspeedy <hardinfo2@bigbear.dk>
Homepage: https://hardinfo2.org
Standards-Version: 4.1.3
Vcs-Browser: https://salsa.debian.org/hwspeedy/hardinfo2
Vcs-Git: https://salsa.debian.org/hwspeedy/hardinfo2.git
Build-Depends: cmake, debhelper (>= 11), libjson-glib-dev, zlib1g-dev, libsoup2.4-dev, libgtk-3-dev
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
