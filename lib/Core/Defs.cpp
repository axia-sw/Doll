#include "doll/Core/Defs.hpp"


/* Mac OS X */
#if defined(__APPLE__)
# include <mach-o/dyld.h>
# include <errno.h>
static char *getexename(char *buff, size_t n) {
	uint32_t size;

	size = n;

	if (_NSGetExecutablePath(buff, &size)==0) {
		errno = ERANGE;
		return (char *)0;
	}

	return buff;
}
/* Linux */
#elif defined(linux)||defined(__linux)
# include <errno.h>
# include <unistd.h>
# include <sys/types.h>
static char *getexename(char *buff, size_t n) {
	pid_t pid;
	char name[256];
	ssize_t r;

	pid = getpid();

	snprintf(name, sizeof(name), "/proc/%i/exe", (int)pid);

	if ((r = readlink(name, buff, n))==-1) {
		return (char *)0;
	} else if((size_t)r >= n) {
		errno = ERANGE;
		return (char *)0;
	}

	buff[r] = 0;
	return buff;
}
/* Solaris */
#elif (defined(sun)||defined(__sun))&&(defined(__SVR4)||defined(__svr4__))
# include <stdlib.h>
static char *getexename(char *buff, size_t n) {
	strncpy(buff, getexecname(), n-1);
	buff[n-1] = 0;

	return buff;
}
/* FreeBSD */
#elif defined(__FreeBSD__)
# include <sys/sysctl.h>
static char *getexename(char *buff, size_t n) {
	size_t size;
	int name[4];

	name[0] = CTL_KERN;
	name[1] = KERN_PROC;
	name[2] = KERN_PROC_PATHNAME;
	name[3] = -1;

	if (sysctl(name, sizeof(name)/sizeof(name[0]), buff, &size, 0, 0)==-1)
		return (char *)0;

	return buff;
}
/* UNIX */
#elif defined(unix)||defined(__unix__)
# include <stdio.h>
# include <stdlib.h>
static char *getexename(char *buff, size_t n) {
	char *s;

	if ((s=getenv("_"))!=(char *)0) {
		snprintf(buff, s, n-1);
		buff[n-1] = 0;

		return buff;
	}

	/* out of luck buddy, write a port */
	fprintf(stderr, "getexename: your unix distribution isn't supported.\n");
	fflush(stderr);

	return (char *)0;
}
/* Windows */
#elif defined(_WIN32)
# include <windows.h>
static char *getexename(char *buff, size_t n) {
	wchar_t wszBuf[ AXSTR_MAX_PATH ];
	if( !GetModuleFileNameW( NULL, wszBuf, sizeof( wszBuf )/sizeof( wszBuf[ 0 ] ) ) ) {
		return nullptr;
	}

	axstr_utf16_to_utf8( ( axstr_utf8_t * )buff, n, ( const axstr_utf16_t * )wszBuf );
	return buff;
}
/* wtf? */
#else
# error "No? Write a port."
#endif

namespace doll
{

	DOLL_FUNC Bool DOLL_API app_getPath( Str &dst )
	{
		static bool bDidInit = false;
		static char szBuf[ AXSTR_MAX_PATH ];

		if( !bDidInit ) {
			if( !getexename( szBuf, sizeof( szBuf ) ) ) {
				dst = Str();
				return false;
			}

			bDidInit = true;
		}

		dst = Str( szBuf );
		return true;
	}

}
