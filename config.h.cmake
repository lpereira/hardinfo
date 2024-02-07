#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERSION			"@HARDINFO2_VERSION@"

#define ARCH			"ARCH_@HARDINFO2_ARCH@"
#define OS			"@HARDINFO2_OS@"
#define PLATFORM		OS "-" ARCH
#define KERNEL			""
#define HOSTNAME		""
#define ARCH_@HARDINFO2_ARCH@

#define LIBDIR			"@CMAKE_INSTALL_LIBDIR@"
#define LIBPREFIX		"@CMAKE_INSTALL_FULL_LIBDIR@/hardinfo2"
#define PREFIX			"@CMAKE_INSTALL_DATAROOTDIR@/hardinfo2"

#cmakedefine HARDINFO2_DEBUG	@HARDINFO2_DEBUG@

#if defined(HARDINFO2_DEBUG) && (HARDINFO2_DEBUG==1)
  #define RELEASE 0
  #define DEBUG(msg,...) fprintf(stderr, "*** %s:%d (%s) *** " msg "\n", \
        	           __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
  #define RELEASE 1
  #define DEBUG(msg,...)
#endif	/* HARDINFO2_DEBUG */

#define ENABLE_BINRELOC 1
#define HAS_LINUX_WE 1
#define HAS_LIBSOUP 1

#cmakedefine01 HAS_LIBSENSORS

#endif	/* __CONFIG_H__ */
