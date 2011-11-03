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

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <map>
#include <sstream>
#include <VXItrd.h>

#include <vglue_ipc.h>

/*  voiceglue IPC routines  */

/*  Note:  a VXIthreadID is a long  */
#include <sys/select.h>
#include <errno.h>
static pthread_mutex_t voiceglue_threadmap_mutex;
static std::map<VXIthreadID, int> voiceglue_threadid_to_fd;
static std::map<VXIthreadID, int> voiceglue_threadid_to_callid;

/* 
 * Register a voiceglue IPC file descriptor and callid with this thread
 *
 * @param fd  The file descriptor to register
 * @param callid The callid used by the SATC server
 */
int voiceglue_registeripcfd (int fd, int callid)
{
    VXIthreadID thread_id = VXItrdThreadGetID();
    pthread_mutex_lock (&voiceglue_threadmap_mutex);
    voiceglue_threadid_to_fd[thread_id] = fd;
    voiceglue_threadid_to_callid[thread_id] = callid;
    pthread_mutex_unlock (&voiceglue_threadmap_mutex);
    return (0);
};

/* 
 * Unregister a voiceglue IPC file descriptor and callid from this thread
 *
 * @param fd  The file descriptor to unregister
 * @param callid The callid used by the SATC server
 */
int voiceglue_unregisteripcfd()
{
    VXIthreadID thread_id = VXItrdThreadGetID();
    voiceglue_sendipcmsg ("\n");
    pthread_mutex_lock (&voiceglue_threadmap_mutex);
    voiceglue_threadid_to_fd.erase(thread_id);
    voiceglue_threadid_to_callid.erase(thread_id);
    pthread_mutex_unlock (&voiceglue_threadmap_mutex);
    //  Do not close fds here, as with:
    //  int fd = voiceglue_threadid_to_fd[thread_id];;
    //  close (fd);
    //  This creates a race condition with new
    //  thread start-ups.
    //  Instead, allow empty message to signal shutdown.
    return (0);
};

/* 
 * Send a voiceglue IPC message
 *
 * @param msg  The null-terminated bytes to send exactly (must supply own \n)
 */
int voiceglue_sendipcmsg (const char *msg)
{
    int fd;
    int written;
    int r;
    VXIthreadID myThreadID = VXItrdThreadGetID();
    int len = strlen (msg);

    /*  Look up IPC fd  */
    pthread_mutex_lock (&voiceglue_threadmap_mutex);
    fd = voiceglue_threadid_to_fd[myThreadID];
    pthread_mutex_unlock (&voiceglue_threadmap_mutex);

    if (voiceglue_loglevel() >= LOG_DEBUG)
    {
	std::ostringstream debugmsg;
	std::string msgcontent (msg);
	msgcontent.erase (len-1, 1);
	debugmsg << "snd vg: "
		 << msgcontent;
	voiceglue_log ((char) LOG_DEBUG, debugmsg);
    };

    /*  Write the data  */
    written = 0;
    while (written < len)
    {
	r = write (fd, msg + written, len - written);
	if (r == -1)
	{
	    if (errno != EINTR)
	    {
		printf ("FATAL voiceglue error: thread %d failed writing to fd=%d, errno=%d\n", (int) myThreadID, fd, errno);
		return (-1);
	    };
	}
	else
	{
	    written += r;
	};
    };
    return (0);
};

/* 
 * Send a voiceglue IPC message
 *
 * @param msg  The string to send exactly (must supply own \n)
 */
int voiceglue_sendipcmsg (std::string &msg)
{
    return voiceglue_sendipcmsg (msg.c_str());
};

/* 
 * Send a voiceglue IPC message
 *
 * @param msg  The string to send exactly (must supply own \n)
 */
int voiceglue_sendipcmsg (std::ostringstream &msg)
{
    return voiceglue_sendipcmsg (msg.str().c_str());
};

/*!
** Receives an voiceglue IPC message
** @return the message with terminating newline removed
*/
std::string voiceglue_getipcmsg()
{
    int fd;
    int bytes_read = 0;
    int bufsize = 1024;
    int r;
    char buf[1024];
    std::string msg;
    VXIthreadID myThreadID = VXItrdThreadGetID();

    /*  Look up IPC fd  */
    pthread_mutex_lock (&voiceglue_threadmap_mutex);
    fd = voiceglue_threadid_to_fd[myThreadID];
    pthread_mutex_unlock (&voiceglue_threadmap_mutex);

    /*  Read the data  */
    while ((bytes_read == 0) || (buf[bytes_read-1] != '\n'))
    {
	/*  See if we have exhausted the buffer  */
	if (bytes_read == bufsize)
	{
	    /*  Store this buffer contents into msg, and start clean  */
	    msg.append (buf, bytes_read);
	    bytes_read = 0;
	};

	/*  Perform the read to fill up buf  */
	r = read (fd, buf + bytes_read, bufsize - bytes_read);
	if (r == -1)
	{
	    if (errno != EINTR)
	    {
		printf ("FATAL voiceglue error: thread %d failed reading from fd=%d, errno=%d\n", (int) myThreadID, fd, errno);
		return ("");
	    };
	}
	else
	{
	    bytes_read += r;
	};
    };
    msg.append (buf, bytes_read - 1);    //  Strip off terminating newline

    if (voiceglue_loglevel() >= LOG_DEBUG)
    {
	std::ostringstream debugmsg;
	debugmsg << "rcv vg: " << msg;
	voiceglue_log ((char) LOG_DEBUG, debugmsg);
    };

    return (msg);
};

