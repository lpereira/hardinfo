[![Test](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml/badge.svg)](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml)
[![GitHub release](https://img.shields.io/github/release/hardinfo2/hardinfo2.svg)](https://github.com/hardinfo2/hardinfo2/releases)
[![GitHub release](https://img.shields.io/badge/PreRelease-v2.0.3-blue.svg)](https://github.com/hardinfo2/hardinfo2/releases/tag/release-2.0.3pre)

HARDINFO2
=========

Hardinfo2 is based on hardinfo, which have not been released >10 years. Hardinfo2 is the reboot that was needed.

Hardinfo2 offers System Information and Benchmark for Linux Systems. It is able to
obtain information from both hardware and basic software. It can benchmark your system and compare
to other machines online.

Features include:
- Report generation (in either HTML or plain text)
- Online Benchmarking - compare your machine against other machines

Status
------
- Capabilities: Hardinfo2 currently detects most software and hardware detected by the OS.
- Features: Online database for exchanging benchmark results.
- Development: Currently done by contributors, hwspeedy maintains

Server code can be found here: [https://github.com/hardinfo2/server](https://github.com/hardinfo2/server)

Dependencies
------------
- GTK3 >=3.00 or GTK2+ >=2.20 - Prefer **GTK3** (GTK2+: cmake -DHARDINFO2_GTK3=0 ..)
- GLib >=2.24
- Zlib
- glib JSON
- Libsoup2.4 >=2.42 or Libsoup-3.0 (EXPERIMENTAL) (LS3: cmake -DHARDINFO2_LIBSOUP3=1 ..)

Building and installing
-----------------------
**Debian/Ubuntu/Mint/PopOS**
- sudo apt install git cmake build-essential gettext
- sudo apt install libjson-glib-dev zlib1g-dev libsoup2.4-dev libgtk-3-dev libglib2.0-dev
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not polute your distro and it can be updated by distro releases)
- sudo apt install ./hardinfo_*  (Use reinstall instead of install if already inst.)
- sudo apt install lm-sensors sysbench lsscsi mesa-utils dmidecode udisks2 xdg-utils
- hardinfo2

**Fedore/Centos/RedHat/Rocky/Alma/Oracle**
* NOTE: Centos 7 needs epel-release and cmake3 instead of cmake - use cmake3 instead of cmake
- sudo yum install epel-release  (only CentOS 7)
- sudo yum install git cmake gcc gcc-c++ gettext rpmdevtools
- sudo yum install json-glib-devel zlib-devel libsoup-devel gtk3-devel
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not polute your distro and it can be updated by distro releases)
- sudo yum install ./hardinfo-*  (Use reinstall instead of install if already inst.)
- sudo yum install lm_sensors sysbench lsscsi glx-utils dmidecode udisks2 xdg-utils
- hardinfo2

**openSUSE**: use zypper instead of yum, use libsoup2-devel instead of libsoup-devel
  
Distro building
---------------
For distribution in the different distros - please use the cmake build system with CPack:
- cmake -DDISTRO=1.el8 ..   (rpm style)
- cmake -DDISTRO=1 ..       (deb style)
- make package

Please add/change CMakeLists.txt for your distro, so we have an easy to use package for all distributions.


Setting up addition tools
---------------------------
Most hardware is detected automatically by Hardinfo2, but some might need manual set up.

- **sysbench**: is needed to run standard sysbench benchmarks.
- **udisks2**: is needed to provide NVME++ informations.
- **dmi-decode**: is needed to provide DMI informations.
- **mesa-utils**: is needed to provide opengl and run standard sysbench benchmarks.
- **lsscsi**: gives information about hard drives.
- **lm-sensors**: is needed to provide sensors values.
- **hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.
- **eeprom module**: must be loaded to display info about your currently installed memory.
Load with `modprobe eeprom` and refresh the module screen.
- **xdg-utils**: xdg_open is used to open your browser for bugs, homepage & links.
- **apcaccess**: apcaccess is used for battery information. (optional)
- **lspci/lsusb**: is used for bus information - installed by distro. (optional)

License
------
The Project License has been changed in 2024 from GPL2 to **GPL2 or later**

Because we use LGPL2.1+ and GPL3 code. To future proof the project, lpereira and other developers has agreed to change license of source code also to GPL2+. (https://github.com/lpereira/hardinfo/issues/530) (https://github.com/lpereira/hardinfo/issues/707).

It is all about open source and creating together - Read more about GPL license here: https://www.gnu.org/licenses/gpl-faq.html#AllCompatibility

Privacy Policy
---------------
When using the Synchronize feature in Hardinfo2, some information may be stored indefinitely in our servers.

This information is completely anonymous, and is comprised solely from the machine configuration (e.g. CPU manufacturer and model, number of cores, maximum frequency of cores, GPU manufacturer and model, etc.), version of benchmarking tools used, etc. You can opt out by unticking the "Send benchmark results" entry in the Synchronize window.

Both the Hardinfo2 client and its server components are open source GPL2 or Later and can be audited.
