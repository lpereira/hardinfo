cmake_minimum_required(VERSION 3.0.0)
cmake_policy(VERSION 3.0.0)
project(hardinfo2)

set(HARDINFO2_VERSION "2.0.8")

option(HARDINFO2_GTK3 "Build for GTK3 (0/off for GTK2)" 1)
option(HARDINFO2_LIBSOUP3 "Build for libsoup-3.0(EXPERIMENTAL) (0/off for libsoup-2.4)" 0)

set(OVRARCH "" CACHE STRING "Override HARDINFO2_ARCH value")
set(OVRCPUINFO "" CACHE STRING "Specify a different file for /proc/cpuinfo")
set(OVRDTRROOT "" CACHE STRING "Specify a different path for /proc/device-tree")
set(DISTRO "" CACHE STRING "Distro build version eg. 1.el(rpm) or 1(deb)")

SET(CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
	set(HARDINFO2_DEBUG 1)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	set(HARDINFO2_OS "linux")
else()
	message(FATAL_ERROR "Unsupported operating system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	set(HARDINFO2_ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
	set(HARDINFO2_ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "(ppc|ppc32|ppc64|ppc64le)")
	set(HARDINFO2_ARCH "ppc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "mips")
	set(HARDINFO2_ARCH "mips")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "parisc.*")
	set(HARDINFO2_ARCH "parisc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sparc{32,64}")
	set(HARDINFO2_ARCH "sparc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm.*")
	set(HARDINFO2_ARCH "arm")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64.*")
	set(HARDINFO2_ARCH "arm")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "ia64")
	set(HARDINFO2_ARCH "ia64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "loongarch64")
	set(HARDINFO2_ARCH "loongarch64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "alpha")
	set(HARDINFO2_ARCH "alpha")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "s390.*")
	set(HARDINFO2_ARCH "s390")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "m68k")
	set(HARDINFO2_ARCH "m68k")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sh[3-5]")
	set(HARDINFO2_ARCH "sh")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "(riscv|riscv32|riscv64)")
	set(HARDINFO2_ARCH "riscv")
