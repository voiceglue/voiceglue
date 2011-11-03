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

#include <stdlib.h>
#include <string.h>

#include <vglue_ipc_c.h>
#include <vglue_ipc.h>

/*  voiceglue IPC in C routines  */

/* 
 * Register a voiceglue IPC file descriptor and callid with this thread
 *
 * @param fd  The file descriptor to register
 * @param callid The callid used by the SATC server
 */
int voiceglue_c_registeripcfd (int fd, int callid)
{
    return (voiceglue_registeripcfd (fd, callid));
};

/* 
 * Unregister a voiceglue IPC file descriptor and callid from this thread
 */
int voiceglue_c_unregisteripcfd()
{
    return (voiceglue_unregisteripcfd());
};

/* 
 * Send a voiceglue IPC message
 *
 * @param msg  The null-terminated bytes to send exactly (must supply own \n)
 */
int voiceglue_c_sendipcmsg (const char *msg)
{
    return (voiceglue_sendipcmsg (msg));
};

/* 
 * Receive a voiceglue IPC message
 *
 * Returns a malloc'd null-terminated message that must be
 * free'd by the caller
 */
char *voiceglue_c_getipcmsg ()
{
    char *result;
    int resultlen;
    std::string msg_received;
    msg_received = voiceglue_getipcmsg();
    resultlen = msg_received.size() + 1;
    result = (char *) malloc(resultlen);
    strncpy(result,msg_received.c_str(),resultlen);
    return result;
};

/*!
** Open the voiceglue log channel
** @param logfd The file descriptor representing the log channel
## @param loglevel The initial log level
** @return 0 on success
*/
int voiceglue_c_log_init (int logfd, int loglevel)
{
    return (voiceglue_log_init (logfd, loglevel));
};

/*!
** Writes a message to the voiceglue log channel
** @param level The level of the message
** @param message The UTF-8 message text, must be null terminated
## @return 0 on success
*/
int voiceglue_c_log (char level, const char *message)
{
    return (voiceglue_log (level, message));
};

int voiceglue_c_loglevel()
{
    return (voiceglue_loglevel());
};
