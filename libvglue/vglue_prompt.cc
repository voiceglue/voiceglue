//  Copyright 2006-2009 Ampersand Inc., Doug Campbell
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
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <vglue_ipc.h>
#include <vglue_tostring.h>
#include <vglue_prompt.h>
#include <vglue_tostring.h>
#include <string>
#include <sstream>

#include <VXIinet.h>

/*  voiceglue prompt (prompt support) routines  */

/*!
**  Queues a prompt
**  @param prompt_spec An SSML spec
**  @param properties The VXML properties
**  @return VXIprompt_RESULT_SUCCESS on success, other code on failure.
*/
VXIpromptResult voiceglue_prompt (const VXIchar *prompt_spec,
				  const VXIMap *properties)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIpromptQueue called with spec="
		<< VXIchar_to_Std_String (prompt_spec)
		<< ", properties="
		<< VXIValue_to_Std_String ((const VXIValue *) properties);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  //  Get the bargein param
  std::string bargein_param_utf8 =
      VXIchar_to_Std_String
      (VXIStringCStr
       (((const VXIString *) VXIMapGetProperty
	 (((const VXIMap *) properties), L"bargein"))));
  if (bargein_param_utf8.length() == 0)
  {
      bargein_param_utf8 = "true";
  };

  //  Determine if this is an in-memory audio sample
  const VXIValue *pcmdata_map =
      VXIMapGetProperty (properties, PROMPT_AUDIO_REFS);
  if (pcmdata_map != NULL)
  {
      //  This is an in-memory audio sample
      VXIvalueType maptype = VXIValueGetType (pcmdata_map);
      if (maptype != VALUE_MAP)
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream logstring;
	      logstring << "Invalid type for "
			<< "vxi.prompt.audioReferences proptery, expected "
			<< VALUE_MAP << " (map) but got "
			<< maptype;
	      voiceglue_log ((char) LOG_ERR, logstring);
	  };
	  return VXIprompt_RESULT_INVALID_ARGUMENT;
      };
      //  Get the key and value in the map
      const VXIchar *pcmdata_map_key = NULL;
      const VXIValue *pcmdata_map_value = NULL;
      VXIMapIterator *it =
	  VXIMapGetFirstProperty ((const VXIMap*) pcmdata_map,
				  &pcmdata_map_key,
				  &pcmdata_map_value);
      if ((! pcmdata_map_key) || (! pcmdata_map_value))
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream logstring;
	      logstring << "No contents of "
			<< "vxi.prompt.audioReferences proptery map";
	      voiceglue_log ((char) LOG_ERR, logstring);
	  };
	  return VXIprompt_RESULT_INVALID_ARGUMENT;
      };
      std::string pcmdata_name = VXIchar_to_Std_String (pcmdata_map_key);
      VXIvalueType contenttype = VXIValueGetType (pcmdata_map_value);
      if (contenttype != VALUE_CONTENT)
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream logstring;
	      logstring << "Invalid type for "
			<< "vxi.prompt.audioReferences map value, expected "
			<< VALUE_CONTENT << " (content) but got "
			<< contenttype;
	      voiceglue_log ((char) LOG_ERR, logstring);
	  };
	  return VXIprompt_RESULT_INVALID_ARGUMENT;
      };
      const VXIchar *content_type_string;
      VXIbyte *content_ptr;
      VXIulong content_size;
      VXIContentValue ((const VXIContent*) pcmdata_map_value,
		       &content_type_string,
		       (const VXIbyte **) &content_ptr,
		       &content_size);
      
      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "Found in-memory audio prompt named "
		    << pcmdata_name
		    << " of type "
		    << VXIchar_to_Std_String (content_type_string)
		    << " length "
		    << content_size
		    << " bytes";
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };

      //  Request a filepath from perl
      std::ostringstream ipc_msg;
      ipc_msg << "GetPCMPath "
	      << pcmdata_name << " "
	      << VXIchar_to_Std_String (content_type_string)
	      << "\n";
      voiceglue_sendipcmsg (ipc_msg);
      std::string ipcmsg_result = voiceglue_getipcmsg();
      if ((ipcmsg_result.length() < 9) ||
	  (ipcmsg_result.substr(0,8).compare("PCMPath ") != 0))
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream errmsg;
	      errmsg << "invalid response to GetPCMPath: "
		     << ipcmsg_result;
	      voiceglue_log ((char) LOG_ERR, errmsg);
	  };
	  return VXIprompt_RESULT_INVALID_ARGUMENT;
      };
      ipcmsg_result.erase (0, 8);
      //  Rest is the filepath to write

      //  Write the file
      unlink (ipcmsg_result.c_str());
      int rec_file_fd = open (ipcmsg_result.c_str(),
			      (O_WRONLY | O_CREAT), 0664);
      if (rec_file_fd == -1)
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream errmsg;
	      errmsg << "PCMPath returned unopenable filepath \""
		     << ipcmsg_result
		     << "\", open errno="
		     << errno;
	      voiceglue_log ((char) LOG_ERR, errmsg);
	  };
	  return VXIprompt_RESULT_INVALID_ARGUMENT;
      };
      unsigned int bytes_written = 0;
      ssize_t write_result;
      while (bytes_written < content_size)
      {
	  write_result = write (rec_file_fd,
				(char *) content_ptr + bytes_written,
				content_size - bytes_written);
	  if (write_result <= 0)
	  {
	      if (voiceglue_loglevel() >= LOG_ERR)
	      {
		  std::ostringstream errmsg;
		  errmsg << "PCM stream file \""
			 << ipcmsg_result
			 << "\", cannot be written, write return code="
			 << write_result
			 << " errno="
			 << errno;
		  voiceglue_log ((char) LOG_ERR, errmsg);
	      };
	      return VXIprompt_RESULT_INVALID_ARGUMENT;
	  };
	  bytes_written += write_result;
      };
      
      //  Queue the written file
      std::ostringstream ipc_msg2;
      ipc_msg2 << "PCMQueue "
	       << ipcmsg_result
	       << " "
	       << bargein_param_utf8
	       << "\n";
      voiceglue_sendipcmsg (ipc_msg2);
  }
  else
  {
      //  This is SSML (could contain TTS and/or <audio>)
      std::string prompt_spec_utf8 = VXIchar_to_Std_String (prompt_spec);
      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logmsg;
	  logmsg << "VXIpromptQueue (" << prompt_spec_utf8
		 << ")" << "\n";
	  voiceglue_log ((char) LOG_DEBUG, logmsg);
      };
      std::ostringstream ipc_msg;
      ipc_msg
	  << "Queue "
	  << voiceglue_escape_SATC_string (prompt_spec_utf8)
	  << " "
	  << bargein_param_utf8
	  << "\n";
      voiceglue_sendipcmsg (ipc_msg);
  };

  return VXIprompt_RESULT_SUCCESS;
};

