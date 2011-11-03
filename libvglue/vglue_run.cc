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

#include <syslog.h>
#include <sstream>
#include <cstdlib>

#include <VXItrd.h>
#include <VXIvximain.h>

#include <vglue_ipc.h>
#include <vglue_run.h>
#include <vglue_tostring.h>

/*  voiceglue run routines  */

/*!
**   Starts the VXML platform.
**   Into *platformHandle is written the address of
**   the malloc'd config args structure, used in vxiStartOneThread().
**   (It's really a VXIMap**).
**   @param num_channels The maximum number of simultaneous calls to handle
**   @param logfd The file descriptor to write dynlog log messages to
**   @param loglevel The starting value of the loglevel for dynlog
**   @param platformHandle Memory in which is returned a handle pointer
**   @return 0 on success, 1 on failure.
*/
int voiceglue_start_platform (int num_channels, int logfd, int loglevel,
			      void **platformHandle)
{
    VXIMap* config_args   = NULL;
    std::ostringstream config_file;
    const char* sbenv = getenv( "SWISBSDK" );
    int result;

    if (voiceglue_log_init (logfd, loglevel) != 0)
    {
	//  Can't even get logging going, so bail out
	return (1);
    };

    /* Parse the configuration file */
    if (! sbenv)
    {
	sbenv = "/var/lib/openvxi-3.4";
	setenv ("SWISBSDK", sbenv, 1);
    };
    config_file << sbenv << "/config/" << "SBclient.cfg";
    result =
	voiceglue_parse_config_file (&config_args, config_file.str().c_str());
    if (result != 0)
    {
	std::ostringstream errmsg;
	errmsg << "ParseConfigFile() returns " << result;
	voiceglue_log ((char) LOG_ERR, errmsg.str().c_str());
	return (1);
    };

    /*  Add the logfd and loglevel to the configArgs  */
    VXIMapSetProperty (config_args, L"logfd",
		       (VXIValue *) VXIIntegerCreate (logfd));
    VXIMapSetProperty (config_args, L"loglevel",
		       (VXIValue *) VXIIntegerCreate (loglevel));

    /* Initialize the platform */
    result =
	voiceglue_init_platform (config_args, (VXIunsigned *) &num_channels);
    if (result != 0)
    {
	std::ostringstream errmsg;
	errmsg << "VXIplatformInit() returns " << result;
	voiceglue_log ((char) LOG_ERR, errmsg.str().c_str());
	return (1);
    };

    /*  Write the return value  */
    *platformHandle = (void *) config_args;

    return (0);
};

/*!
 *  Frees the VXMLDocument whose address is represented by addr
 */
void voiceglue_free_parse_tree (const char *addr)
{
    vxistring wide_addr = Std_String_to_vxistring (addr);
    voiceglue_free_vxmldocument (wide_addr.c_str());
}

/*!
** Starts a VXML call processing thread.
** @param platformHandle Handle returned from vxiStartPlatform().
** @param callid Unique integer to refer to this call.
** @param ipcfd The file descriptor used for IPC with voiceglue.
** @param url The ASCII URL of the VXML start page.
** @param ani The ASCII ANI (caller-ID) of the incoming call.
** @param dnis The ASCII DNIS (DID) of the incoming call.
** @param javascript_init The call-specific session-level javascript init code
** @param channelHandle Memory in which is returned the channel thread handle.
** @return 0 on success, 1 on failure (on failure, check message log)
*/
int voiceglue_start_thread (void *platformHandle,
			    int callid, int ipcfd, char *url,
			    char *ani, char *dnis,
			    char *javascript_init,
			    void **channelHandle)
{
    return vxiStartOneThread (platformHandle, callid, ipcfd, url,
			      ani, dnis, javascript_init, channelHandle);
};

/*!
** Terminates a channel thread with the handle channelHandle
** from vxiStartOneThread() (really a ChannelThreadArgs *)
** @param channel_handle The handle returned from vxiStartOneThread().
** @return 0 on success, 1 on failure (on failure, check message log)
*/
int voiceglue_stop_thread (void *channel_handle)
{
    return vxiFinishThread (channel_handle);
};

/*!
** Stops the VXI platform.  The
** config args configArgs are from vxiStartPlatform().
** @param platform_ The handle returned from vxiStartPlatform
** @return 0 on success, 1 on failure (on failure, check message log)
*/
int voiceglue_stop_platform (void *platform_handle)
{
    return vxiStopPlatform (platform_handle);
};

/*!
**   Turns VXI platform diagnostic tracing on or off.
**   @param state true means to turn on, false means off.
*/
void voiceglue_set_trace (int trace_state)
{
    vxiSetTrace (trace_state);
};
