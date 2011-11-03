//                    -*-C-*-

//  This file is part of Vxglue.
//  Copyright 2006,2007 Ampersand Inc., Doug Campbell
//
//  Vxglue is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  Vxglue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Vxglue; if not, see <http://www.gnu.org/licenses/>.

/*  This is C code, passed unchanged to the .c file  */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <vglue_run.h>

/*  This starts the xsubpp-translated code  */

MODULE = Vxglue   PACKAGE = Vxglue

void
_PerlFree (sv)
        SV *sv
INIT:
    void *ptr;
PPCODE:
    ptr = (void *) SvIV(SvRV(sv));
    if (ptr != NULL)
    {
	Safefree (ptr);
    };

MODULE = Vxglue   PACKAGE = Vxglue

void
_start_voiceglue_platform (int numChannels, int logfd, int loglevel)
INIT:
/*  ($ok, $msg, $platform_handle) =              */
/*     _start_voiceglue_platform ($numChannels, $fd, $loglevel); */
    int ok = 1;
    SV *msg = &PL_sv_undef;
    void *platformHandle;
    int result;
PPCODE:
    result = voiceglue_start_platform
        (numChannels, logfd, loglevel, &platformHandle);
    if (result != 0)
    {
	ok = 0;
	msg = sv_2mortal (newSVpv("voiceglue_start_platform failed, check log",
				  0));
    };

    EXTEND (SP, 3);
    XPUSHs (sv_2mortal (newSViv (ok)));
    XPUSHs (msg);
    XPUSHs (sv_setref_pv (newSViv(0), NULL,
			  (void *) platformHandle));


MODULE = Vxglue   PACKAGE = Vxglue

void
_start_voiceglue_thread (SV *platform_handle, int callid, int ipcfd, char *url, char *ani, char *dnis, char *javascript_init)
INIT:
/*  ($ok, $msg, $call_handle) =
     _start_voiceglue_thread ($platform_handle, $callid, $ipcfd, $url,
                              $ani, $dnis, $javascript_init);
 */
    int ok = 1;
    SV *msg = &PL_sv_undef;
    void *channel;
    int result;
    void *platform;
PPCODE:
    platform = (void *) SvIV(SvRV(platform_handle));

    result = voiceglue_start_thread
	(platform, callid, ipcfd, url, ani, dnis, javascript_init, &channel);
    if (result != 0)
    {
	ok = 0;
	msg = sv_2mortal (newSVpv("voiceglue_start_thread failed, check log",
				  0));
    };
    
    EXTEND (SP, 3);
    XPUSHs (sv_2mortal (newSViv (ok)));
    XPUSHs (msg);
    XPUSHs (sv_setref_pv (newSViv(0), NULL,
			  (void *) channel));


MODULE = Vxglue   PACKAGE = Vxglue

void
_stop_voiceglue_thread (SV *call_handle)
INIT:
/*  ($ok, $msg) =
     _stop_voiceglue_stop_thread (call_handle); */
    int ok = 1;
    SV *msg = &PL_sv_undef;
    void *channelArgs;
    int result;
PPCODE:
    channelArgs = (void *) SvIV(SvRV(call_handle));

    result = voiceglue_stop_thread (channelArgs);
    if (result != 0)
    {
	ok = 0;
	msg = sv_2mortal (newSVpv("voiceglue_stop_thread failed, check log",
				  0));
    };

    EXTEND (SP, 2);
    XPUSHs (sv_2mortal (newSViv (ok)));
    XPUSHs (msg);

MODULE = Vxglue   PACKAGE = Vxglue

void
_stop_voiceglue_platform (SV *platform_handle)
INIT:
    int ok = 1;
    SV *msg = &PL_sv_undef;
    void *platform;
    int result;
PPCODE:
    platform = (void *) SvIV(SvRV(platform_handle));

    result = voiceglue_stop_platform (platform);
    if (result != 0)
    {
	ok = 0;
	msg = sv_2mortal (newSVpv("voiceglue_stop_platform failed, check log",
				  0));
    };

    EXTEND (SP, 2);
    XPUSHs (sv_2mortal (newSViv (ok)));
    XPUSHs (msg);


MODULE = Vxglue   PACKAGE = Vxglue

void
_set_voiceglue_trace (int trace_level)
INIT:
PPCODE:
    voiceglue_set_trace (trace_level);


MODULE = Vxglue   PACKAGE = Vxglue

void
_voiceglue_vxmlparse_free (char *parse_tree_addr_string)
INIT:
PPCODE:
    voiceglue_free_parse_tree (parse_tree_addr_string);

