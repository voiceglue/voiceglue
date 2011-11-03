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
#include <sys/types.h>
#include <sys/stat.h>
#include <vglue_ipc.h>
#include <vglue_tel.h>
#include <vglue_tostring.h>
#include <string>
#include <sstream>

/*  voiceglue tel (telephony support) routines  */

/*!
**  Gets current telephone line status
**  @return VXItel_STATUS_ACTIVE (connected) or VXItel_STATUS_INACTIVE (hungup)
*/
VXItelStatus voiceglue_get_line_status ()
{
    //  Send request to perl
    voiceglue_sendipcmsg ("GetLineStatus\n");

    //  Get response
    std::string ipcmsg_result = voiceglue_getipcmsg();
    if ((ipcmsg_result.length() < 12) ||
	ipcmsg_result.substr(0, 11).compare("LineStatus ") != 0)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to GetLineStatus: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXItel_STATUS_INACTIVE;
    };
    if (ipcmsg_result.substr(11, 1).compare("1") == 0)
    {
	if (voiceglue_loglevel() >= LOG_DEBUG)
	{
	    std::ostringstream errmsg;
	    errmsg << "LineStatus is CONNECTED";
	    voiceglue_log ((char) LOG_DEBUG, errmsg);
	};
	return VXItel_STATUS_ACTIVE;
    };
    if (ipcmsg_result.substr(11, 1).compare("0") == 0)
    {
	if (voiceglue_loglevel() >= LOG_DEBUG)
	{
	    std::ostringstream errmsg;
	    errmsg << "LineStatus is DISCONNECTED";
	    voiceglue_log ((char) LOG_DEBUG, errmsg);
	};
	return VXItel_STATUS_INACTIVE;
    };
    if (voiceglue_loglevel() >= LOG_ERR)
    {
	std::ostringstream errmsg;
	errmsg << "invalid parameter from LineStatus msg: "
	       << ipcmsg_result;
	voiceglue_log ((char) LOG_ERR, errmsg);
    };
    return VXItel_STATUS_INACTIVE;
};

/*
 *  Peforms a transfer
 *  @param dest The destination
 *  @param from The "From" (ANI/callerID)
 *  @param type One of 0 = blind, 1 = bridged
 *  @param timeout Timeout
 *  @param result Out result string, one of "ANSWER", "NOANSWER", "BUSY", etc.
 */
VXItelResult voiceglue_transfer (std::string dest,
				 std::string from,
				 int type,
				 std::string timeout,
				 std::string &result)
{
    std::ostringstream ipc_msg;
    std::string type_string;

    //  Decode transfer type
    if (type == 0)
    {
	type_string = "blind";
    }
    else if (type == 1)
    {
	type_string = "bridged";
    }
    else
    {
	return VXItel_RESULT_INVALID_ARGUMENT;
    };

    //  Send request to voiceglue
    ipc_msg << "Transfer "
	    << voiceglue_escape_SATC_string (dest.c_str())
	    << " "
	    << voiceglue_escape_SATC_string (from.c_str())
	    << " "
	    << voiceglue_escape_SATC_string (type_string)
	    << " "
	    << voiceglue_escape_SATC_string (timeout.c_str())
	    << "\n";
    voiceglue_sendipcmsg (ipc_msg.str().c_str());

    //  Wait for it to finish
    std::string ipcmsg_result = voiceglue_getipcmsg();
    
    //  Decode result
    std::vector<std::string> result_words = split(ipcmsg_result);
    if (result_words.size() != 3)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Transfer: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXItel_RESULT_UNSUPPORTED;
    };
    result = result_words[2];
    return VXItel_RESULT_SUCCESS;
}
