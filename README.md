[![Test](https://github.com/hwspeedy/hardinfo/actions/workflows/test.yml/badge.svg)](https://github.com/hwspeedy/hardinfo/actions/workflows/test.yml)
[![GitHub release](https://img.shields.io/github/release/hwspeedy/hardinfo.svg)](https://github.com/hwspeedy/hardinfo/releases)

HARDINFO
========

HardInfo is a system profiler and benchmark for Linux systems. It is able to
obtain information from both hardware and basic software, and organize it
in a simple to use GUI.

Features include:
- Report generation (in either HTML or plain text)
- Benchmark result synchronization
- Ability to explore the information on remote computers

Status
------

- Capabilities: HardInfo currently detects most software and hardware detected by the OS.
- Features: Online database for exchanging benchmark results.
- Development: Currently done by contributors, hwspeedy maintains (Missing OK from previous maintainer!!)

Server code can be found in the "server" branch: https://github.com/hwspeedy/hardinfo/tree/server

DEPENDENCIES
------------
- GTK+ 2.10 (or newer) - Prefer **GTK3**
- GLib 2.10 (or newer)
- Zlib (for zlib benchmark)
- Json-glib
- Libsoup 2.4 (or newer) - Prefer **Libsoup-3.0**

BUILDING
--------
**Debian/Ubuntu/Mint/PopOS**
NOTE: older version only has libsoup-2.4-dev so use "cmake -DHARDINFO_LIBSOUP3=0 .." instead of "cmake .."
- sudo apt install git cmake build-essential gettext
- sudo apt install libjson-glib-dev zlib1g-dev libsoup-3.0-dev libgtk-3-dev libglib2.0-dev
- git clone https://github.com/hwspeedy/hardinfo
- cd hardinfo
- mkdir build
- cd build
- cmake ..
- make
- sudo make install
- sudo apt install lm-sensors sysbench lsscsi mesa-utils dmidecode udisks2
- hardinfo

**Fedore/Centos/RedHat/Rocky/Alma/Oracle**
NOTE: older version only has libsoup-2.4-dev so use "cmake -DHARDINFO_LIBSOUP3=0 .." instead of "cmake .."
NOTE: Centos 7 needs yum install cmake3 instead of cmake and use "cmake3 -DHARDINFO_LIBSOUP3=0 .." instead of "cmake .."
- sudo yum install epel-release
- sudo yum install git cmake gcc gcc-c++ gettext
- sudo yum install json-glib-devel zlib-devel libsoup-devel gtk3-devel
- git clone https://github.com/hwspeedy/hardinfo
- cd hardinfo
- mkdir build
- cd build
- cmake ..
- make
- sudo make install
- sudo yum install lm_sensors sysbench lsscsi glx-utils dmidecode udisks2
- hardinfo


There are some build variables that can be changed for debug or distro release:
 * `CMAKE_BUILD_TYPE`: Can either be ``Release`` or ``Debug``.
   * `[Default: Release]` ``Debug`` prints messages to console and is not recommended for general use.
 * `CMAKE_INSTALL_PREFIX`: Sets the installation prefix.
   * `[Default: /usr/local]`: Distributions usually change this to `/usr`.
 
To set a variable, use cmake's -D parameter. For example:
`	build $ cmake .. -DCMAKE_BUILD_TYPE=Debug `


SETTING UP ADDITIONAL TOOLS
---------------------------
Most hardware is detected automatically by HardInfo, however, some hardware 
needs manual set up.

- **sysbench**: is needed to run standard sysbench benchmarks
- **udisks2**: is needed to provide NVME++ informations.
- **dmi-decode**: is needed to provide DMI informations - also requires root to use it.
- **mesa-utils**: is needed to provide opengl and run standard sysbench benchmarks
- **lsscsi**: gives information about hard drives
- **lm-sensors**: If your computer is compatible with lm-sensors module, use by example the
`sensors-detect` program included with the lm-sensors package of Debian based distros, and be sure
to have the detected kernel modules loaded.
- **hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.
- **eeprom module**: must be loaded to display info about your currently installed memory.
Load with `modprobe eeprom` and refresh the module screen.
