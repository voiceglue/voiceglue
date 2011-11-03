//  Copyright 2006,2007 Doug Campbell
//
//  This file is part of Cam-Scom.
//
//  Cam-Scom is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  Cam-Scom is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Cam-Scom; if not, see <http://www.gnu.org/licenses/>.

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

MODULE = Cam::Scom  PACKAGE = Cam::Scom

void
_mask_to_array_in_c (maskvar, maskbitlenvar)
    SV *maskvar
    SV *maskbitlenvar
  INIT:
    AV *intlist;
    int i;
    int maskbitlen;
    char *mask;
  PPCODE:
    maskbitlen = SvIV(maskbitlenvar);
    mask = SvPV (maskvar, PL_na);
    intlist = newAV();
    for (i = 0; i < maskbitlen; ++i)
    {
	if (FD_ISSET (i, (fd_set *) mask))
	{
	    av_push (intlist, newSViv (i));
	};
    };
    XPUSHs (sv_2mortal (newRV_noinc ((SV*) intlist)));

MODULE = Cam::Scom  PACKAGE = Cam::Scom

void
_fcntl_3arg_in_c (fdarg, cmdarg, argarg)
    SV *fdarg
    SV *cmdarg
    SV *argarg
  INIT:
    int fd;
    int cmd;
    long arg;
    int r;
  PPCODE:
    fd = SvIV(fdarg);
    cmd = SvIV(cmdarg);
    arg = (long) SvIV(argarg);
    r = fcntl (fd, cmd, arg);
    XPUSHs (sv_2mortal (newSViv (r)));

MODULE = Cam::Scom  PACKAGE = Cam::Scom

void
_c_socket (domainarg, typearg, protocolarg)
    SV *domainarg
    SV *typearg
    SV *protocolarg
  INIT:

  /*  Provides socket(2) system call to perl  */
  /*  Use as:  $fd = _c_socket (domain, type, protocol);  */

    int fd;
    int domain;
    int type;
    int protocol;
  PPCODE:
    domain = SvIV(domainarg);
    type = SvIV(typearg);
    protocol = SvIV(protocolarg);
    fd = socket (domain, type, protocol);
    XPUSHs (sv_2mortal (newSViv (fd)));

void
_c_socketpair (domainarg, typearg, protocolarg)
    SV *domainarg
    SV *typearg
    SV *protocolarg
  INIT:

  /*  Provides socketpair(2) system call to perl  */
  /*  Use as:  ($r, $fd0, $fd1) = _c_socketpair (domain, type, protocol);  */

    int r;
    int fds[2];
    int domain;
    int type;
    int protocol;
  PPCODE:
    domain = SvIV(domainarg);
    type = SvIV(typearg);
    protocol = SvIV(protocolarg);
    r = socketpair (domain, type, protocol, fds);
    XPUSHs (sv_2mortal (newSViv (r)));
    XPUSHs (sv_2mortal (newSViv (fds[0])));
    XPUSHs (sv_2mortal (newSViv (fds[1])));

void
_c_getsockopt (fdarg, levelarg, optnamearg)
    SV *fdarg
    SV *levelarg
    SV *optnamearg
  INIT:

  /*  Provides getsockopt(2) system call to perl  */
  /*  Use as:  ($r [, $value]) = _c_getsockopt (fd, level, optname);  */

    int r;
    int fd;
    int level;
    int optname;
    char optval[256];
    socklen_t optvalbuflen;
  PPCODE:
    fd = SvIV(fdarg);
    level = SvIV(levelarg);
    optname = SvIV(optnamearg);
    optvalbuflen = 256;
    r = getsockopt (fd, level, optname,
    			(void *) optval, &optvalbuflen);
    XPUSHs (sv_2mortal (newSViv (r)));
    if (r != -1)
    {
	XPUSHs (sv_2mortal (newSVpv (optval, optvalbuflen)));
    };

void
_c_setsockopt (fdarg, levelarg, optnamearg, optvalarg)
    SV *fdarg
    SV *levelarg
    SV *optnamearg
    SV *optvalarg
  INIT:

  /*  Provides setsockopt(2) system call to perl  */
  /*  Use as:  $r = _c_setsockopt (fd, level, optname);  */

    int r;
    int fd;
    int level;
    int optname;
    const void *optval;
    STRLEN optvallen;
  PPCODE:
    fd = SvIV(fdarg);
    level = SvIV(levelarg);
    optname = SvIV(optnamearg);
    optval = SvPV(optvalarg, optvallen);
    r = setsockopt (fd, level, optname, optval, (socklen_t) optvallen);
    XPUSHs (sv_2mortal (newSViv (r)));

void
_c_bind (fdarg, addrarg)
    SV *fdarg
    SV *addrarg
  INIT:

  /*  Provides bind(2) system call to perl  */
  /*  Use as:  $r = _c_bind (fd, addr);  */

    int r;
    int fd;
    const struct sockaddr *addr;
    STRLEN addrlen;
  PPCODE:
    fd = SvIV(fdarg);
    addr = (const struct sockaddr *) SvPV(addrarg, addrlen);
    r = bind (fd, addr, (socklen_t) addrlen);
    XPUSHs (sv_2mortal (newSViv (r)));

void
_c_listen (fdarg, backlogarg)
    SV *fdarg
    SV *backlogarg
  INIT:

  /*  Provides listen(2) system call to perl  */
  /*  Use as:  $r = _c_listen (fd, backlog);  */

    int r;
    int fd;
    int backlog;
  PPCODE:
    fd = SvIV(fdarg);
    backlog = SvIV(backlogarg);
    r = listen (fd, backlog);
    XPUSHs (sv_2mortal (newSViv (r)));

void
_c_accept (fdarg)
    SV *fdarg
  INIT:

  /*  Provides accept(2) system call to perl  */
  /*  Use as:  ($new_fd [, $addr]) = _c_accept ($fd);  */

    int fd;
    struct sockaddr addr;
    socklen_t addrlen;
    int new_fd;
  PPCODE:
    fd = SvIV(fdarg);
    addrlen = sizeof(addr);
    new_fd = accept (fd, &addr, &addrlen);
    XPUSHs (sv_2mortal (newSViv (fd)));
    if (fd != -1)
    {
        XPUSHs (sv_2mortal (newSVpv ((const char *) &addr, (STRLEN) addrlen)));
    }

void
_c_connect (fdarg, addrarg)
    SV *fdarg
    SV *addrarg
  INIT:

  /*  Provides connect(2) system call to perl  */
  /*  Use as:  $r = _c_connect ($fd, $addr);  */

    int r;
    int fd;
    const struct sockaddr *addr;
    STRLEN addrlen;
  PPCODE:
    fd = SvIV(fdarg);
    addr = (const struct sockaddr *) SvPV(addrarg,addrlen);
    r = connect (fd, addr, (socklen_t) addrlen);
    XPUSHs (sv_2mortal (newSViv (r)));