else()
	message(FATAL_ERROR "Unsupported architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

if (OVRARCH)
	set(HARDINFO2_ARCH ${OVRARCH})
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

message(STATUS "Building HardInfo2 for architecture: ${HARDINFO2_OS}-${HARDINFO2_ARCH}")

add_definitions("-std=gnu89")
#add_definitions("-Wno-deprecated-declarations")
#add_definitions("-Wextra")
add_definitions("-Wno-missing-field-initializers")
add_definitions("-Wformat")
add_definitions("-Wformat-security")

set(PACKAGE_LIBSOUP2_MINVERSION 2.42)
set(PACKAGE_LIBGLIB2_MINVERSION 2.24)

###################################DISTRO CHANGES###############################
#Please add all distro relates stuff here
SET(PACKAGE_MANPAGES 1)
set(RPM 0)
file(STRINGS /etc/os-release distro REGEX "^NAME=")
string(REGEX REPLACE "NAME=" "" distro "${distro}")
string(REGEX REPLACE "\"" "" distro "${distro}")
string(REGEX REPLACE "/" "" distro "${distro}")
string(REGEX REPLACE " " "" distro "${distro}")
file(STRINGS /etc/os-release disversion REGEX "^VERSION_ID=")
string(REGEX REPLACE "VERSION_ID=" "" disversion "${disversion}")
string(REGEX REPLACE "\"" "" disversion "${disversion}")
string(REGEX REPLACE "/" "" disversion "${disversion}")
string(REGEX REPLACE " " "" disversion "${disversion}")
#RPM Distro's
if(${distro} MATCHES "Alma*" OR ${distro} MATCHES "Rocky*" OR ${distro} MATCHES "Fedora*" OR ${distro} MATCHES "Red*" OR ${distro} MATCHES "CentOS*" OR ${distro} MATCHES "Oracle*" OR ${distro} MATCHES "openSUSE*")
  set(RPM 1)
endif()
#Package architecture
if(${RPM})
  set(PACKAGE_DELIMITER "-")
  set(PACKAGE_ARCHITECTURE ".${CMAKE_HOST_SYSTEM_PROCESSOR}")
else()
  set(PACKAGE_DELIMITER "_")
  set(PACKAGE_ARCHITECTURE "_${CMAKE_HOST_SYSTEM_PROCESSOR}")
  if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
    set(PACKAGE_ARCHITECTURE "_amd64")
  endif()
endif()
#Package Distro build version - part of package name
set(PACKAGE_DISTRO ${HARDINFO2_VERSION}-${distro}-${disversion}${CPACK_SYSTEM_NAME})
SET(CPACK_PACKAGE_VERSION ${HARDINFO2_VERSION})
if(DISTRO)
    string(REGEX REPLACE "[.].*" "" DISTRO_BUILD_VER ${DISTRO})
    set(CPACK_PACKAGE_VERSION "${HARDINFO2_VERSION}-${DISTRO_BUILD_VER}")
    set(PACKAGE_DISTRO "${HARDINFO2_VERSION}-${DISTRO}")
    if(${HARDINFO2_GTK3})
    else()
      message(FATAL_ERROR "Distro-Building not supported with GTK2!!")
    endif()
endif()
#Put it together
string(REPLACE " " "_" CPACK_PACKAGE_FILE_NAME "hardinfo${PACKAGE_DELIMITER}${PACKAGE_DISTRO}${PACKAGE_ARCHITECTURE}")
message("-- Found Package Target ${CPACK_PACKAGE_FILE_NAME}")
message("-- Found Package Version ${CPACK_PACKAGE_VERSION}")

#flavour output
set(HARDINFO2_OS ${distro})

# currently all RPM flavours uses libsoup-2.4
if(${RPM})
  set(HARDINFO2_LIBSOUP3 0)
endif()

#oracle 6
if(${distro}${disversion} MATCHES "Oracle6*")
    set(HARDINFO2_LIBSOUP3 0)
    set(HARDINFO2_GTK3 0)
    #old deprecated version - but ok
    set(PACKAGE_LIBSOUP2_MINVERSION 2.34.3)
endif()

#debian 7
if(${distro}${disversion} MATCHES "DebianGNULinux7")
    set(HARDINFO2_LIBSOUP3 0)
    set(HARDINFO2_GTK3 0)
    #old deprecated version - but ok
    set(PACKAGE_LIBSOUP2_MINVERSION 2.34.3)
endif()

#debian 8
if(${distro}${disversion} MATCHES "DebianGNULinux8")
    set(HARDINFO2_LIBSOUP3 0)
endif()

#ubuntu 20.04
if(${distro}${disversion} MATCHES "Ubuntu20.04")
    set(HARDINFO2_LIBSOUP3 0)
endif()

#Debian 11
if(${distro}${disversion} MATCHES "DebianGNULinux11")
    set(HARDINFO2_LIBSOUP3 0)
endif()

#Fedora 39 - bug with man pages - probably needs upgrade..
if(${distro}${disversion} MATCHES "FedoraLinux39")
  set(PACKAGE_MANPAGES 0)
endif()

################################################################################
add_subdirectory(po)


include(FindPkgConfig)
if (HARDINFO2_GTK3)
    message(STATUS "Building for GTK3")
    pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.0 cairo>=1.0 cairo-png>=1.0 gthread-2.0>=2.10 gmodule-export-2.0>=2.10)
    add_definitions(-DGTK_DISABLE_SINGLE_INCLUDES)
else()
    message(STATUS "Building for GTK2 - (Deprecated - Please update your distro!!)")
    pkg_check_modules(GTK REQUIRED gtk+-2.0>=2.10 gthread-2.0>=2.10 gmodule-export-2.0>=2.10)
endif()

if (HARDINFO2_LIBSOUP3)
    pkg_check_modules(LIBSOUP REQUIRED libsoup-3.0>=3.00)
else()
    pkg_check_modules(LIBSOUP REQUIRED libsoup-2.4>=${PACKAGE_LIBSOUP2_MINVERSION})
