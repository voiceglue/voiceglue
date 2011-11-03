
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
 * registered trademarks of Vocalocity, Inc. 
 * OpenVXI is a trademark of Scansoft, Inc. and used under license 
 * by Vocalocity.
 ***********************************************************************/

#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

class SBinetNString;

class SBinetHttpUtils
{
 public:
  enum HttpCode
  {
    HTTP_CONTINUE = 100,                    /* Continue an operation */
    HTTP_UPGRADE = 101,                     /* Switching protocols */

    HTTP_OK = 200,                          /* Everything's OK */
    HTTP_CREATED = 201,                     /* New object is created */
    HTTP_ACCEPTED = 202,                    /* Accepted */
    HTTP_NO_DATA = 204,                     /* OK but no data was loaded */
    HTTP_RESET_CONTENT = 205,               /* Reset content */
    HTTP_PARTIAL_CONTENT = 206,             /* Partial Content */

    HTTP_MULTIPLE_CHOICES = 300,            /* Multiple choices */
    HTTP_PERM_REDIRECT = 301,               /* Permanent redirection */
    HTTP_FOUND = 302,                       /* Found */
    HTTP_SEE_OTHER = 303,                   /* See other */
    HTTP_NOT_MODIFIED = 304,                /* Not Modified */
    HTTP_USE_PROXY = 305,                   /* Use Proxy */
    HTTP_PROXY_REDIRECT = 306,              /* Proxy Redirect */
    HTTP_TEMP_REDIRECT = 307,               /* Temporary redirect */

    HTTP_NO_ACCESS = 401,                  /* Unauthorized */
    HTTP_FORBIDDEN = 403,                  /* Access forbidden */
    HTTP_NOT_FOUND = 404,                  /* Not found */
    HTTP_METHOD_NOT_ALLOWED = 405,         /* Method not allowed */
    HTTP_NOT_ACCEPTABLE = 406,             /* Not Acceptable */
    HTTP_NO_PROXY_ACCESS = 407,            /* Proxy Authentication Failed */
    HTTP_CONFLICT = 409,                   /* Conflict */
    HTTP_LENGTH_REQUIRED = 411,            /* Length required */
    HTTP_PRECONDITION_FAILED = 412,        /* Precondition failed */
    HTTP_TOO_BIG = 413,                    /* Request entity too large */
    HTTP_URI_TOO_BIG = 414,                /* Request-URI too long */
    HTTP_UNSUPPORTED = 415,                /* Unsupported */
    HTTP_BAD_RANGE = 416,                  /* Request Range not satisfiable */
    HTTP_EXPECTATION_FAILED = 417,         /* Expectation Failed */
    HTTP_REAUTH = 418,                     /* Reauthentication required */
    HTTP_PROXY_REAUTH = 419,               /* Proxy Reauthentication required */

    HTTP_SERVER_ERROR = 500,               /* Internal server error */
    HTTP_NOT_IMPLEMENTED = 501,            /* Not implemented (server error) */
    HTTP_BAD_GATEWAY = 502,                /* Bad gateway (server error) */
    HTTP_RETRY = 503,                      /* Service not available (server error) */
    HTTP_GATEWAY_TIMEOUT = 504,            /* Gateway timeout (server error) */
    HTTP_BAD_VERSION = 505,                /* Bad protocol version (server error) */
    HTTP_NO_PARTIAL_UPDATE = 506,          /* Partial update not implemented (server error) */

    HTTP_INTERNAL = 900,                   /* Weird -- should never happen. */
    HTTP_WOULD_BLOCK = 901,                /* If we are in a select */
    HTTP_INTERRUPTED = 902,                /* Note the negative value! */
    HTTP_PAUSE = 903,                      /* If we want to pause a stream */
    HTTP_RECOVER_PIPE = 904,               /* Recover pipe line */
    HTTP_TIMEOUT = 905,                    /* Connection timeout */
    HTTP_NO_HOST = 906                     /* Can't locate host */
  };


  enum HttpUriEncoding
  {
    URL_XALPHAS = 0x1,                      /* Escape all unsafe characters */
    URL_XPALPHAS = 0x2,                     /* As URL_XALPHAS but allows '+' */
    URL_PATH = 0x4,                         /* As URL_XPALPHAS but allows '/' */
    URL_DOSFILE = 0x8                       /* As URL_PATH but allows ':' */
  };

  /**
   * Replace characters by their HEX encoding.
   **/
  static void escapeString(const char * str, HttpUriEncoding mask, SBinetNString& result);
  static void escapeData(const void *data, const int size,
				  HttpUriEncoding mask, SBinetNString& result);


  /**
   * Performs a UTF-8 encoding of the wide string.
   * If the string is invalid (contains character greater than 0x7FFFFFFF), an empty string is returned.
   **/
  static void utf8encode(const VXIchar *str, SBinetNString& result);


  // HTTP parsing utilities.  They all advance str to the first character that
  // does not compose the lexeme and return it.  The lexeme is returned by
  // reference.
  static const char *getToken(const char *str, SBinetNString& token);
  static const char *getCookieValue(const char *str, SBinetNString& value);
  static const char *getValue(const char *str, SBinetNString& value);

  // advances to the next non-whitespace character and returns it, However, if
  // the character found is not in delim, it returns NULL. Note that the '\0'
  // character is always returned as is (it is always considered to be part of
  // delim).
  static const char *expectChar(const char *start, const char *delim);

  // Avoid locale dependant ctype.h macros
  static inline bool isSpace(const char c) {
    return (((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) ?
	    true : false);
  }

  static inline bool isSpace(const wchar_t c) {
    return (((c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r')) ?
	    true : false);
  }

  static inline bool isAlpha(const wchar_t c) {
    return ((((c >= L'a') && (c <= L'z')) || ((c >= L'A') && (c <= L'Z'))) ?
	    true : false);
  }

  static inline bool isDigit(const wchar_t c) {
    return (((c >= L'0') && (c <= L'9')) ?  true : false);
  }

  static inline wchar_t toLower(const wchar_t c) {
    return ((( c >= L'A' ) && ( c <= L'Z' )) ? c + (L'a' - L'A') : c);
  }

  static inline wchar_t toUpper(const wchar_t c) {
    return ((( c >= L'a' ) && ( c <= L'z' )) ? c - (L'a' - L'A') : c);
  }

  // Sleep with a duration in microseconds
  static void usleep(unsigned long microsecs);

  // Case insensitive compares, can't use the C library versions as
  // they are locale dependant (and some C libraries don't even have
  // these as they are not ANSI/ISO C standard), and can't use
  // "wcscasecmp" and "wcsncasecmp" as the names otherwise it
  // conflicts with #defines for Microsoft Visual Studio C++
  static int casecmp(const wchar_t *s1, const wchar_t *s2);
  static int casecmp(const wchar_t *s1, const wchar_t *s2, register size_t n);

 private:
  // Disable constructor.
  SBinetHttpUtils();

  // Helper function used to parse a string that is known to be in quote.
  static const char *getQuotedString(const char *str, SBinetNString& value);
};

#endif
