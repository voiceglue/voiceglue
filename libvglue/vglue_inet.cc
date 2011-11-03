//  Copyright 2010 Ampersand Inc., Doug Campbell
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
#include <vglue_inet.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include <VXIinet.h>

/*  voiceglue inet (HTTP fetch) routines  */

/*
 *  Support for urlencoding
 */
static int HexPairValue(const char * code) {
  int value = 0;
  const char * pch = code;
  for (;;) {
    int digit = *pch++;
    if (digit >= '0' && digit <= '9') {
      value += digit - '0';
    }
    else if (digit >= 'A' && digit <= 'F') {
      value += digit - 'A' + 10;
    }
    else if (digit >= 'a' && digit <= 'f') {
      value += digit - 'a' + 10;
    }
    else {
      return -1;
    }
    if (pch == code + 2)
      return value;
    value <<= 4;
  }
}

int UrlDecode(const char *source, char *dest)
{
  char * start = dest;

  while (*source) {
    switch (*source) {
    case '+':
      *(dest++) = ' ';
      break;
    case '%':
      if (source[1] && source[2]) {
        int value = HexPairValue(source + 1);
        if (value >= 0) {
          *(dest++) = value;
          source += 2;
        }
        else {
          *dest++ = '?';
        }
      }
      else {
        *dest++ = '?';
      }
      break;
    default:
      *dest++ = *source;
    }
    source++;
  }
  
  *dest = 0;
  return dest - start;
}  

int UrlEncode(const char *source, char *dest, unsigned max)  
{
  static const char *digits = "0123456789ABCDEF";
  unsigned char ch;
  unsigned len = 0;
  char *start = dest;

  while (len < max - 4 && *source)
  {
    ch = (unsigned char)*source;
    if (*source == ' ') {
      *dest++ = '+';
    }
    else if (isalnum(ch) || strchr("-_.!~*'()", ch)) {
      *dest++ = *source;
    }
    else {
      *dest++ = '%';
      *dest++ = digits[(ch >> 4) & 0x0F];
      *dest++ = digits[       ch & 0x0F];
    }  
    source++;
  }
  *dest = 0;
  return start - dest;
}

std::string
UrlDecodeString(const std::string & encoded) {
  const char * sz_encoded = encoded.c_str();
  size_t needed_length = encoded.length();
  for (const char * pch = sz_encoded; *pch; pch++) {
    if (*pch == '%')
      needed_length += 2;
  }
  needed_length += 10;
  char stackalloc[64];
  char * buf = needed_length > sizeof(stackalloc)/sizeof(*stackalloc) ?
    (char *)malloc(needed_length) : stackalloc;
  UrlDecode(encoded.c_str(), buf);
  std::string result(buf);
  if (buf != stackalloc) {
    free(buf);
  }
  return result;
}

std::string
UrlEncodeString(const std::string & decoded) {
  const char * sz_decoded = decoded.c_str();
  size_t needed_length = decoded.length() * 3 + 3;
  char stackalloc[64];
  char * buf = needed_length > sizeof(stackalloc)/sizeof(*stackalloc) ?
    (char *)malloc(needed_length) : stackalloc;
  UrlEncode(decoded.c_str(), buf, needed_length);
  std::string result(buf);
  if (buf != stackalloc) {
    free(buf);
  }
  return result;
}

/*!
**  Gets a filename of a downloaded URL
**  @param method IN: The HTTP method ("GET", "get", "POST", "post")
**  @param url IN: The URL to download
**  @param postdata_map IN: arguments to pass via HTTP
**  @param parsevxml IN: whether a VXML parse should be obtained
**  @param path OUT: local path to fetched content
**  @param content_type OUT: content-type of fetched content
**  @param parse_tree_addr OUT: VXML document's parse tree addr, or "" for none*/
void voiceglue_http_get (const VXIchar *method,
			 const VXIchar *url,
			 const VXIMap *postdata_map,
			 int parsevxml,
			 std::string &path,
    			 std::string &content_type,
			 std::string &parse_tree_addr)
{
  std::string method_utf8 = VXIchar_to_Std_String (method);
  std::string url_utf8 = VXIchar_to_Std_String (url);

  //  Convert postdata_map to URL string
  std::string postdata_utf8 = "\"\"";
  if (postdata_map != NULL)
  {
      std::ostringstream postdata_utf8_stream;
      int items_shown = 0;
      const VXIchar *key = NULL;
      const VXIValue *gvalue = NULL;
      VXIMapIterator *it =
	  VXIMapGetFirstProperty (postdata_map, &key, &gvalue);
      int ret = 0;
      while (ret == 0 && key && gvalue)
      {
	  if (items_shown++)
	  {
	      postdata_utf8_stream << "&";
	  };
	  postdata_utf8_stream
	      << UrlEncodeString(VXIchar_to_Std_String (key)) << "="
	      << UrlEncodeString(VXIValue_to_Std_String (gvalue));
	  ret = VXIMapGetNextProperty(it, &key, &gvalue);
      }
      VXIMapIteratorDestroy(&it);
      postdata_utf8 = postdata_utf8_stream.str();
      if (postdata_utf8.length() == 0)
      {
	  postdata_utf8 = "\"\"";
      };
  };

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "voiceglue_http_get called with method=" << method_utf8
		<< " url=" << url_utf8
		<< " postdata="
		<< postdata_utf8
		<< " (encoded from map: "
		<< VXIValue_to_Std_String((const VXIValue *)postdata_map)
		<< ") parsevxml=" << parsevxml;
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  std::ostringstream ipc_msg_out;
  ipc_msg_out
      << "HttpGet "
      << voiceglue_escape_SATC_string (method_utf8)
      << " "
      << voiceglue_escape_SATC_string (url_utf8)
      << " "
      << postdata_utf8
      << " "
      << parsevxml
      << "\n";
  voiceglue_sendipcmsg (ipc_msg_out);

  std::string ipc_msg_in;
  ipc_msg_in = voiceglue_getipcmsg();

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "voiceglue_http_get returns " << ipc_msg_in;
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  int space_pos = ipc_msg_in.find (" ");
  if ((space_pos > 0) && (space_pos < (ipc_msg_in.length() - 1)))
  {
      path = ipc_msg_in.substr(0, space_pos);
      ipc_msg_in = ipc_msg_in.substr(space_pos+1);
      space_pos = ipc_msg_in.find (" ");
      if ((space_pos > 0) && (space_pos < (ipc_msg_in.length() - 1)))
      {
	  content_type = UrlDecodeString(ipc_msg_in.substr(0, space_pos));
	  parse_tree_addr = ipc_msg_in.substr(space_pos+1);
      }
      else
      {
	  content_type = UrlDecodeString(ipc_msg_in);
	  parse_tree_addr = "";
      }
  }
  else
  {
      path = ipc_msg_in;
      content_type = "";
      parse_tree_addr = "";
  };

  return;
};
