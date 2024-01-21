cmake_minimum_required(VERSION 2.8.12)
cmake_policy(VERSION 2.8.12)
project(hardinfo)

set(HARDINFO_VERSION "1.0.1")
option(HARDINFO_GTK3 "Build for GTK3 (0/off for GTK2)" 1)
option(HARDINFO_LIBSOUP3 "Build for libsoup-3.0 (0/off for libsoup-2.4)" 1)
set(OVRARCH "" CACHE STRING "Override HARDINFO_ARCH value")
set(OVRCPUINFO "" CACHE STRING "Specify a different file for /proc/cpuinfo")
set(OVRDTRROOT "" CACHE STRING "Specify a different path for /proc/device-tree")
SET( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
include(GNUInstallDirs)

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
	set(HARDINFO_DEBUG 1)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	set(HARDINFO_OS "linux")
else()
	message(FATAL_ERROR "Unsupported operating system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	set(HARDINFO_ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
	set(HARDINFO_ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "(ppc|ppc32|ppc64|ppc64le)")
	set(HARDINFO_ARCH "ppc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "mips")
	set(HARDINFO_ARCH "mips")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "parisc.*")
	set(HARDINFO_ARCH "parisc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sparc{32,64}")
	set(HARDINFO_ARCH "sparc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm.*")
	set(HARDINFO_ARCH "arm")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64.*")
	set(HARDINFO_ARCH "arm")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "ia64")
	set(HARDINFO_ARCH "ia64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "loongarch64")
	set(HARDINFO_ARCH "loongarch64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "alpha")
	set(HARDINFO_ARCH "alpha")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "s390.*")
	set(HARDINFO_ARCH "s390")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "m68k")
	set(HARDINFO_ARCH "m68k")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sh[3-5]")
	set(HARDINFO_ARCH "sh")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "(riscv|riscv32|riscv64)")
	set(HARDINFO_ARCH "riscv")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "e2k")
        set(HARDINFO_ARCH "e2k")
else()
	message(FATAL_ERROR "Unsupported architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

if (OVRARCH)
	set(HARDINFO_ARCH ${OVRARCH})
endif()
if (OVRCPUINFO)
	add_definitions(-DPROC_CPUINFO=${OVRCPUINFO})
	message(STATUS "/proc/cpuinfo override: ${OVRCPUINFO}")
endif()
if (OVRDTRROOT)
	add_definitions(-DDTR_ROOT=${OVRDTRROOT})
	message(STATUS "/proc/device-tree override: ${OVRDTRROOT}")
endif()
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}")
message(STATUS "LOCALEDIR = ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}")

message(STATUS "Building HardInfo for architecture: ${HARDINFO_OS}-${HARDINFO_ARCH}")

add_definitions("-std=gnu89")
#add_definitions("-Wno-deprecated-declarations")
#add_definitions("-Wextra")
add_definitions("-Wformat")
add_definitions("-Wformat-security")

###################################DISTRO CHANGES###############################
#Please add all distro relates stuff here
SET(PACKAGE_MANPAGES 1)
set(RPM 0)
file(STRINGS /etc/os-release distro REGEX "^NAME=")
string(REGEX REPLACE "NAME=\"(.*)\"" "\\1" distro "${distro}")
file(STRINGS /etc/os-release disversion REGEX "^VERSION_ID=")
string(REGEX REPLACE "VERSION_ID=" "" disversion "${disversion}")
string(REGEX REPLACE "\"" "" disversion "${disversion}")
string(REPLACE "/" "_" CPACK_SYSTEM_NAME ${distro}-${disversion}.${CMAKE_HOST_SYSTEM_PROCESSOR})
string(REPLACE " " "_" CPACK_SYSTEM_NAME ${CPACK_SYSTEM_NAME})
message("-- Found Package Target ${CPACK_SYSTEM_NAME}")

#flavour output
set(HARDINFO_OS ${distro})

# currently all RPM flavours uses libsoup-2
if(${distro} MATCHES "Fedora*" OR ${distro} MATCHES "Red*" OR ${distro} MATCHES "CentOS*")
  set(RPM 1)
  message(STATUS "Building for LibSoup 2.4 due to Fedora/Rehat/Centos... not upgraded with libsoup 3.0")
  set(HARDINFO_LIBSOUP3 0)
endif()

#ubuntu 20.04
if(${distro}${disversion} MATCHES "Ubuntu20.04")
  set(HARDINFO_LIBSOUP3 0)
endif()

#Fedora 39
if(${distro}${disversion} MATCHES "Fedora Linux39")
  set(PACKAGE_MANPAGES 0)
endif()

################################################################################
add_subdirectory(po)


