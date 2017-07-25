#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERSION			"@HARDINFO_VERSION@"

#define ARCH			"ARCH_@HARDINFO_ARCH@"
#define OS			"@HARDINFO_OS@"
#define PLATFORM		OS "-" ARCH
#define KERNEL			""
#define HOSTNAME		""
#define ARCH_@HARDINFO_ARCH@

#define LIBDIR			"@CMAKE_INSTALL_LIBDIR@"
#define LIBPREFIX		"@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_LIBDIR@/hardinfo"
#define PREFIX			"@CMAKE_INSTALL_PREFIX@/share/hardinfo"

#cmakedefine LIBSOUP_FOUND
#cmakedefine HARDINFO_DEBUG	@HARDINFO_DEBUG@

#ifdef LIBSOUP_FOUND
#	define HAS_LIBSOUP
#endif	/* LIBSOUP_FOUND */

#if defined(HARDINFO_DEBUG) && (HARDINFO_DEBUG==1)
#	define RELEASE 0
#	define DEBUG(msg,...) fprintf(stderr, "*** %s:%d (%s) *** " msg "\n", \
        	           __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#	define RELEASE 1
#	define DEBUG(msg,...)
#endif	/* HARDINFO_DEBUG */

#define ENABLE_BINRELOC 1
#define HAS_LINUX_WE 1

#endif	/* __CONFIG_H__ */