endif()
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=${PACKAGE_LIBGLIB2_MINVERSION})
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0>=0.14.2)
pkg_check_modules(X11 REQUIRED x11)

include(FindZLIB REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/includes
	${CMAKE_SOURCE_DIR}/includes/${HARDINFO2_ARCH}
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

set(HARDINFO2_MODULES
	computer
	devices
	benchmark
	network
)
set(HARDINFO2_RESOURCES
	"data/benchmark.data"
	"data/benchmark.json"
	"data/vendor.ids"
	"data/sdcard.ids"
	"data/usb.ids"
	"data/arm.ids"
	"data/edid.ids"
	"data/ieee_oui.ids"
	"data/pci.ids"
	"data/kernel-module-icons.json"
)
if(${PACKAGE_MANPAGES})
set(HARDINFO2_MANPAGES
	"hardinfo2.1"
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
	modules/devices/${HARDINFO2_ARCH}/processor.c
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
	modules/benchmark/iperf3.c
)
set(MODULE_benchmark_SOURCES_GTK2
	modules/benchmark/drawing.c
	modules/benchmark/guibench.c
)
if (HARDINFO2_GTK3)
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
	hardinfo2/problem_marker.c
	hardinfo2/hinote_util.c
	PROPERTIES
	COMPILE_FLAGS "-std=c99 -Wall -Wextra -Wno-unused-function -Wno-deprecated-declarations -Wno-switch"
)

foreach (_module ${HARDINFO2_MODULES})
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

if (HARDINFO2_GTK3)
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
	hardinfo2/usb_util.c
	hardinfo2/pci_util.c
	hardinfo2/gpu_util.c
	hardinfo2/cpu_util.c
	hardinfo2/x_util.c
	hardinfo2/dt_util.c
	PROPERTIES
	COMPILE_FLAGS "-Wall -Wextra -Wno-parentheses -Wno-unused-function"
)

