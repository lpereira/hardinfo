project(HardInfo)
cmake_minimum_required(VERSION 2.6)

set(HARDINFO_VERSION "0.5.2pre")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set(HARDINFO_DEBUG 1)
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	set(HARDINFO_OS "linux")
else()
	message(FATAL_ERROR "Unsupported operating system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i[3-6]86|x86_64")
	set(HARDINFO_ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "ppc{32,64}")
	set(HARDINFO_ARCH "ppc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "mips")
	set(HARDINFO_ARCH "mips")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "parisc.*")
	set(HARDINFO_ARCH "parisc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sparc{32,64}")
	set(HARDINFO_ARCH "sparc")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm.*")
	set(HARDINFO_ARCH "arm")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "ia64")
	set(HARDINFO_ARCH "ia64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "alpha")
	set(HARDINFO_ARCH "alpha")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "s390.*")
	set(HARDINFO_ARCH "s390")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "m68k")
	set(HARDINFO_ARCH "m68k")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "sh{3,4}")
	set(HARDINFO_ARCH "sh")
else()
	message(FATAL_ERROR "Unsupported architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

message(STATUS "Building HardInfo for architecture: ${HARDINFO_OS}-${HARDINFO_ARCH}")

include(FindPkgConfig)
pkg_check_modules(GTK REQUIRED gtk+-2.0>=2.10 glib-2.0>=2.10 gthread-2.0>=2.10)
pkg_check_modules(LIBSOUP libsoup-2.4>=2.26)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/includes
	${CMAKE_SOURCE_DIR}/includes/${HARDINFO_ARCH}
	${CMAKE_BINARY_DIR}
	${GTK_INCLUDE_DIRS}
	${LIBSOUP_INCLUDE_DIRS}
)
link_directories(
	${GTK_LIBRARY_DIRS}
	${LIBSOUP_LIBRARY_DIRS}
)

set(HARDINFO_MODULES
	computer
	devices
	benchmark
	network
)
set(HARDINFO_RESOURCES
	"benchmark.conf"
	"benchmark.data"
)

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
	modules/computer/modules.c
	modules/computer/os.c
	modules/computer/uptime.c
	modules/computer/users.c
)
set(MODULE_devices_SOURCES
	modules/devices.c
	modules/devices/${HARDINFO_ARCH}/processor.c
	modules/devices/battery.c
	modules/devices/devmemory.c
	modules/devices/dmi.c
	modules/devices/inputdevices.c
	modules/devices/pci.c
	modules/devices/printers.c
	modules/devices/resources.c
	modules/devices/sensors.c
	modules/devices/storage.c
	modules/devices/usb.c
)
set(MODULE_network_SOURCES
	modules/network.c
	modules/network/net.c
	modules/network/nfs.c
	modules/network/samba.c
)
set(MODULE_benchmark_SOURCES
	modules/benchmark.c
	modules/benchmark/blowfish.c
	modules/benchmark/cryptohash.c
	modules/benchmark/drawing.c
	modules/benchmark/fbench.c
	modules/benchmark/fftbench.c
	modules/benchmark/fib.c
	modules/benchmark/guibench.c
	modules/benchmark/md5.c
	modules/benchmark/nqueens.c
	modules/benchmark/raytrace.c
	modules/benchmark/sha1.c
)

foreach (_module ${HARDINFO_MODULES})
	add_library(${_module} MODULE ${MODULE_${_module}_SOURCES})
endforeach()

add_library(hardinfo-remote STATIC
        remote/remote.c
        remote/ssh-conn.c
        remote/xmlrpc-client.c
        remote/xmlrpc-server.c
)
add_library(hardinfo-help-viewer STATIC
        help-viewer/egg-markdown.c
        help-viewer/help-viewer.c
        help-viewer/markdown-text-view.c
)
add_library(hardinfo-shell STATIC
        shell/callbacks.c
        shell/iconcache.c
        shell/loadgraph.c
        shell/menu.c
        shell/report.c
        shell/shell.c
        shell/stock.c
	shell/syncmanager.c
)
add_executable(hardinfo
	util/binreloc.c
	util/expr.c
	util/socket.c
	util/vendor.c
	util/util.c
        hardinfo/hardinfo.c
)
target_link_libraries(hardinfo 
	${GTK_LIBRARIES}
	${LIBSOUP_LIBRARIES}
	hardinfo-shell
)
target_link_libraries(hardinfo-shell
	hardinfo-help-viewer
	hardinfo-remote
)

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h @ONLY)

install(TARGETS hardinfo ${HARDINFO_MODULES}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib/hardinfo/modules
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
install(FILES ${HARDINFO_RESOURCES}
	DESTINATION share/hardinfo
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(DIRECTORY pixmaps
	DESTINATION share/hardinfo
	PATTERN "*.{png,svg}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(DIRECTORY doc
	DESTINATION share/hardinfo
	PATTERN "*.{hlp,png}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
	