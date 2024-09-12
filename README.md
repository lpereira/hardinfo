[![Test](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml/badge.svg)](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml)
[![GitHub release](https://img.shields.io/github/v/release/hardinfo2/hardinfo2?display_name=release)](https://hardinfo2.org/github?latest_release)
[![GitHub release](https://img.shields.io/github/v/release/hardinfo2/hardinfo2?include_prereleases&label=PreRelease&color=blue&display_name=release)](https://hardinfo2.org/github?latest_prerelease)

HARDINFO2
=========

Hardinfo2 is based on hardinfo, which has not been released >10 years. Hardinfo2 is the reboot that was needed.

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
- Development: Currently done by community, hwspeedy maintains

Server code can be found here: [https://github.com/hardinfo2/server](https://github.com/hardinfo2/server)

Packaging status
--------------
[![Packaging status](https://hardinfo2.org/repology.svg)](https://hardinfo2.org/repology.svg)

Prebuilds: [Downloads](https://hardinfo2.org/github?latest_prerelease)

Dependencies
------------
- GTK3 >=3.00 or GTK2+ >=2.20 - (GTK2+ DEPRECATED: cmake -DHARDINFO2_GTK3=0 ..)
- GLib >=2.24
- Zlib
- glib JSON
- Libsoup3 >=3.00 or Libsoup24 >=2.42 (LS24: cmake -DHARDINFO2_LIBSOUP3=0 ..)
- Qt5 >=5.10 (disable QT5/OpenGL: cmake -DHARDINFO2_QT5=0 ..)

Building and installing
-----------------------
**Debian/Ubuntu/Mint/PopOS**
- sudo apt install git cmake build-essential gettext curl
- sudo apt install libjson-glib-dev zlib1g-dev libsoup2.4-dev libgtk-3-dev libglib2.0-dev libqt5opengl5-dev qtbase5-dev
- sudo apt install libsoup-3.0-dev  (might fail if not available on distro - OK)
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- ./tools/git_latest_release.sh (Switch to latest stable release, tools/git_unstable_master.sh for developers)
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not pollute your distro and it can be updated by distro releases)
- sudo apt install ./hardinfo2_*  (Use reinstall instead of install if already installed)
- sudo apt install lm-sensors sysbench lsscsi mesa-utils dmidecode udisks2 xdg-utils iperf3 fwupd x11-xserver-utils vulkan-tools
- hardinfo2

**Fedora/CentOS/RedHat/Rocky/Alma/Oracle**
* NOTE: CentOS 7 needs epel-release and cmake3 instead of cmake - use cmake3 instead of cmake
- sudo yum install epel-release  (only CentOS 7)
- sudo yum install git cmake gcc gcc-c++ gettext rpmdevtools curl
- sudo yum install json-glib-devel zlib-devel libsoup-devel gtk3-devel qt5-qtbase-devel
- sudo yum install libsoup3-devel  (might fail if not available on distro - OK)
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- ./tools/git_latest_release.sh (Switch to latest stable release, tools/git_unstable_master.sh for developers)
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not pollute your distro and it can be updated by distro releases)
- sudo yum install ./hardinfo2-*  (Use reinstall instead of install if already installed)
- sudo yum install lm_sensors sysbench lsscsi glx-utils dmidecode udisks2 xdg-utils iperf3 fwupd xrandr vulkan-tools
- hardinfo2

**openSUSE**: use zypper instead of yum, zypper --no-gpg-checks install ./hardinfo2-*
libqt5-qtbase-devel instead of qt5-qtbase-devel

**ArchLinux/Garuda/Manjaro - AUR Package**
 - git clone https://aur.archlinux.org/hardinfo2 (hardinfo2-git for unstable master for developers)
 - cd hardinfo2
 - makepkg -cis
 - hardinfo2

Setting up addition tools
---------------------------
Most hardware is detected automatically by Hardinfo2, but some might need manual set up.

**Package installs these**
- **sysbench**: is needed to run standard sysbench benchmarks.
- **udisks2**: is needed to provide NVME++ informations.
- **dmi-decode**: is needed to provide DMI informations.
- **mesa-utils**: is needed to provide opengl and run standard sysbench benchmarks.
- **lsscsi**: gives information about hard drives.
- **lm-sensors**: is needed to provide sensors values.
- **xdg-utils**: xdg_open is used to open your browser for bugs, homepage & links.
- **iperf3**: iperf3 is used to benchmark internal network speed.
- **fwupd**: fwupd is used to read and display information about firmware in system.
- **Service**: Service loads SPD modules (at24/ee1004/spd5118) to display SPD info for your DIMMs memory. Show addresses for iomem+ioports.
- **xrandr/x11-xserver-utils**: xrandr is used to read monitor setup
- **vulkan-tools**: vulkaninfo is used to display vulcan information.

**User can install/setup these depending on hardware**
- **hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.
- **apcaccess**: apcaccess is used for battery information.
- **pciutils/usbutils**: is only used on old kernels without sysfs.

License
------
The Project License has been changed in 2024 from GPL2 to **GPL2 or later**

Because we use LGPL2.1+ and GPL3+ code. To future proof the project, lpereira and other developers have agreed to change license of source code also to GPL2+. [530](https://github.com/hardinfo2/hardinfo2/blob/master/tools/LICENSES/github_com_lpereira_hardinfo_issues_530.pdf)  [707](https://github.com/hardinfo2/hardinfo2/blob/master/tools/LICENSES/github_com_lpereira_hardinfo_issues_707.pdf).

It is all about open source and creating together - Read more about GPL license here: https://www.gnu.org/licenses/gpl-faq.html#AllCompatibility

Privacy Policy
---------------
When using the Synchronize feature in Hardinfo2, some information may be stored indefinitely in our servers.

This information is completely anonymous, and is comprised solely from the machine configuration (e.g. CPU manufacturer and model, number of cores, maximum frequency of cores, GPU manufacturer and model, etc.), version of benchmarking tools used, etc. You can opt out by unticking the "Send benchmark results" entry in the Synchronize window.

Both the Hardinfo2 client and its server components are open source GPL2 or Later and can be audited.
