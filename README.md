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

Capabilities: HardInfo currently detects most software and hardware detected by the OS.
Features: Online database for exchanging benchmark results.
Development: Currently done by contributors.

Server code can be found in the "server" branch: https://github.com/lpereira/hardinfo/tree/server

DEPENDENCIES
------------
- GTK+ 2.10 (or newer) - Prefer **GTK3**
- GLib 2.10 (or newer)
- Zlib (for zlib benchmark)
- Json-glib
- Libsoup 2.24 (or newer) - optional - for network synchronization but prefered for functionallity

BUILDING
--------
**Debian/Ubuntu/Mint/PopOS**
- sudo apt install git cmake build-essential gettext
- sudo apt install libjson-glib-dev zlib1g-dev libsoup2.4-dev
- sudo apt install libgtk-3-dev libglib2.0-dev
- git clone https://github.com/hwspeedy/hardinfo
- cd hardinfo
- mkdir build
- cd build
- cmake ..
- make
- sudo make install
- hardinfo

**Fedore/Centos/RedHat/Rocky/Alma/Oracle**
- sudo yum install git cmake gcc gcc-c++ 
- sudo yum install json-glib-devel zlib-devel libsoup-devel gtk3-devel
- git clone https://github.com/hwspeedy/hardinfo
- cd hardinfo
- mkdir build
- cd build
- cmake ..   (NOTE: Centos 7 needs the cmake3 from epel)
- make
- sudo make install
- hardinfo


There are some build variables that can be changed:
 * `CMAKE_BUILD_TYPE`: Can either be ``Release`` or ``Debug``.
   * `[Default: Release]` ``Debug`` prints messages to console and is not recommended for general use.
 * `CMAKE_INSTALL_PREFIX`: Sets the installation prefix.
   * `[Default: /usr/local]`: Distributions usually change this to `/usr`.
 * `HARDINFO_NOSYNC`: Network sync is enabled if libsoup is detected. Disables network synchronization.

To set a variable, use cmake's -D parameter. For example:
`	build $ cmake .. -DCMAKE_BUILD_TYPE=Debug `


SETTING UP
----------
Most hardware is detected automatically by HardInfo, however, some hardware 
needs manual set up.

- **sysbench**: is needed to run standard sysbench benchmarks
- **lsscsi**: gives information about hard drives
- **lm-sensors**: If your computer is compatible with lm-sensors module, use by example the
`sensors-detect` program included with the lm-sensors package of Debian based distros, and be sure
to have the detected kernel modules loaded.
- **hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.
- **eeprom module**: must be loaded to display info about your currently installed memory.
Load with `modprobe eeprom` and refresh the module screen.
