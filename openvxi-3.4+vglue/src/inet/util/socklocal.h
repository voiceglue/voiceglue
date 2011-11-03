#ifdef HAVE_BUILTIN_H
#  include <builtin.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else

#ifdef __osf__
typedef int socklen_t;
#endif

EXTERN_C_BEGIN
#ifdef HAVE_STRING_H
#  include <string.h>
#else
#  ifdef HAVE_MEMORY_H
#    include <memory.h>
#  endif
#endif

#ifdef _ALL_SOURCE
#  define _BSD 44  // AIX
#  include <sys/select.h>
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE
#  endif
#  undef _ALL_SOURCE
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#  include <sys/wait.h>
#endif

#ifndef _WIN32
#include <sys/signal.h>
#include <netinet/in.h>
#endif

#ifndef SA_RESTART
#  define SA_RESTART 0
#endif
typedef int SOCKET;
#define INVALID_SOCKET (-1)
EXTERN_C_END



#if defined (__sun__) && !defined (__svr4__) && defined (_S_LIBGXX)
// libg++-2.6.x has stopped providing prototypes for the following
// for sunos 4.1.x

extern "C" {
  int socketpair (int domain, int typ, int protocol, int* sockpair);
  int socket (int domain, int typ, int protocol);
  int bind   (SOCKET sock, void* addr, int addrlen);
  int connect (SOCKET sock, void* addr, int addrlen);
  int listen (SOCKET sock, int num);
  int accept (SOCKET sock, void* addr, int* addrlen);

  int recv (SOCKET sock, void* buf, int buflen, int msgflag);
  int recvfrom (SOCKET sock, void* buf, int buflen, int msgflag,
		void* addr, int* addrlen);
  int send (SOCKET sock, void* buf, int buflen, int msgflag);
  int sendto (SOCKET sock, void* buf, int buflen, int msgflag,
	      void* addr, int addrlen);

  int recvmsg (SOCKET sock, struct msghdr* msg, int msgflag);
  int sendmsg (SOCKET sock, struct msghdr* msg, int msgflag);

  int select (SOCKET sock, void* rd, void* wr, void* ex, struct timeval* tv);

  int getsockopt (SOCKET sock, int level, int option, void* val, int* vallen);
  int setsockopt (SOCKET sock, int level, int option, void* val, int vallen);

  int getsockname (SOCKET sock, void* addr, int* addrlen);
  int getpeername (SOCKET sock, void* addr, int* addrlen);
}
#endif

//extern "C" int shutdown (int, int); // they have forgotten this

// <arpa/inet.h> does not have a prototype for inet_addr () and gethostname()
extern "C" unsigned long inet_addr (const char*);

// arpa/in.h does not provide a protype for the following
extern "C" char* inet_ntoa (in_addr ina);

#if !defined (__linux__) && !defined(_solaris_)
  extern "C" int gethostname (char* hostname, int len);
  extern char* SYS_SIGLIST [];
#endif

//typedef RETSIGTYPE (*funcptr) (...);

#endif /* !_WIN32 */
