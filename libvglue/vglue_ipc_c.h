//  Copyright 2006,2007 Ampersand Inc., Doug Campbell
//
//  This file is part of libvglue.
//
//  libvglue is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  libvglue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with libvglue; if not, see <http://www.gnu.org/licenses/>.

#ifndef VGLUE_IPC_C_H
#define VGLUE_IPC_C_H

#ifdef __cplusplus
extern "C" {
#endif

int voiceglue_c_registeripcfd (int fd, int callid);
int voiceglue_c_unregisteripcfd();
int voiceglue_c_sendipcmsg (const char *msg);
char *voiceglue_c_getipcmsg();
int voiceglue_c_log_init (int logfd, int loglevel);
int voiceglue_c_log (char level, const char *message);
int voiceglue_c_loglevel();

#ifdef __cplusplus
}
#endif

#endif /* include guard VGLUE_IPC_C_H */
