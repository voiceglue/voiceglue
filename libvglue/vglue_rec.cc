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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <cerrno>
#include <vglue_ipc.h>
#include <vglue_rec.h>
#include <vglue_tostring.h>
#include <string>
#include <sstream>

#include <VXIrec.h>

/*  voiceglue rec (recognition support) routines  */

/*!
**  Loads a grammar into voiceglue
**  @param gram_str The grammar to load
**  @param gram_type The type of the grammar
**  @param properties The property map
**  @param gram_id The id of the grammar
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_load_grammar (const VXIchar *gram_str,
				     const VXIchar *gram_type,
				     const VXIMap *props,
				     const char *gram_id)
{
    std::string grammar = VXIchar_to_Std_String (gram_str);
    std::string type = VXIchar_to_Std_String (gram_type);

    //  Sanity-check grammar
    if (grammar.length() == 0)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    voiceglue_log ((char) LOG_ERR, "empty grammar");
	};
	return VXIrec_RESULT_SYNTAX_ERROR;
    };

    //  Create property string parameter
    std::ostringstream prop_parameter;
    prop_parameter << "inputmodes="
		   << VXIMap_Property_to_Std_String (props, "inputmodes")
		   << " "
		   << "bargein="
		   << VXIMap_Property_to_Std_String (props, "bargein")
		   << " "
		   << "fetchaudiodelay="
		   << VXIMap_Property_to_Std_String (props, "fetchaudiodelay")
		   << " "
		   << "fetchtimeout="
		   << VXIMap_Property_to_Std_String (props, "fetchtimeout")
		   << " "
		   << "termchar="
		   << VXIMap_Property_to_Std_String (props, "termchar")
		   << " "
		   << "termtimeout="
		   << VXIMap_Property_to_Std_String (props, "termtimeout")
		   << " "
		   << "interdigittimeout="
		   << VXIMap_Property_to_Std_String (props,
						     "interdigittimeout");

    //  Send parse message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "Grammar "
	   << gram_id << " "
	   << voiceglue_escape_SATC_string (type)
	   << " "
	   << voiceglue_escape_SATC_string (grammar) << " "
	   << voiceglue_escape_SATC_string (prop_parameter.str().c_str())
	   << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get parse response
    std::string ipcmsg_result = voiceglue_getipcmsg();
    if ((ipcmsg_result.length() < 9) ||
	ipcmsg_result.substr(0, 9).compare("Grammar 0") != 0)
    {
	return VXIrec_RESULT_FAILURE;
    };
    
    return VXIrec_RESULT_SUCCESS;
};

/*!
**  Activates a grammar in voiceglue
**  @param props The property map
**  @param gram_id The id of the grammar
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_activate_grammar (const VXIMap *props,
					 const char *gram_id)
{
    //  Send activate message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "ActivateGrammar "
	   << gram_id << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get parse response
    std::string ipcmsg_result = voiceglue_getipcmsg();
    if ((ipcmsg_result.length() < 1) ||
	ipcmsg_result.substr(0, 1).compare("0") != 0)
    {
	return VXIrec_RESULT_FAILURE;
    };

    return VXIrec_RESULT_SUCCESS;
};

/*!
**  Deactivates a grammar in voiceglue
**  @param gram_id The id of the grammar
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_deactivate_grammar (const char *gram_id)
{
    //  Send deactivate message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "DeactivateGrammar "
	   << gram_id << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get parse response
    std::string ipcmsg_result = voiceglue_getipcmsg();
    if ((ipcmsg_result.length() < 1) ||
	ipcmsg_result.substr(0, 1).compare("0") != 0)
    {
	return VXIrec_RESULT_FAILURE;
    };

    return VXIrec_RESULT_SUCCESS;
};

/*!
**  Frees a grammar in voiceglue
**  @param gram_id The id of the grammar
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_free_grammar (const char *gram_id)
{
    //  Send deactivate message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "FreeGrammar "
	   << gram_id << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get parse response
    std::string ipcmsg_result = voiceglue_getipcmsg();
    if ((ipcmsg_result.length() < 1) ||
	ipcmsg_result.substr(0, 1).compare("0") != 0)
    {
	return VXIrec_RESULT_FAILURE;
    };

    return VXIrec_RESULT_SUCCESS;
};


/*!
**  Performs a recognition
**  @param props The property map
**  @param nlsml_result Gets filled with the NLSML recognition result
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_recognize (const VXIMap *props,
				  vxistring &nlsml_result)
{
    //  Create property string parameter
    std::ostringstream prop_parameter;
    prop_parameter << "inputmodes="
		   << VXIMap_Property_to_Std_String (props, "inputmodes")
		   << " "
		   << "bargein="
		   << VXIMap_Property_to_Std_String (props, "bargein")
		   << " "
		   << "timeout="
		   << VXIMap_Property_to_Std_String (props, "timeout")
		   << " "
		   << "fetchaudiodelay="
		   << VXIMap_Property_to_Std_String (props, "fetchaudiodelay")
		   << " "
		   << "fetchtimeout="
		   << VXIMap_Property_to_Std_String (props, "fetchtimeout")
		   << " "
		   << "termchar="
		   << VXIMap_Property_to_Std_String (props, "termchar")
		   << " "
		   << "termtimeout="
		   << VXIMap_Property_to_Std_String (props, "termtimeout")
		   << " "
		   << "interdigittimeout="
		   << VXIMap_Property_to_Std_String (props,
						     "interdigittimeout");

    //  Send recognize message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "Recognize "
	   << voiceglue_escape_SATC_string (prop_parameter.str().c_str())
	   << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get recognize response
    std::string ipcmsg_result = voiceglue_getipcmsg();

    //  Parse out message type
    if ((ipcmsg_result.length() < 11) ||
	(ipcmsg_result.substr(0,11).compare("Recognized ") != 0))
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Recognize: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    ipcmsg_result.erase (0, 11);

    //  Parse out integer result code
    std::string::size_type ws_index = ipcmsg_result.find (' ', 0);
    if (ws_index == std::string::npos)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Recognize, "
		   << "no space char found to delimit return code: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    int ret_code = atoi (ipcmsg_result.c_str());
    VXIrecResult vxi_ret_code = (VXIrecResult) ret_code;
    ipcmsg_result.erase (0, ws_index + 1);

    //  Send NLSML back
    nlsml_result = Std_String_to_vxistring (ipcmsg_result);
    return vxi_ret_code;
};

/*!
**  Performs a record.
**  @param props The property map
**  @param waveform Gets filled with pointer to malloc'd buffer of waveform
**  @param waveform_len Gets filled with number of bytes in waveform
**  @param duration Gets filled with duration of recording in ms
**  @param recorded_maxtime Gets filled with whether maxtime was hit (1 or 0)
**  @param terminator_digit The digit that terminated the recording, or 0
**  @return VXIrec_RESULT_SUCCESS on success, an error code on failure.
*/
VXIrecResult voiceglue_record (const VXIMap *props,
			       unsigned char **waveform,
			       unsigned int *waveform_len,
			       unsigned int *duration,
			       int *recorded_maxtime,
			       VXIbyte *terminator_digit)
{
    //  Initialize return for error condition,
    //  will replace later on success.
    *waveform = NULL;
    *waveform_len = 0;

    //  Create property string parameter
    std::ostringstream prop_parameter;
    prop_parameter
	<< "beep="
	<< VXIMap_Property_to_Std_String (props, "vxi.rec.beep")
	<< " "
	<< "rectype="
	<< VXIMap_Property_to_Std_String (props, "@rectype")
	<< " "
	<< "maxtime="
	<< VXIMap_Property_to_Std_String (props, "@maxtime")
	<< " "
	<< "finalsilence="
	<< VXIMap_Property_to_Std_String (props, "@finalsilence")
	<< " "
	<< "dtmfterm="
	<< VXIMap_Property_to_Std_String (props, "@dtmfterm");

    //  Send record message to perl
    std::ostringstream ipcmsg;
    ipcmsg << "Record "
	   << voiceglue_escape_SATC_string (prop_parameter.str().c_str())
	   << "\n";
    voiceglue_sendipcmsg (ipcmsg);

    //  Get record response
    std::string ipcmsg_result = voiceglue_getipcmsg();

    //  Parse out message type
    if ((ipcmsg_result.length() < 9) ||
	(ipcmsg_result.substr(0,9).compare("Recorded ") != 0))
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Record: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    ipcmsg_result.erase (0, 9);

    //  Parse out integer result code
    std::string::size_type ws_index = ipcmsg_result.find (' ', 0);
    if (ws_index == std::string::npos)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Record, "
		   << "no space char found to delimit return code: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    int ret_code = atoi (ipcmsg_result.c_str());
    VXIrecResult vxi_ret_code = (VXIrecResult) ret_code;
    ipcmsg_result.erase (0, ws_index + 1);
    if (vxi_ret_code != 0)
    {
	return vxi_ret_code;
    };

    //  Parse out integer reason code
    ws_index = ipcmsg_result.find (' ', 0);
    if (ws_index == std::string::npos)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Record, "
		   << "no space char found to delimit reason code: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    int reason_code = atoi (ipcmsg_result.c_str());
    if (reason_code == 4)
    {
	*recorded_maxtime = 1;
    }
    else
    {
	*recorded_maxtime = 0;
    };
    ipcmsg_result.erase (0, ws_index + 1);
    if (reason_code == 5)
    {
	//  Case of no audio started (noinput)
	return vxi_ret_code;
    };

    //  Parse out integer duration code
    ws_index = ipcmsg_result.find (' ', 0);
    if (ws_index == std::string::npos)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Record, "
		   << "no space char found to delimit duration: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    *duration = atoi (ipcmsg_result.c_str());
    ipcmsg_result.erase (0, ws_index + 1);

    //  Parse out terminator digit
    ws_index = ipcmsg_result.find (' ', 0);
    if (ws_index == std::string::npos)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "invalid response to Record, "
		   << "no space char found to delimit digit: "
		   << ipcmsg_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    VXIbyte digit = *(ipcmsg_result.c_str());
    if (digit == '-') {digit = (VXIbyte) 0;};
    *terminator_digit = digit;
    ipcmsg_result.erase (0, ws_index + 1);
    //  Rest is the filename of the recorded waveform

    //  Read the file
    int rec_file_fd = open (ipcmsg_result.c_str(), O_RDONLY);
    if (rec_file_fd == -1)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "Recorded returned unopenable file \""
		   << ipcmsg_result
		   << "\", open errno="
		   << errno;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    struct stat stat_buf;
    int stat_result;
    stat_result = fstat (rec_file_fd, &stat_buf);
    if (stat_result != 0)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream errmsg;
	    errmsg << "Recorded returned inaccessible file \""
		   << ipcmsg_result
		   << "\", stat return code="
		   << stat_result;
	    voiceglue_log ((char) LOG_ERR, errmsg);
	};
	return VXIrec_RESULT_PLATFORM_ERROR;
    };
    unsigned int rec_file_size = (unsigned int) stat_buf.st_size;
    *waveform_len = rec_file_size;
    if (voiceglue_loglevel() >= LOG_DEBUG)
    {
	std::ostringstream errmsg;
	errmsg << "Recorded file \""
	       << ipcmsg_result
	       << "\" of "
	       << rec_file_size
	       << " bytes, duration = "
	       << *duration
	       << "ms, maxtime = "
	       << *recorded_maxtime;
	voiceglue_log ((char) LOG_DEBUG, errmsg);
    };
    if (rec_file_size == 0)
    {
	return vxi_ret_code;
    };
    *waveform = (unsigned char *) malloc (rec_file_size);
    unsigned int bytes_read = 0;
    ssize_t read_result;
    while (bytes_read < rec_file_size)
    {
	read_result = read (rec_file_fd, *waveform + bytes_read,
			    rec_file_size - bytes_read);
	if (read_result <= 0)
	{
	    if (voiceglue_loglevel() >= LOG_ERR)
	    {
		std::ostringstream errmsg;
		errmsg << "Recorded file \""
		       << ipcmsg_result
		       << "\", cannot be read, read return code="
		       << read_result
		       << " errno="
		       << errno;
		voiceglue_log ((char) LOG_ERR, errmsg);
	    };
	    free (*waveform);
	    *waveform = NULL;
	    *waveform_len = 0;
	    return VXIrec_RESULT_PLATFORM_ERROR;
	};
	bytes_read += read_result;
    };

    return vxi_ret_code;
};

