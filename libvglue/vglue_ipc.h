//                 -*-C++-*-

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

#ifndef VGLUE_IPC_H
#define VGLUE_IPC_H

#ifndef __cplusplus
#error "This is a C++ only header file"
#endif

#include <string>
#include <sstream>

#include <VXItrd.h>

int voiceglue_registeripcfd (int fd, int callid);
int voiceglue_unregisteripcfd();
int voiceglue_sendipcmsg (const char *msg);
int voiceglue_sendipcmsg (std::string &msg);
int voiceglue_sendipcmsg (std::ostringstream &msg);
std::string voiceglue_getipcmsg();
std::string voiceglue_escape_SATC_string (const char *input_bytes);
std::string voiceglue_escape_SATC_string (std::string &input_bytes);
std::string voiceglue_escape_SATC_string (std::ostringstream &input_bytes);
int voiceglue_log_init (int logfd, int loglevel);
int voiceglue_log (char level, const char *message);
int voiceglue_log (char level, std::string &message);
int voiceglue_log (char level, std::ostringstream &message);
int voiceglue_loglevel();

#endif /* include guard VGLUE_IPC_H */