include(FindPkgConfig)
if (HARDINFO_GTK3)
	message(STATUS "Building for GTK3")
	pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.0 cairo>=1.0 cairo-png>=1.0 glib-2.0>=2.10 gthread-2.0>=2.10 gmodule-export-2.0>=2.10)
	add_definitions(-DGTK_DISABLE_SINGLE_INCLUDES)
else()
	message(STATUS "Building for GTK2")
	pkg_check_modules(GTK REQUIRED gtk+-2.0>=2.10 glib-2.0>=2.10 gthread-2.0>=2.10 gmodule-export-2.0>=2.10)
endif()

if (HARDINFO_LIBSOUP3)
pkg_check_modules(LIBSOUP libsoup-3.0>=3.00)
else()
pkg_check_modules(LIBSOUP libsoup-2.4>=2.42)
endif()
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)

pkg_check_modules(X11 REQUIRED x11)

include(FindZLIB REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/includes
	${CMAKE_SOURCE_DIR}/includes/${HARDINFO_ARCH}
	${CMAKE_SOURCE_DIR}/deps/uber-graph
	${CMAKE_SOURCE_DIR}/deps/sysobj_early/include
	${CMAKE_SOURCE_DIR}/deps/sysobj_early/gui
	${CMAKE_BINARY_DIR}
	${GTK_INCLUDE_DIRS}
	${LIBSOUP_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${X11_INCLUDE_DIRS}
	${JSON_GLIB_INCLUDE_DIRS}
)
link_directories(
	${GTK_LIBRARY_DIRS}
	${LIBSOUP_LIBRARY_DIRS}
	${X11_LIBRARY_DIRS}
	${JSON_GLIB_LIBRARY_DIRS}
)

set(HARDINFO_MODULES
	computer
	devices
	benchmark
	network
)
set(HARDINFO_RESOURCES
	"data/benchmark.data"
	"data/benchmark.json"
	"data/vendor.ids"
	"data/sdcard.ids"
	"data/usb.ids"
	"data/arm.ids"
	"data/edid.ids"
	"data/ieee_oui.ids"
	"data/pci.ids.min"
	"data/kernel-module-icons.json"
)
if(${PACKAGE_MANPAGES})
set(HARDINFO_MANPAGES
	"hardinfo.1"
)
endif()
set(MODULE_computer_SOURCES
	modules/computer.c
	modules/computer/alsa.c
	modules/computer/boots.c
	modules/computer/display.c
	modules/computer/environment.c
	modules/computer/filesystem.c
	modules/computer/languages.c
	modules/computer/loadavg.c
	modules/computer/memory.c
	modules/computer/memory_usage.c
	modules/computer/modules.c
	modules/computer/os.c
	modules/computer/ubuntu_flavors.c
	modules/computer/uptime.c
	modules/computer/users.c
	modules/computer/groups.c
)
set(MODULE_devices_SOURCES
	modules/devices.c
	modules/devices/${HARDINFO_ARCH}/processor.c
	modules/devices/gpu.c
	modules/devices/monitors.c
	modules/devices/battery.c
	modules/devices/dmi.c
	modules/devices/dmi_memory.c
	modules/devices/firmware.c
	modules/devices/devicetree.c
	modules/devices/inputdevices.c
	modules/devices/pci.c
	modules/devices/printers.c
	modules/devices/resources.c
	modules/devices/sensors.c
#	modules/devices/spd-decode.c
	modules/devices/storage.c
	modules/devices/usb.c
)
set(MODULE_network_SOURCES
	modules/network.c
	modules/network/net.c
	modules/network/nfs.c
	modules/network/samba.c
)
set(MODULE_benchmark_SOURCES_GTKANY
	modules/benchmark.c
	modules/benchmark/bench_util.c
	modules/benchmark/blowfish.c
	modules/benchmark/blowfish2.c
	modules/benchmark/cryptohash.c
	modules/benchmark/fbench.c
	modules/benchmark/fftbench.c
	modules/benchmark/fft.c
	modules/benchmark/fib.c
	modules/benchmark/md5.c
	modules/benchmark/nqueens.c
	modules/benchmark/raytrace.c
	modules/benchmark/sha1.c
	modules/benchmark/zlib.c
	modules/benchmark/sysbench.c
)
set(MODULE_benchmark_SOURCES_GTK2
	modules/benchmark/drawing.c
	modules/benchmark/guibench.c
)
if (HARDINFO_GTK3)
	set(MODULE_benchmark_SOURCES ${MODULE_benchmark_SOURCES_GTKANY})
else()
	set(MODULE_benchmark_SOURCES ${MODULE_benchmark_SOURCES_GTKANY} ${MODULE_benchmark_SOURCES_GTK2})
endif()

set_source_files_properties(
	modules/benchmark/blowfish.c
	modules/benchmark/fftbench.c
	modules/benchmark/md5.c
	modules/benchmark/sha1.c
	PROPERTIES
	COMPILE_FLAGS "-O0"
)

set_source_files_properties(
	modules/devices/monitors.c
	modules/devices/dmi_memory.c
#	modules/devices/spd-decode.c
	hardinfo/problem_marker.c
	hardinfo/hinote_util.c
	PROPERTIES
	COMPILE_FLAGS "-std=c99 -Wall -Wextra -Wno-unused-function -Wno-deprecated-declarations -Wno-switch"
)

foreach (_module ${HARDINFO_MODULES})
	add_library(${_module} MODULE ${MODULE_${_module}_SOURCES})
	set_target_properties(${_module} PROPERTIES PREFIX "")
	target_link_libraries(${_module} ${JSON_GLIB_LIBRARIES})
endforeach()

find_library(LIBSENSORS_LIBRARY NAMES libsensors.so)
if (LIBSENSORS_LIBRARY)
	set(HAS_LIBSENSORS 1)
	target_link_libraries(devices ${LIBSENSORS_LIBRARY})
endif ()

add_library(sysobj_early STATIC
	deps/sysobj_early/src/gg_slist.c
	deps/sysobj_early/src/strstr_word.c
	deps/sysobj_early/src/auto_free.c
	deps/sysobj_early/src/util_ids.c
	deps/sysobj_early/src/util_sysobj.c
	deps/sysobj_early/src/appf.c
	deps/sysobj_early/src/nice_name.c
	deps/sysobj_early/gui/uri_handler.c
	deps/sysobj_early/src/util_edid.c
	deps/sysobj_early/src/format_early.c
	deps/sysobj_early/src/cpubits.c
)
set_target_properties(sysobj_early PROPERTIES COMPILE_FLAGS "-std=c99 -Wall -Wextra -Wno-parentheses -Wno-unused-function")
target_link_libraries(sysobj_early m)

if (HARDINFO_GTK3)
add_library(uber-graph STATIC
	deps/uber-graph/g-ring.c
	deps/uber-graph/uber-frame-source.c
	deps/uber-graph/uber-graph.c
	deps/uber-graph/uber-heat-map.c
	deps/uber-graph/uber-label.c
	deps/uber-graph/uber-line-graph.c
	deps/uber-graph/uber-range.c
	deps/uber-graph/uber-scale.c
	deps/uber-graph/uber-scatter.c
	deps/uber-graph/uber-timeout-interval.c
	deps/uber-graph/uber-window.c
)
set_target_properties(uber-graph PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
endif()

set_source_files_properties(
	hardinfo/usb_util.c
	hardinfo/pci_util.c
	hardinfo/gpu_util.c
	hardinfo/cpu_util.c
	hardinfo/x_util.c
	hardinfo/dt_util.c
	PROPERTIES
	COMPILE_FLAGS "-Wall -Wextra -Wno-parentheses -Wno-unused-function"
)

if (HARDINFO_GTK3)
add_executable(hardinfo
	hardinfo/usb_util.c
	hardinfo/pci_util.c
	hardinfo/binreloc.c
	hardinfo/expr.c
        hardinfo/hardinfo.c
	hardinfo/socket.c
	hardinfo/util.c
	hardinfo/gg_key_file_parse_string_as_value.c
	hardinfo/gg_strescape.c
	hardinfo/problem_marker.c
	hardinfo/hinote_util.c
	hardinfo/vendor.c
	hardinfo/info.c
	hardinfo/cpu_util.c
	hardinfo/dmi_util.c
	hardinfo/dt_util.c
	hardinfo/x_util.c
	hardinfo/gpu_util.c
	hardinfo/udisks2_util.c
	hardinfo/storage_util.c
	shell/callbacks.c
	shell/iconcache.c
	shell/menu.c
	shell/report.c
	shell/shell.c
	shell/stock.c
	shell/syncmanager.c
	shell/loadgraph-uber.c
)
target_link_libraries(hardinfo
	-Wl,-whole-archive sysobj_early -Wl,-no-whole-archive
	uber-graph
	${GTK_LIBRARIES}
	${LIBSOUP_LIBRARIES}
	m
	${ZLIB_LIBRARIES}
	${X11_LIBRARIES}
	${JSON_GLIB_LIBRARIES}
)
set_target_properties(hardinfo PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
else()
add_executable(hardinfo
	hardinfo/usb_util.c
	hardinfo/pci_util.c
	hardinfo/binreloc.c
	hardinfo/expr.c
        hardinfo/hardinfo.c
	hardinfo/socket.c
	hardinfo/util.c
	hardinfo/gg_key_file_parse_string_as_value.c
	hardinfo/gg_strescape.c
	hardinfo/problem_marker.c
	hardinfo/hinote_util.c
	hardinfo/vendor.c
	hardinfo/info.c
	hardinfo/cpu_util.c
	hardinfo/dmi_util.c
	hardinfo/dt_util.c
	hardinfo/x_util.c
	hardinfo/gpu_util.c
	hardinfo/udisks2_util.c
	hardinfo/storage_util.c
	shell/callbacks.c
	shell/iconcache.c
	shell/menu.c
	shell/report.c
	shell/shell.c
	shell/stock.c
	shell/syncmanager.c
	shell/loadgraph.c
)
target_link_libraries(hardinfo
	-Wl,-whole-archive sysobj_early -Wl,-no-whole-archive
	${GTK_LIBRARIES}
	${LIBSOUP_LIBRARIES}
	m
	${ZLIB_LIBRARIES}
	${X11_LIBRARIES}
	${JSON_GLIB_LIBRARIES}
)
endif()

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h @ONLY)
configure_file(hardinfo.desktop.cmake ${CMAKE_BINARY_DIR}/hardinfo.desktop @ONLY)

install(TARGETS hardinfo ${HARDINFO_MODULES}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo/modules
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
install(FILES ${CMAKE_BINARY_DIR}/hardinfo.desktop
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES ${HARDINFO_RESOURCES}
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/hardinfo
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(DIRECTORY pixmaps
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/hardinfo
	PATTERN "*.{png,svg}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(DIRECTORY doc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/hardinfo
	PATTERN "*.{hlp,png}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES ${HARDINFO_MANPAGES}
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES pixmaps/hardinfo.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

########################PACKAGE BUILDING FOR DISTROS###########################
#Please update here for building packages
#currently all recommends packages are turned off as it's wrong - please help.

SET(CPACK_PACKAGE_VERSION ${HARDINFO_VERSION})
SET(CPACK_PACKAGE_HOMEPAGE_URL "https://hardinfo.bigbear.dk")
SET(CPACK_GENERATOR "RPM;DEB")
if(${RPM})
  SET(CPACK_GENERATOR "RPM")
else()
  SET(CPACK_GENERATOR "DEB")
endif()
SET(CPACK_RPM_PACKAGE_MAINTAINER "hwspeedy <hardinfo@bigbear.dk>")
SET(CPACK_RPM_PACKAGE_DESCRIPTION "HardInfo is a small application that displays information about your hardware and operating system. Currently it knows about PCI, ISA PnP, USB, IDE, SCSI, Serial and parallel port devices.")
#SET(CPACK_RPM_PACKAGE_RECOMMENDS "sysbench (>=1.0.0) udisk2 (>=1.0.0) mesa-utils (>=1.0.0) lsscsi (>=1.0.0) lm-sensors (>=1.0.0)")
SET(CPACK_RPM_PACKAGE_HOMEPAGE "https://github.com/hwspeedy/hardinfo")
if(HARDINFO_LIBSOUP3)
  SET(CPACK_RPM_PACKAGE_DEPENDS "libjson-glib-1.0-0 (>=1.0.0), zlib (>=1.0.0), libsoup (>=3.0.0), libgtk-3-0 (>=3.0.0), libglib2.0-0 (>=2.0.0)")
else()
  SET(CPACK_RPM_PACKAGE_DEPENDS "libjson-glib-1.0-0 (>=1.0.0), zlib (>=1.0.0), libsoup (>=2.4.0), libgtk-3-0 (>=3.0.0), libglib2.0-0 (>=2.0.0)")
endif()
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "hwspeedy <hardinfo@bigbear.dk>")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "HardInfo is a small application that displays information about your hardware and operating system. Currently it knows about PCI, ISA PnP, USB, IDE, SCSI, Serial and parallel port devices.")
#SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "sysbench (>=1.0.0) udisk2 (>=1.0.0) mesa-utils (>=1.0.0) lsscsi (>=1.0.0) lm-sensors (>=1.0.0)")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/hwspeedy/hardinfo")
if(HARDINFO_LIBSOUP3)
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-glib-1.0-0 (>=1.0.0), zlib1g (>=1.0.0), libsoup-3.0-0 (>=3.0.0), libgtk-3-0 (>=3.0.0)")
else()
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-glib-1.0-0 (>=1.0.0), zlib1g (>=1.0.0), libsoup2.4-0 (>=2.4.0), libgtk-3-0 (>=3.0.0)")
  if(${distro}${disversion} MATCHES "Ubuntu20.04")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-glib-1.0-0 (>=1.0.0), zlib1g (>=1.0.0), libsoup2.4-1 (>=2.4.0), libgtk-3-0 (>=3.0.0)")
  endif()
endif()  
INCLUDE(CPack)