if (HARDINFO2_GTK3)
add_executable(hardinfo2
	hardinfo2/usb_util.c
	hardinfo2/pci_util.c
	hardinfo2/binreloc.c
	hardinfo2/expr.c
        hardinfo2/hardinfo.c
	hardinfo2/socket.c
	hardinfo2/util.c
	hardinfo2/gg_key_file_parse_string_as_value.c
	hardinfo2/gg_strescape.c
	hardinfo2/problem_marker.c
	hardinfo2/hinote_util.c
	hardinfo2/vendor.c
	hardinfo2/info.c
	hardinfo2/cpu_util.c
	hardinfo2/dmi_util.c
	hardinfo2/dt_util.c
	hardinfo2/x_util.c
	hardinfo2/gpu_util.c
	hardinfo2/udisks2_util.c
	hardinfo2/storage_util.c
	shell/callbacks.c
	shell/iconcache.c
	shell/menu.c
	shell/report.c
	shell/shell.c
	shell/stock.c
	shell/syncmanager.c
	shell/loadgraph-uber.c
)
target_link_libraries(hardinfo2
	-Wl,-whole-archive sysobj_early -Wl,-no-whole-archive
	uber-graph
	${GTK_LIBRARIES}
	${LIBSOUP_LIBRARIES}
	m
	${ZLIB_LIBRARIES}
	${X11_LIBRARIES}
	${JSON_GLIB_LIBRARIES}
)
set_target_properties(hardinfo2 PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
else()
add_executable(hardinfo2
	hardinfo2/usb_util.c
	hardinfo2/pci_util.c
	hardinfo2/binreloc.c
	hardinfo2/expr.c
        hardinfo2/hardinfo.c
	hardinfo2/socket.c
	hardinfo2/util.c
	hardinfo2/gg_key_file_parse_string_as_value.c
	hardinfo2/gg_strescape.c
	hardinfo2/problem_marker.c
	hardinfo2/hinote_util.c
	hardinfo2/vendor.c
	hardinfo2/info.c
	hardinfo2/cpu_util.c
	hardinfo2/dmi_util.c
	hardinfo2/dt_util.c
	hardinfo2/x_util.c
	hardinfo2/gpu_util.c
	hardinfo2/udisks2_util.c
	hardinfo2/storage_util.c
	shell/callbacks.c
	shell/iconcache.c
	shell/menu.c
	shell/report.c
	shell/shell.c
	shell/stock.c
	shell/syncmanager.c
	shell/loadgraph.c
)
target_link_libraries(hardinfo2
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
configure_file(hardinfo2.desktop.cmake ${CMAKE_BINARY_DIR}/hardinfo2.desktop @ONLY)

install(TARGETS hardinfo2 ${HARDINFO2_MODULES}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
install(FILES ${CMAKE_BINARY_DIR}/hardinfo2.desktop
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES ${HARDINFO2_RESOURCES}
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/hardinfo2
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(DIRECTORY pixmaps
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/hardinfo2
	PATTERN "*.{png,svg}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES ${HARDINFO2_MANPAGES}
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES pixmaps/hardinfo2.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

########################PACKAGE BUILDING FOR DISTROS###########################
#Please update here for building packages
#keep package name as hardinfo to allow for upgrading in distro
SET(CPACK_PACKAGE_NAME "hardinfo")
SET(CPACK_PACKAGE_HOMEPAGE_URL "https://www.hardinfo2.org")
if(${RPM})
    SET(CPACK_GENERATOR "RPM")
    SET(PACKAGE_GTK2 "libgtk-2-0")
else()
    SET(CPACK_GENERATOR "DEB")
    SET(PACKAGE_GTK2 "libgtk2.0-0")
endif()

#RPM
SET(CPACK_RPM_PACKAGE_MAINTAINER "hwspeedy <hardinfo2@bigbear.dk>")
SET(CPACK_RPM_PACKAGE_DESCRIPTION "Hardinfo2 is a small application that displays information about your hardware and operating system.  It has online benchmarking to check your machine performance against other machines.")
SET(CPACK_RPM_PACKAGE_RECOMMENDS "sysbench, udisks2, mesa-utils, lsscsi, lm-sensors, xdg-utils, iperf3")
SET(CPACK_RPM_PACKAGE_LICENSE "GPL2+")
SET(CPACK_RPM_PACKAGE_HOMEPAGE "https://github.com/hardinfo2/hardinfo2")
if(HARDINFO2_LIBSOUP3)
    SET(PACKAGE_LS "libsoup (>=3.0)")
else()
    SET(PACKAGE_LS "libsoup (>=${PACKAGE_LIBSOUP2_MINVERSION})")
endif()
if(HARDINFO2_GTK3)
    SET(PACKAGE_GTK "libgtk-3-0 (>=3.0)")
else()
    SET(PACKAGE_GTK "${PACKAGE_GTK2} (>=2.20)")
endif()
SET(CPACK_RPM_PACKAGE_DEPENDS "libjson-glib-1.0-0, zlib, ${PACKAGE_LS}, ${PACKAGE_GTK}, libglib2.0-0 (>=2.10)")

#DEB
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "hwspeedy <hardinfo2@bigbear.dk>")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Hardinfo2 is a small application that displays information about your hardware and operating system. It has online benchmarking to check your machine performance against other machines.")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "sysbench, udisks2, mesa-utils, lsscsi, lm-sensors, xdg-utils, iperf3")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/hardinfo2/hardinfo2")
if(HARDINFO2_LIBSOUP3)
    SET(PACKAGE_LS "libsoup-3.0.0 (>=3.0)")
else()
    SET(PACKAGE_LS "libsoup2.4-1 (>=${PACKAGE_LIBSOUP2_MINVERSION})")
    if(${distro}${disversion} MATCHES "Debian GNU/Linux11")
        SET(PACKAGE_LS "libsoup2.4-0 (>=2.42)")
    endif()
endif()
if(HARDINFO2_GTK3)
    SET(PACKAGE_GTK "libgtk-3-0 (>=3.0)")
else()
    SET(PACKAGE_GTK "${PACKAGE_GTK2} (>=2.20)")
endif()
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-glib-1.0-0, zlib1g, ${PACKAGE_LS}, ${PACKAGE_GTK}")

INCLUDE(CPack)