/*!
** Converts an ASCII string into SATC-quoted equivalent
**
** @param input_bytes String to convert, null-terminated
** @return the result
*/
std::string voiceglue_escape_SATC_string (const char *input_bytes)
{
    size_t input_bytes_length = strlen (input_bytes);
    const char *input_ptr = input_bytes;
    const char *input_end = input_ptr + input_bytes_length;
    char hex_buf[3];
    std::ostringstream output_bytes;
    char c;

    output_bytes << '"';

    //  Invariant:
    //    input_ptr points to the next character to be converted
    //    input_end points to one past the last character to be converted
    //    output_bytes contains already-converted characters
    while (input_ptr < input_end)
    {
	c = *(input_ptr++);
	if (c == '\\')
	{
	    output_bytes << "\\\\";
	}
	else if (c == '\n')
	{
	    output_bytes << "\\n";
	}
	else if (c == '\'')
	{
	    output_bytes << "\\\'";
	}
	else if (c == '\"')
	{
	    output_bytes << "\\\"";
	}
	else if ((c < ' ') || (c > '~'))
	{
	    output_bytes << '\\';
	    output_bytes << 'x';
	    sprintf (hex_buf, "%02x", (unsigned char) c);
	    output_bytes << hex_buf;
	}
	else
	{
	    output_bytes << c;
	};
    };

    //  Final quote
    output_bytes << '\"';
    return output_bytes.str();
};

static int voiceglue_logfd;
static FILE *voiceglue_logfh;
static int voiceglue_loglevel_value = -1;
static pthread_mutex_t voiceglue_log_mutex;

/*!
** Converts an ASCII string into SATC-quoted equivalent
**
** @param input_bytes String to convert, null-terminated
** @return the result
*/
std::string voiceglue_escape_SATC_string (std::string &input_bytes)
{
    return voiceglue_escape_SATC_string (input_bytes.c_str());
};

/*!
** Converts an ASCII string into SATC-quoted equivalent
**
** @param input_bytes String to convert, null-terminated
** @return the result
*/
std::string voiceglue_escape_SATC_string (std::ostringstream &input_bytes)
{
    return voiceglue_escape_SATC_string (input_bytes.str().c_str());
};

/*!
** Open the voiceglue log channel, and initializes mutex for thread ipc.
** @param logfd The file descriptor representing the log channel
## @param loglevel The initial log level
** @return 0 on success
*/
int voiceglue_log_init (int logfd, int loglevel)
{
    voiceglue_logfd = logfd;
    voiceglue_loglevel_value = loglevel;
    voiceglue_logfh = fdopen(logfd, "w");
    if (! voiceglue_logfh)
    {
	fprintf(stderr, "ERROR: Cannot open FILE* for fd=%d: %s errno=%d\n",
		logfd, strerror(errno), errno);
	return -1;
    };
    if (pthread_mutex_init (&voiceglue_log_mutex, NULL))
    {
	fprintf(stderr, "ERROR: Cannot init log mutex: %s errno=%d\n",
		strerror(errno), errno);
	return -1;
    };
    if (pthread_mutex_init (&voiceglue_threadmap_mutex, NULL))
    {
	fprintf(stderr, "ERROR: Cannot init threadmap mutex: %s errno=%d\n",
		strerror(errno), errno);
	return -1;
    };
    voiceglue_log ((char) 5, "OpenVXI started feed to dynlog\n");
    return 0;
};

/*!
** Writes a message to the voiceglue log channel
** @param level The level of the message
** @param message The UTF-8 message text, must be null terminated
## @return 0 on success
*/
int voiceglue_log (char level, const char *message)
{
    int r, i, len;
    len = strlen (message);
    char message_cleaned [len+2];
    std::ostringstream prefix;
    int prefix_len;
    VXIthreadID thread_id = VXItrdThreadGetID();
    strcpy (message_cleaned, message);
    if (message_cleaned[len-1] != '\n')
    {
	message_cleaned[len] = '\n';
	message_cleaned[len+1] = '\0';
	++len;
    };
    for (i = 0; i < len-1; ++i)
    {
	if (message_cleaned[i] == '\n')
	{
	    message_cleaned[i] = ' ';
	};
    };
    //  See if the element has a defined callid
    pthread_mutex_lock (&voiceglue_threadmap_mutex);
    std::map<VXIthreadID, int>::iterator iter =
	voiceglue_threadid_to_callid.find (thread_id);
    if (iter != voiceglue_threadid_to_callid.end())
    {
	//  Has a callid
	int callid = voiceglue_threadid_to_callid[thread_id];
	prefix << "callid=[" << callid << "] ";
    };
    pthread_mutex_unlock (&voiceglue_threadmap_mutex);
    prefix_len = prefix.str().length();
    pthread_mutex_lock (&voiceglue_log_mutex);
    fwrite (&level, 1, 1, voiceglue_logfh);
    if (prefix_len != 0)
    {
	fwrite (prefix.str().c_str(), prefix_len, 1, voiceglue_logfh);
    };
    r = fwrite (message_cleaned, len, 1, voiceglue_logfh);
    if (r == 0)
    {
	pthread_mutex_unlock (&voiceglue_log_mutex);
	return 1;
    };
    fflush (voiceglue_logfh);
    pthread_mutex_unlock (&voiceglue_log_mutex);
    return 0;
};

/*!
** Writes a message to the voiceglue log channel
** @param level The level of the message
** @param message The UTF-8 message text, must be null terminated
## @return 0 on success
*/
int voiceglue_log (char level, std::string &message)
{
    return voiceglue_log (level, message.c_str());
};

/*!
** Writes a message to the voiceglue log channel
** @param level The level of the message
** @param message The UTF-8 message text, must be null terminated
## @return 0 on success
*/
int voiceglue_log (char level, std::ostringstream &message)
{
    return voiceglue_log (level, message.str().c_str());
};

int voiceglue_loglevel()
{
    return voiceglue_loglevel_value;
};
