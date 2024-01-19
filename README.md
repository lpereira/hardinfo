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
Development: Currently done by contributors, a new dedicated maintainer is needed.

Server code can be found in the "server" branch: https://github.com/lpereira/hardinfo/tree/server

DEPENDENCIES
------------

Required:
- GTK+ 2.10 (or newer) - Prefer GTK3
- GLib 2.10 (or newer)
- Zlib (for zlib benchmark)
- Json-glib

Optional (for synchronization/remote):
- Libsoup 2.24 (or newer) - Prefer for functionallity

BUILDING
--------

Create a build directory and build from there:

``` bash
	hardinfo $ mkdir build
	hardinfo $ cd build
	build $ cmake ..
	build $ make
```

There are some variables that can be changed:

 * `CMAKE_BUILD_TYPE`: Can either be ``Release`` or ``Debug``.
   * `[Default: Release]` ``Debug`` prints messages to console and is not recommended for general use.
 * `CMAKE_INSTALL_PREFIX`: Sets the installation prefix.
   * `[Default: /usr/local]`: Distributions usually change this to `/usr`.
 * `HARDINFO_NOSYNC`: Disables network synchronization.

To set a variable, use cmake's -D parameter. For example:

`	build $ cmake .. -DCMAKE_BUILD_TYPE=Debug `

Network sync is enabled if libsoup is detected. If having trouble building with libsoup, disable it with:

`	build $ cmake -DHARDINFO_NOSYNC=1`

SETTING UP
----------

Most hardware is detected automatically by HardInfo, however, some hardware 
needs manual set up. They are:

### Sensors

**lm-sensors**: If your computer is compatible with lm-sensors module, use by example the
`sensors-detect` program included with the lm-sensors package of Debian based distros, and be sure
to have the detected kernel modules loaded.

**hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.

### Memory Speed

The module `eeprom` must be loaded to display info about your currently installed memory.
Load with `modprobe eeprom` and refresh the module screen.
