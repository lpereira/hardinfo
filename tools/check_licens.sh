#!/bin/bash

cd ..
rm -rf build
mkdir build
cd build

licensecheck -r .. | grep '\.c: \|\.h: ' >licenses_all.txt

echo "GPL2.0+:"
cat licenses_all.txt| grep 'General Public License v2.0 or later\|GNU Library General Public License v2 or later'
echo ""
									 
echo "LGPL2.1+:"
cat licenses_all.txt| grep 'GNU Lesser General Public License v2.1 or later'
echo ""

echo "LGPL2.1:"
cat licenses_all.txt| grep -v 'GNU Lesser General Public License v2.1 or later' | grep 'GNU Lesser General Public License, Version 2.1'
echo ""

echo "GPL3.0+:"
cat licenses_all.txt| grep 'General Public License v3.0 or later'
echo ""
   
#remaining licenses
echo "Others:"
echo "includes/loongarch64/processor-platform.h detected wrong - is GPL2+:"
cat licenses_all.txt|grep -v 'General Public License v2.0 or later\|GNU Library General Public License v2 or later' \
    | grep -v 'GNU Lesser General Public License v2.1 or later'| grep -v 'GNU Lesser General Public License, Version 2.1' \
    | grep -v 'General Public License v3.0 or later' \
    | grep -v '*No copyright*'

echo ""
