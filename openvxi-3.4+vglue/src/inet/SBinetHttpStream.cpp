
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

#ifndef _SB_USE_STD_NAMESPACE
#define _SB_USE_STD_NAMESPACE
#endif

#ifdef WIN32

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sys/timeb.h>                  // for _ftime( )/ftime( )
#include <stdio.h>                      // for sprintf

#include "VXIvalue.h"
#include "VXIinet.h"
#include "VXItrd.h"

#include "SBinetLog.h"
#include "SBinetURL.h"
#include "SBinetHttpStream.hpp"
#include "SBinetChannel.h"
#include "SBinetHttpConnection.hpp"
#include "SBinetValidator.h"
#include "SBinetCookie.h"
#include "SBinetUtils.hpp"
#include "SWITimeWatch.hpp"

#include "util_date.h"
#include "ap_ctype.h"
#include "HttpUtils.hpp"
#include <SWIsocket.hpp>
#include <SWIdataOutputStream.hpp>
#include <SWIbufferedInputStream.hpp>
#include <SWIbufferedOutputStream.hpp>

#include <list>
typedef std::basic_string<VXIchar> inetstring;

#define HTTP_UNDEFINED -1234  /* No HTTP error code yet */

#define READ_CHUNK_SIZE 128 * 1024 // Read from socket in 128 Kb chunks

SBinetHttpStream::SBinetHttpStream(SBinetURL* url,
                                   SubmitMethod method,
                                   SBinetChannel* ch,
                                   SBinetValidator* cacheValidator,
                                   VXIlogInterface *log,
                                   VXIunsigned diagLogBase):
  SBinetStoppableStream(url, SBinetStream_HTTP, log, diagLogBase),
  _HttpStatus(HTTP_UNDEFINED), _leftToRead(~0), _chunked(false),
  _method(method), _channel(ch), _connection(NULL),
  _inputStream(NULL), _outputStream(NULL), _closeConnection(TRUE),
  _validator(NULL), _cacheValidator(cacheValidator), _connectionAborted(FALSE)
{}

SBinetHttpStream::~SBinetHttpStream()
{
  delete _validator;
  Close();
}

VXIinetResult SBinetHttpStream::initSocket(SubmitMethod method,
                                           const VXIMap *properties,
                                           VXIMap *streamInfo)
{
  _connection = _channel->getHttpConnection(_url, properties);

  if (_connection == NULL)
  {
    Error(262, L"%s%s", L"URL", _url->getAbsolute());
    return VXIinet_RESULT_FETCH_ERROR;
  }

  SWITimeWatch timeWatch;
  timeWatch.start();

  switch (_connection->connect(getDelay()))
  {
   case SWIstream::TIMED_OUT:
     Error(241, NULL);
     return VXIinet_RESULT_FETCH_TIMEOUT;
     break;
   case SWIstream::SUCCESS:
     break;
   default:
     Error(244, L"%s%s", L"URL", _url->getAbsolute());
     return VXIinet_RESULT_FETCH_ERROR;
  }

  _inputStream = _connection->getInputStream();
  _outputStream = _connection->getOutputStream();

  Diag(MODULE_SBINET_TIMING_TAGID, L"SBinetHttpStream::initSocket",
       L"%i msecs, Connect to socket", timeWatch.getElapsed());

  writeDebugTimeStamp();

  if (method == SBinetHttpStream::GET_METHOD)
    _channel->writeString(_outputStream, "GET ");
  else
    _channel->writeString(_outputStream, "POST ");

  if (_connection->usesProxy())
    _channel->writeString(_outputStream, _url->getNAbsolute());
  else
    _channel->writeString(_outputStream, _url->getNPath());

  _channel->writeString(_outputStream, " HTTP/1.1" CRLF);
  _channel->writeString(_outputStream, "Accept: */*" CRLF);

  // Write HOST header.
  _channel->writeString(_outputStream, "Host: ");
  _channel->writeString(_outputStream, _url->getNHost());
  int defaultPort = _url->getProtocol() == SBinetURL::HTTP_PROTOCOL ? 80 : 443;
  int port = _url->getPort();

  if (port != defaultPort)
  {
    _channel->writeString(_outputStream, ":");
    _channel->writeInt(_outputStream, port);
  }
  _channel->writeString(_outputStream, CRLF);

  // Write USER-AGENT header.
  _channel->writeString(_outputStream, "User-Agent: ");
  _channel->writeString(_outputStream, _channel->getUserAgent());
  _channel->writeString(_outputStream, CRLF);

  // Write REFERER header.
  SBinetNString baseUrl = _url->getNBase();
  if (baseUrl.length() > 0)
  {
    // Cut query arguments, if present
    VXIunsigned queryArgsSeparator = baseUrl.find('?');
    if (queryArgsSeparator < baseUrl.length())
      baseUrl.resize(queryArgsSeparator);

    if (baseUrl.length() > 0)
    {
      _channel->writeString(_outputStream, "Referer: ");
      _channel->writeString(_outputStream, baseUrl.c_str());
      _channel->writeString(_outputStream, CRLF);
    }
  }

  SBinetUtils::getInteger(properties, INET_CLOSE_CONNECTION, _closeConnection);

  if (_closeConnection)
  {
    _channel->writeString(_outputStream, "Connection: close" CRLF);
  }

  VXIint32 maxAge = -1;
  VXIint32 maxStale = -1;

  getCacheInfo(properties, maxAge, maxStale);

  if (maxAge != -1)
  {
    _channel->writeString(_outputStream, "Cache-Control: max-age=");
    _channel->writeInt(_outputStream, maxAge);
    _channel->writeString(_outputStream, CRLF);
  }

  if (maxStale != -1)
  {
    _channel->writeString(_outputStream, "Cache-Control: max-stale=");
    _channel->writeInt(_outputStream, maxStale);
    _channel->writeString(_outputStream, CRLF);
  }

  time_t lastModified = (time_t) -1;
  SBinetNString etag;

  if (_cacheValidator)
  {
    // Otherwise, use the validator that was stored in the cache, if any.
    lastModified = _cacheValidator->getLastModified();
    const VXIchar* eTagStr = _cacheValidator->getETAG();
    if (eTagStr)
      etag = eTagStr;
  }

  if ((method == SBinetHttpStream::GET_METHOD) && (lastModified != (time_t) -1))
  {
    char lastModStr[32];
    ap_gm_timestr_822(lastModStr, lastModified);
    _channel->writeString(_outputStream, "If-Modified-Since: ");
    _channel->writeString(_outputStream, lastModStr);
    _channel->writeString(_outputStream, CRLF);
  }

  if (etag.length() > 0 && etag[0])     
  {
    _channel->writeString(_outputStream, "If-None-Match: ");
    _channel->writeString(_outputStream, etag.c_str());
    _channel->writeString(_outputStream, CRLF);
  }

  if (_channel->cookiesEnabled())
  {
    _channel->collectCookies(_outputStream, _url->getNHost(), _url->getNPath());
  }

  return VXIinet_RESULT_SUCCESS;
}

VXIinetResult SBinetHttpStream::flushAndCheckConnection()
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SWIstream::Result src = _outputStream->flush();
  switch( src ) {
    case SWIstream::SUCCESS:
      break;
    case SWIstream::CONNECTION_ABORTED:
    case SWIstream::CONNECTION_RESET:
      _connectionAborted = TRUE;
      // If the server reset the connection 
      // due to keepalive timeout, connection timeout, etc.
      // then retry the connection again without output the error
      // intentional fall through        
    default:
      Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::doGet",
           L"_outputStream->flush() error = %d, connection %s", 
           src, (_connectionAborted ? L"aborted" : L"error"));     
      rc = VXIinet_RESULT_IO_ERROR;
  }  
  return rc;
}

VXIinetResult SBinetHttpStream::doGet(VXIint32 flags,
                                      const VXIMap *properties,
                                      VXIMap *streamInfo)

{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  try
  {
    rc = initSocket(GET_METHOD, properties, streamInfo);
    if (rc != VXIinet_RESULT_SUCCESS)
      return rc;

    _channel->writeString(_outputStream, CRLF);

    rc = flushAndCheckConnection();
    
  }
  catch (std::exception& e)
  {
    // An error occurred sending data over the socket.  Abort the connection and
    // try again.
    if (strcmp(e.what(), "CONNECTION_ABORTED") == 0)
      _connectionAborted = TRUE;
    rc = VXIinet_RESULT_IO_ERROR;
    Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::doGet",
         L"initSocket caught exception return error = %d, connection aborted", rc);     
  }

  return rc;
}

/*
 * POST simple form data. Don't know how to combine Form data and audio in multipart POST
 */
VXIinetResult SBinetHttpStream::doPost(VXIint32 flags,
                                       const VXIMap *properties,
                                       VXIMap *streamInfo)
{
  const VXIMap *queryArgs =
    (const VXIMap *)VXIMapGetProperty(properties, INET_URL_QUERY_ARGS);

  SBinetNString queryArgsStr;
  VXIint argsLen;

  EncodingType encodingType = getEncodingType(properties);
  const char *encodingStr;

  if (encodingType == TYPE_MULTIPART)
  {
    encodingStr = SB_MULTIPART;
    queryArgsStr = _url->queryArgsToMultipart(queryArgs);
    argsLen = queryArgsStr.length();
    Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetHttpStream::doPost",
         L"Performing multipart POST, data size %i", argsLen);
  }
  else
  {
    encodingStr = SB_URLENCODED;
    queryArgsStr = _url->queryArgsToNString(queryArgs);
    argsLen = queryArgsStr.length();
    Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetHttpStream::doPost",
         L"Performing URL-encoded POST, data size %i", argsLen);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  
  try
  {
    rc = initSocket(POST_METHOD, properties, streamInfo);
    if (rc != VXIinet_RESULT_SUCCESS)
      return rc;

    _channel->writeString(_outputStream, "Expect: 100-continue" CRLF);

    _channel->writeString(_outputStream, "Content-Length: ");
    _channel->writeInt(_outputStream, argsLen);
    _channel->writeString(_outputStream, CRLF);

    _channel->writeString(_outputStream, "Content-Type: ");
    _channel->writeString(_outputStream, encodingStr);
    _channel->writeString(_outputStream, CRLF CRLF);

    rc = flushAndCheckConnection();
    if( rc != VXIinet_RESULT_SUCCESS ) 
      return rc;

    // Wait for input stream to be ready.  If time-out, just send data anyway.
    // If we don't time out, we should be able to get an HTTP_CONTINUE.
    VXIint32 continueTimeout = SBinetChannel::getPostContinueTimeout();
    int maxDelay = getDelay();
    if (continueTimeout < 0 ||
        (maxDelay >=0 && continueTimeout > maxDelay))
      continueTimeout = maxDelay;
    
    SWIstream::Result src = _inputStream->waitReady(continueTimeout);
    switch (src)
    {
     case SWIstream::SUCCESS:
       // Read the HTTP_CONTINUE header
       if ((rc = getHeaderInfo(streamInfo)) != VXIinet_RESULT_SUCCESS)
         return rc;

       if (_HttpStatus != SBinetHttpUtils::HTTP_CONTINUE) {
         Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::doPost",
              L"%s%d", L"waitReady epxected HTTP_CONTINUE but got status ", _HttpStatus);     
         return VXIinet_RESULT_FETCH_ERROR;
       }
       
       break;
     case SWIstream::TIMED_OUT:
       // We still have to send the body of the message.  This is done
       // after the switch anyway so we do nothing.
       break;
     default:
      Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::doPost",
           L"%s%d", L"waitReady returned IO ERROR ", src);     
       // I/O error on the stream.  Just return an error.
       return VXIinet_RESULT_FETCH_ERROR;
    }

    if (encodingType == TYPE_MULTIPART)
    {
      _channel->writeData(_outputStream, queryArgsStr.data(), argsLen);
    }
    else
    {
      _channel->writeString(_outputStream, queryArgsStr.c_str());
    }

    rc = flushAndCheckConnection();
    if( rc != VXIinet_RESULT_SUCCESS )
      return rc;
  }
  catch (std::exception& e)
  {
    // An error occurred sending data over the socket.  Abort the connection and
    // try again.
    Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::doPost",
         L"%s", L"exception caught returned IO ERROR for retry");     
    if( strcmp(e.what(), "CONNECTION_ABORTED") == 0 )
      _connectionAborted = TRUE;
    return VXIinet_RESULT_IO_ERROR;
  }

  return rc;
}

bool SBinetHttpStream::handleRedirect(VXIMap *streamInfo)
{
  const VXIchar *location;

  switch (_HttpStatus)
  {
    case SBinetHttpUtils::HTTP_SEE_OTHER:
     // This return code implies that we change a POST into a GET.
     _method = GET_METHOD;
     // no break: intentional

   case SBinetHttpUtils::HTTP_MULTIPLE_CHOICES:
   case SBinetHttpUtils::HTTP_PERM_REDIRECT:
   case SBinetHttpUtils::HTTP_FOUND:
   case SBinetHttpUtils::HTTP_TEMP_REDIRECT:
     location = SBinetUtils::getString(streamInfo, L"Location");
     if (location == NULL || !*location)
     {
       Error(247, L"%s%s", L"URL", _url->getAbsolute());
       return false;
     }

     if (_url->parse(location, _url->getAbsolute()) != VXIinet_RESULT_SUCCESS)
     {
       Error(248, L"%s%s%s%s", L"URL", _url->getAbsolute(),
             L"Location", location);
       return false;
     }

     return true;
     break;
   default:
     return false;
  }
}

// stupid function required because the people who designed VXIMap, in their
// great wisdom, did not include a VXIMapClear function or the ability for the
// iterator to delete the current entry.
static void clearMap(VXIMap *streamInfo)
{
#if (VXI_CURRENT_VERSION >= 0x00030000)
  VXIMapClear(streamInfo);
#else
  std::list<inetstring> keys;
  const VXIchar *key = NULL;
  const VXIValue *value = NULL;
  int i = 0;
  VXIMapIterator *mapIterator = VXIMapGetFirstProperty(streamInfo,
                                                       &key,
                                                       &value );
  do
  {
    if (key != NULL)
    {
      keys.push_back(key);
    }
  }
  while (VXIMapGetNextProperty(mapIterator, &key, &value) == VXIvalue_RESULT_SUCCESS);
  VXIMapIteratorDestroy(&mapIterator);
  while (!keys.empty())
  {
    VXIMapDeleteProperty(streamInfo, keys.front().c_str());
    keys.pop_front();
  }
#endif  
}


VXIinetResult
SBinetHttpStream::Open(VXIint32                  flags,
		       const VXIMap             *properties,
		       VXIMap                   *streamInfo)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  // Make a copy in case of redirect.
  SBinetString initialUrl = _url->getAbsolute();

  const int maxRetry    = 5;
  int nbRetry           = 0;
  const int maxRedirect = 5;
  int nbRedirect        = 0;
  time_t requestTime = (time_t) -1;

  // If reconnection happens, attempt to retry up to maxRetry
  for (; nbRetry < maxRetry; )
  {
    // Set up request
    rc = VXIinet_RESULT_NON_FATAL_ERROR;
    _HttpStatus = HTTP_UNDEFINED;
    requestTime = time(NULL);
    _chunked = false;
    _leftToRead = ~0;
    _closeConnection = !SBinetChannel::getUsePersistentConnections();
    _connectionAborted = FALSE;

    if (_method == GET_METHOD)
      rc = doGet(flags, properties, streamInfo);
    else if (_method == POST_METHOD)
      rc = doPost(flags, properties, streamInfo);

    if ((rc != VXIinet_RESULT_SUCCESS) ||
       ((rc = getHeaderInfo(streamInfo)) != VXIinet_RESULT_SUCCESS))
    {      
      if (_connectionAborted)
      {
        _closeConnection = TRUE;
        // If the connection was reset by the HTTP server, create a new
        // connection and try again.
        Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::Open",
             L"%s%s%d", (_method == GET_METHOD ? L"GET" : L"POST"), 
             L" method: connection is aborted, retry ", nbRetry);     
        Close();
        ++nbRetry;
        continue;
      }
      else
      {
        Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::Open",
             L"%s%d", L"(no retry) getHeaderInfo returned error: ", rc); 
        _closeConnection = TRUE;
        break;
      }      
    }       
       
    // Avoid race condition on 100-Continue time-out.  If we got a CONTINUE
    // from previous call to getHeaderInfo, we need to get it again.
    if (_HttpStatus == SBinetHttpUtils::HTTP_CONTINUE &&
        (rc = getHeaderInfo(streamInfo)) != VXIinet_RESULT_SUCCESS)
    {
      Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::Open",
           L"%s", L"Unexpected HTTP_CONTINUE"); 
      _closeConnection = TRUE;
      break;
    }

    if (_HttpStatus >= 300 && _HttpStatus <= 399 &&
        handleRedirect(streamInfo))
    {
      // perform a reset on the stream.
      Close();
      if (++nbRedirect > maxRedirect)
      {
        Error(229, L"%s%s", L"URL", initialUrl.c_str());
        return VXIinet_RESULT_FETCH_ERROR;
      }
      // Clear the streamInfo map to avoid previous fetch attribute to remain
      // in the map.
      if (streamInfo != NULL)
        clearMap(streamInfo);
        
      // if redirect happens, reset retry value
      nbRetry = 0;
    }
    else
      break;
  }

  if (streamInfo != NULL && _HttpStatus >= 100 && _HttpStatus <= 999)
  {
    VXIMapSetProperty(streamInfo, INET_INFO_HTTP_STATUS,
                      (VXIValue*) VXIIntegerCreate(_HttpStatus));

    // Set HTTP data source information   
    VXIMapSetProperty(streamInfo, INET_INFO_DATA_SOURCE, 
                     (VXIValue*) VXIIntegerCreate(INET_DATA_SOURCE_HTTP));
  }

  switch (_HttpStatus)
  {
   case SBinetHttpUtils::HTTP_OK:
     if (streamInfo != NULL)
     {
       processHeaderInfo(streamInfo);
       setValidatorInfo(streamInfo, requestTime);
     }
     rc = VXIinet_RESULT_SUCCESS;
     break;
   case SBinetHttpUtils::HTTP_NOT_MODIFIED:
     if (streamInfo != NULL) setValidatorInfo(streamInfo, requestTime, (VXIulong) -1);
     Close();
     rc = VXIinet_RESULT_NOT_MODIFIED;
     break;
   default:
     Close();

     // Map the HTTP error code
     if (_HttpStatus != HTTP_UNDEFINED)
     {
       const VXIchar *errorDesc;
       rc = MapError(_HttpStatus, &errorDesc);
       Error(219, L"%s%s%s%s%s%d%s%s", L"URL", initialUrl.c_str(),
             L"Method", (_method == GET_METHOD) ? L"GET" : L"POST",
             L"HTTPStatus", _HttpStatus, L"HTTPStatusDescription", errorDesc);
     }
     else
     {
       Error(217, L"%s%s", L"URL", initialUrl.c_str());
       if (rc == VXIinet_RESULT_SUCCESS)
         rc = VXIinet_RESULT_NON_FATAL_ERROR;
     }
     break;
  }

  return rc;
}


VXIinetResult SBinetHttpStream::Close()
{
  if (_connection != NULL)
  {
    if (_closeConnection)
      delete _connection;
    else
    {
      // First make sure there is nothing left in the body.
      VXIinetResult rc = skipBody();
      if (rc == VXIinet_RESULT_SUCCESS)
        _channel->putHttpConnection(_connection);
      else
        delete _connection;
    }

    _connection = NULL;
    _inputStream = NULL;
    _outputStream = NULL;
  }

  return(VXIinet_RESULT_SUCCESS);
}


VXIinetResult SBinetHttpStream::skipBody()
{
  // These HTTP status code do not allow for a body.
  if (_HttpStatus == SBinetHttpUtils::HTTP_NOT_MODIFIED ||
      _HttpStatus == SBinetHttpUtils::HTTP_NO_DATA  ||
      (_HttpStatus >= 100 && _HttpStatus <= 199))
    return VXIinet_RESULT_SUCCESS;

  VXIinetResult rc;
  VXIbyte buffer[1024];

  while ((rc = Read(buffer, sizeof(buffer), NULL)) == VXIinet_RESULT_SUCCESS);
  if (rc == VXIinet_RESULT_END_OF_STREAM)
    rc = VXIinet_RESULT_SUCCESS;

  return rc;
}

SBinetHttpStream::EncodingType SBinetHttpStream::getEncodingType(const VXIMap *properties)
{
  SBinetHttpStream::EncodingType encodingType = TYPE_URL_ENCODED;

  // Get the submit MIME type from the properties, if defined
  const VXIchar *strType =
    SBinetUtils::getString(properties, INET_SUBMIT_MIME_TYPE);

  if (strType && *strType)
  {
    // If DEFAULT ever changes, this code will need to be updated
    if (::wcscmp(strType, INET_SUBMIT_MIME_TYPE_DEFAULT) == 0)
      encodingType = TYPE_URL_ENCODED;
    else if (::wcscmp(strType, L"multipart/form-data") == 0)
      encodingType = TYPE_MULTIPART;
    else
      Error(304, L"%s%s%s%s%s%s", L"URL", _url->getAbsolute(),
            L"EncodingType", strType,
            L"DefaultType", INET_SUBMIT_MIME_TYPE_DEFAULT);
  }

  return encodingType;
}

VXIinetResult SBinetHttpStream::getStatus()
{
  _HttpStatus = HTTP_UNDEFINED;

  VXIinetResult rc = waitStreamReady();
  if (rc != VXIinet_RESULT_SUCCESS)
    return rc;

  writeDebugTimeStamp();

  // The HTTP status line should have the following syntax:
  // HTTP/x.x yyy msg CRLF
  //
  // This is buggy if we ever see an HTTP/1.xx in the future
  SWIdataOutputStream line;
  const char *c_line = NULL;

  SWIstream::Result src = _channel->readLine(_inputStream, &line);
  if (src >= 0)
  {
    if (ap_checkmask(c_line = line.getString(), "HTTP/#.# ###*"))
    {
      _HttpStatus = atoi(&c_line[9]);
      int version = (c_line[5] - '0') * 10 + (c_line[7] - '0');
      if (version < 11) _closeConnection = TRUE;
    }
    else
    {
      Error(249, L"%s%s", L"URL", _url->getAbsolute());
    }
  }
  // A return value of END_OF_FILE means the connection was gracefully closed.
  // A return value of CONNECTION_ABORTED means the connection was aborted
  // because of a time out or some other problem.
  else if ((src == SWIstream::CONNECTION_ABORTED) || (src == SWIstream::END_OF_FILE))
  {
    // Return immediately and do not check the _HttpStatus since
    // we don't want to print errors for nothing.
    _connectionAborted = true;
    return VXIinet_RESULT_NON_FATAL_ERROR;
  }
  else
  {
    Error(249, L"%s%s", L"URL", _url->getAbsolute());
  }

  if (_HttpStatus < 100 || _HttpStatus > 999)
  {
    if (_HttpStatus != HTTP_UNDEFINED)
    {
      Error(219, L"%s%s%s%s%s%d",
            L"URL", _url->getAbsolute(),
            L"Method", _method == GET_METHOD ? L"GET" : L"POST",
            L"HTTPStatus", _HttpStatus);
    }
    else
    {
      Error(221, L"%s%s", L"URL", _url->getAbsolute());
    }
    rc = VXIinet_RESULT_NON_FATAL_ERROR;
  }

  return rc;
}

enum HandlerValueType
{
  HANDLER_INT = 0x01,
  HANDLER_DATE = 0x02,
  HANDLER_STRING = 0x04,
  HANDLER_CONTENT = 0x08,
  HANDLER_LIST = 0x10
};


// Currently doing a linear search because of case-insenstivity of
// comparisons.  If the number of entries in this array increases, we should
// consider using a hash implementation instead.
SBinetHttpStream::HeaderInfo SBinetHttpStream::headerInfoMap[] =
{
  { "Age", NULL, HANDLER_INT, NULL },
  { "Cache-Control", NULL, HANDLER_STRING | HANDLER_LIST, NULL },
  { "Content-Length", INET_INFO_SIZE_BYTES, HANDLER_INT, contentLengthHandler },
  { "Content-Type", INET_INFO_MIME_TYPE, HANDLER_STRING, NULL },
  { "Connection", NULL, HANDLER_STRING, connectionHandler },
  { "Date", NULL, HANDLER_DATE, NULL },
  { "ETag", NULL, HANDLER_STRING, NULL },
  { "Expires", NULL, HANDLER_DATE, NULL },
  { "Last-Modified", NULL, HANDLER_DATE, NULL },
  { "Location", NULL, HANDLER_STRING, NULL },
  { "Pragma", NULL, HANDLER_STRING, NULL },
  { "Set-Cookie", NULL, HANDLER_STRING, setCookieHandler },
  { "Set-Cookie2", NULL, HANDLER_STRING, setCookie2Handler },
  { "Transfer-Encoding", NULL, HANDLER_STRING, transferEncodingHandler},
  { NULL, NULL, 0, NULL }
};

void SBinetHttpStream::setCookieHandler(HeaderInfo *headerInfo,
                                        const char *value,
                                        SBinetHttpStream *httpStream,
                                        VXIMap *streamInfo)
{
  if (!httpStream->_channel->cookiesEnabled())
    return;

  const char *p = value;

  while (p != NULL && *p)
  {
    SBinetCookie *cookie = SBinetCookie::parse(httpStream->_url, p, httpStream);
    if (cookie != NULL)
    {
      httpStream->_channel->updateCookie(cookie);
    }
  }
  httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                   L"SBinetHttpStream::setCookieHandler",
                   L"Set-Cookie: %S", value);
}

void SBinetHttpStream::setCookie2Handler(HeaderInfo *headerInfo,
                                         const char *value,
                                         SBinetHttpStream *httpStream,
                                         VXIMap *streamInfo)
{
  if (!httpStream->_channel->cookiesEnabled())
    return;

  httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                   L"SBinetHttpStream::setCookie2Handler",
                   L"Not Supported: \"Set-Cookie2: %S\"", value);
}

void SBinetHttpStream::contentLengthHandler(HeaderInfo *headerInfo,
                                            const char *value,
                                            SBinetHttpStream *httpStream,
                                            VXIMap *streamInfo)
{
  // Ignore content-length if stream is chunked.
  if (!httpStream->_chunked)
  {
    httpStream->_leftToRead = atoi(value);
  }
}

void SBinetHttpStream::connectionHandler(HeaderInfo *headerInfo,
                                         const char *value,
                                         SBinetHttpStream *httpStream,
                                         VXIMap *streamInfo)
{
  SBinetNString attrib;

  for (;;)
  {
    if ((value = SBinetHttpUtils::getValue(value, attrib)) == NULL)
    {
      httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                       L"SBinetHttpStream::connectionHandler",
                       L"Could not get attribute.");
      break;
    }

    if ((value = SBinetHttpUtils::expectChar(value, ",")) == NULL)
    {
      httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                       L"SBinetHttpStream::connectionHandler",
                       L"Expecting ','.");
      break;
    }

    if (::strcasecmp(attrib.c_str(), "close") == 0)
    {
      httpStream->_closeConnection = TRUE;
    }

    // Skip comma, or if at end of string, stop parsing.
    if (*value)
      value++;
    else
      break;
  }
}

void SBinetHttpStream::transferEncodingHandler(HeaderInfo *headerInfo,
                                               const char *value,
                                               SBinetHttpStream *httpStream,
                                               VXIMap *streamInfo)
{
  SBinetNString encoding;

  for (;;)
  {
    if ((value = SBinetHttpUtils::getValue(value, encoding)) == NULL)
    {
      httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                       L"SBinetHttpStream::transferEncodingHandler",
                       L"Could not get encoding.");
      break;
    }

    if ((value = SBinetHttpUtils::expectChar(value, ",")) == NULL)
    {
      httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                       L"SBinetHttpStream::transferEncodingHandler",
                       L"Expecting ','.");
      break;
    }

    if (::strcasecmp(encoding.c_str(), "chunked") == 0)
    {
      httpStream->_chunked = true;
      httpStream->_leftToRead = 0;
    }

    // Skip comma, or if at end of string, stop parsing.
    if (*value)
      value++;
    else
      break;
  }
}

void SBinetHttpStream::defaultHeaderHandler(HeaderInfo *headerInfo,
                                            const char *value,
                                            SBinetHttpStream *httpStream,
                                            VXIMap *streamInfo)
{
  VXIValue* vxi_value = NULL;
  bool listF = (headerInfo->value_type & HANDLER_LIST) == HANDLER_LIST;
  int value_type = headerInfo->value_type & ~HANDLER_LIST;
  const VXIchar *property = NULL;
  SBinetString *propTmp = NULL;

  if (headerInfo->inet_property != NULL)
  {
    property = headerInfo->inet_property;
  }
  else
  {
    propTmp = new SBinetString(headerInfo->header);
    property = propTmp->c_str();
  }

  switch (value_type)
  {
   case HANDLER_INT:
     vxi_value = (VXIValue *) VXIIntegerCreate(atoi(value));
     break;
   case HANDLER_STRING:
     if (listF)
     {
       const VXIchar *v = SBinetUtils::getString(streamInfo, property);
       if (v != NULL)
       {
         SBinetString tmp = v;
         tmp += L", ";
         tmp += value;
         vxi_value = (VXIValue *) VXIStringCreate(tmp.c_str());
       }
     }
     if (vxi_value == NULL)
     {
       vxi_value = (VXIValue *) VXIStringCreate(SBinetString(value).c_str());
     }
     break;
   case HANDLER_DATE:
     {
       // FIXME: Portability issue. This code assumes that a time_t can be
       // stored in a VXIint32 without losing any information.  Two possible
       // options: extend VXIValue with a VXILong type or use VXIContent.
       time_t timestamp = ap_parseHTTPdate(value);
       vxi_value = (VXIValue *) VXIIntegerCreate(timestamp);
     }
     break;
   case HANDLER_CONTENT:
     if (listF)
     {
       const char *content;
       VXIulong contentLength;

       if (SBinetUtils::getContent(streamInfo, property, content, contentLength))
       {
         SBinetNString tmp = content;
         tmp += ", ";
         tmp += value;
         vxi_value = (VXIValue *) SBinetUtils::createContent(tmp.c_str());
       }
     }
     if (vxi_value == NULL)
     {
       vxi_value = (VXIValue *) SBinetUtils::createContent(value);
     }
     break;
   default:
     break;
  }

  if (vxi_value != NULL)
  {
    VXIMapSetProperty(streamInfo, property, vxi_value);
  }
  else
  {
    httpStream->Diag(MODULE_SBINET_STREAM_TAGID,
                     L"SBinetHttpStream::defaultHeaderHandler",
                     L"Could not create VXI value, header: <%S>, value: <%S>",
                     headerInfo->header,
                     value);
  }

  delete propTmp;
}

SBinetHttpStream::HeaderInfo *SBinetHttpStream::findHeaderInfo(const char *header)
{
  for (int i = 0; headerInfoMap[i].header != NULL; i++)
  {
    if (::strcasecmp(headerInfoMap[i].header, header) == 0)
    {
      return &headerInfoMap[i];
    }
  }

  return NULL;
}

void SBinetHttpStream::processHeader(const char *header,
                                     const char *value,
                                     VXIMap* streamInfo)
{
  HeaderInfo *headerInfo = findHeaderInfo(header);

  // If it is not found, we ignore it.
  if (headerInfo == NULL) return;

  HeaderHandler handler = headerInfo->handler;
  if (handler == NULL) handler = defaultHeaderHandler;

  (*handler)(headerInfo, value, this, streamInfo);
}

static const unsigned long BREAK_AT_INITIAL_NEWLINE = 0x01;
static const unsigned long CONTINUE_AT_NEWLINE = 0x02;

int SBinetHttpStream::getValue(char *&value, unsigned long mask)
{
  char c;
  bool done = false;

  // skip white spaces.
  for (;;)
  {
    if ((c = _channel->readChar(_inputStream)) < 0)
      return -8;

    if (!ap_isspace(c))
      break;

    switch (c)
    {
     case '\r':
       if ((c = _channel->readChar(_inputStream)) != '\n')
         return -5;
       // no break: intentional

     case '\n':
       if (mask & BREAK_AT_INITIAL_NEWLINE)
       {
         if ((c = _inputStream->peek()) < 0) return -6;
         if (!ap_isspace(c)) done = true;
       }

       if (done)
       {
         value = new char[2];
         *value = '\0';
         Diag(MODULE_SBINET_STREAM_TAGID,
              L"SBinetHttpStream::getValue",
              L"value = '%S'", value);
         return 0;
       }
       break;

     default:
       break;
    }
  }

  int len = 0;
  int bufSize = 32;
  value = new char[bufSize];

  for (;;)
  {
    if (len == bufSize)
    {
      int newBufSize = (bufSize << 1);
      char *new_value = ::strncpy(new char[newBufSize], value, bufSize);
      delete [] value;
      value = new_value;
      bufSize = newBufSize;
    }

    if (c == '\r')
    {
      if ((c = _channel->readChar(_inputStream)) != '\n')
        break;
    }

    // We need to verify if the value continues on the next line.
    if (c == '\n')
    {
      if (mask & CONTINUE_AT_NEWLINE)
      {
        if ((c = _inputStream->peek()) < 0)
          break;

        if (!ap_isspace(c) || c == '\r' || c == '\n')
          done = true;
      }
      else
        done = true;

      if (done)
      {
        // remove trailing blanks and terminate string.
        while (len > 0 && ap_isspace(value[len-1])) len--;
        value[len] = '\0';
        Diag(MODULE_SBINET_STREAM_TAGID,
             L"SBinetHttpStream::getValue",
             L"value = '%S'", value);
        return 0;
      }
    }

    value[len++] = c;

    if ((c = _channel->readChar(_inputStream)) < 0)
      break;
  }

  // If we get here, we got an error parsing the line.
  delete [] value;
  value = NULL;
  return -7;
}

int SBinetHttpStream::getHeader(char *&header, char delimiter)
{
  int c;

  if ((c = _channel->readChar(_inputStream)) < 0)
    return -1;

  // Check for end of headers.
  if (c == '\r')
  {
    if ((c = _channel->readChar(_inputStream)) != '\n')
      return -2;
    return 1;
  }

  if (c == '\n') return 2;

  if (ap_isspace(c) || c == (unsigned char) delimiter)
    return -3;

  int len = 0;
  int bufSize = 32;
  header = new char[bufSize];

  for (;;)
  {
    if (len == bufSize)
    {
      int newBufSize = bufSize << 1;
      char *new_header = ::strncpy(new char[newBufSize], header, bufSize);
      delete [] header;
      header = new_header;
      bufSize = newBufSize;
    }
    if (c == (unsigned char) delimiter)
    {
      header[len++] = '\0';
      Diag(MODULE_SBINET_STREAM_TAGID,
           L"SBinetHttpStream::getHeader",
           L"header = '%S'", header);
      return 0;
    }
    header[len++] = c;

    if ((c = _channel->readChar(_inputStream)) < 0 || ap_isspace(c))
      break;
  }

  delete [] header;
  header = NULL;
  return -4;
}

int SBinetHttpStream::getHeaderValue(char *&header, char *&value)
{
  int rc = getHeader(header, ':');

  if (rc == 0)
  {
    rc = getValue(value, CONTINUE_AT_NEWLINE | BREAK_AT_INITIAL_NEWLINE);
    if (rc != 0)
    {
      Error(250, NULL);
      delete [] header;
      header = NULL;
    }
  }
  else if (rc < 0)
    Error(250, NULL);

  return rc;
}

VXIinetResult SBinetHttpStream::parseHeaders(VXIMap* streamInfo)
{
  char *header = NULL;
  char *value  = NULL;
  VXIinetResult rc1;
  int rc2;

  while (((rc1 = waitStreamReady()) == VXIinet_RESULT_SUCCESS) &&
         ((rc2 = getHeaderValue(header, value)) == 0))
  {
    if (streamInfo) processHeader(header, value, streamInfo);
    delete [] header;
    delete [] value;
  }

  if (rc1 == VXIinet_RESULT_SUCCESS && rc2 < 0)
  {
    Diag(MODULE_SBINET_STREAM_TAGID,
         L"SBinetHttpStream::parseHeaders",
         L"Error in parsing header line: URL = <%s>, METHOD = %s, rc2 = %d",
         _url->getAbsolute(),
         _method == GET_METHOD ? L"GET" : L"POST",
         rc2);
    rc1 = VXIinet_RESULT_NON_FATAL_ERROR;
  }
  return rc1;
}


void SBinetHttpStream::processHeaderInfo(VXIMap *streamInfo)
{
  // Extract the content-length to find out how many bytes are left to be
  // read from the stream.  If the stream is chunked, ignore
  // content-length from the server.  If we don't have the content, we
  // have to set the property to 0.
  VXIulong contentSize = (_chunked || _leftToRead == ~0) ? 0 : _leftToRead;
  VXIMapSetProperty(streamInfo, INET_INFO_SIZE_BYTES,
                    (VXIValue *) VXIIntegerCreate((int) contentSize));

  const VXIchar *tmp = _url->getAbsolute();
  if (tmp != NULL)
  {
    Diag(MODULE_SBINET_STREAM_TAGID,
         L"SBinetHttpStream::processHeaderInfo", L"%s%s",
         L"absolute", tmp);
    VXIMapSetProperty(streamInfo,
                      INET_INFO_ABSOLUTE_NAME,
                      (VXIValue*)VXIStringCreate(tmp));
  }

  const VXIString *contentType =
    (const VXIString *) VXIMapGetProperty(streamInfo, INET_INFO_MIME_TYPE);

  tmp = contentType != NULL ? VXIStringCStr(contentType) : NULL;

  if (!tmp || !*tmp
      || ::wcscmp(tmp, _channel->getDefaultMimeType()) == 0)
  {
    VXIString *newContentType = _url->getContentTypeFromUrl();
    if (newContentType != NULL)
    {
      Error (300, L"%s%s%s%s",
             L"URL", _url->getAbsolute(),
             L"mime-type", VXIStringCStr(newContentType));
    }
    else
    {
      newContentType = VXIStringCreate(_channel->getDefaultMimeType());
      Error (301, L"%s%s", L"URL", _url->getAbsolute());
    }
    VXIMapSetProperty(streamInfo, INET_INFO_MIME_TYPE,
		      (VXIValue *) newContentType);
  }
}


void SBinetHttpStream::setValidatorInfo(VXIMap *streamInfo, time_t requestTime)
{
  _validator = new SBinetValidator(GetLog(), GetDiagBase());

  if (_validator->Create(_url->getAbsolute(), requestTime, streamInfo) == VXIinet_RESULT_SUCCESS)
  {
    SBinetUtils::setValidatorInfo(streamInfo, _validator);
  }
}

void SBinetHttpStream::setValidatorInfo(VXIMap *streamInfo, time_t requestTime, const VXIulong sizeBytes)
{
  _validator = new SBinetValidator(GetLog(), GetDiagBase());

  if (_validator->Create(_url->getAbsolute(), sizeBytes, requestTime, streamInfo) == VXIinet_RESULT_SUCCESS)
  {
    SBinetUtils::setValidatorInfo(streamInfo, _validator);
  }
}

VXIinetResult SBinetHttpStream::getHeaderInfo(VXIMap* streamInfo)
{
  SWITimeWatch timeWatch;
  timeWatch.start();

  VXIinetResult rc = getStatus();
  Diag(MODULE_SBINET_TIMING_TAGID, L"SBinetHttpStream::getStatus",
       L"%i msecs, Wait for HTTP response", timeWatch.getElapsed());

  if (rc == VXIinet_RESULT_SUCCESS)
  {
    rc = parseHeaders(streamInfo);
  }

  Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetHttpStream::getHeaderInfo",
       L"rc = %d, HttpStatus = %d", rc, _HttpStatus);
  return rc;
}

VXIinetResult SBinetHttpStream::getChunkSize(VXIulong& chunk_size)
{
  // Read next chunk size.
  char *chunk_size_str;

  if (getValue(chunk_size_str, 0) < 0)
  {
    Error(245, L"%s%s", L"URL", _url->getAbsolute());
    return VXIinet_RESULT_FETCH_ERROR;
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  bool parseError = false;
  char *endParse;
  chunk_size = (VXIulong) strtol(chunk_size_str, &endParse, 16);

  // If we did not parse a single character, this is an error.
  if (endParse == chunk_size_str)
  {
    parseError = true;
  }
  else if (*endParse)
  {
    // We did not stop parsing at the end of the string.  If the only
    // remaining characters are whitespace, this is not an error.
    while (*endParse && ap_isspace(*endParse));

    // This is not really an eror as there might be a chunk extension that we
    // currently ignore.
    //
    //if (*endParse) parseError = true;
  }

  if (parseError)
  {
    // Either an empty string to parse or stopped at non hexadecimal
    // character.
    Error(246, L"%s%s%s%S", L"URL", _url->getAbsolute(),
          L"ChunkSize", chunk_size_str);
    rc = VXIinet_RESULT_FETCH_ERROR;
  }

  delete [] chunk_size_str;
  return rc;
}

static SWIstream::Result inetToStream(VXIinetResult rc)
{
  switch (rc)
  {
   case VXIinet_RESULT_SUCCESS:
     return SWIstream::SUCCESS;
   case VXIinet_RESULT_FETCH_TIMEOUT:
     return SWIstream::TIMED_OUT;
   default:
     return SWIstream::READ_ERROR;
  }
}

int SBinetHttpStream::readChunked(VXIbyte *buffer,
                                  VXIulong buflen)
{
  // Note.  This is not fully compliant as it does not parse chunk extension.
  // The trailer is parsed but no action is taken.
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  if (_leftToRead == ~0) return 0;

  int totalRead = 0;

  for (;;)
  {
    if (_leftToRead == 0)
    {
      if ((rc = waitStreamReady()) != VXIinet_RESULT_SUCCESS)
        return ::inetToStream(rc);

      if ((rc = getChunkSize(_leftToRead)) != VXIinet_RESULT_SUCCESS)
        return SWIstream::READ_ERROR;

      if (_leftToRead == 0)
      {
        parseHeaders(NULL);
        _leftToRead = ~0;
        break;
      }
    }

    if (buflen == 0) break;

    VXIulong toRead = (buflen > _leftToRead ? _leftToRead : buflen);

    VXIulong nbRead = 0;
    while (toRead > 0)
    {
      if ((rc = waitStreamReady()) != VXIinet_RESULT_SUCCESS)
        return ::inetToStream(rc);

      int count = _channel->readData(_inputStream, &buffer[nbRead], toRead);

      // Read error.
      if (count == 0) count = SWIstream::WOULD_BLOCK;
      if (count < 0) return count;

      nbRead += count;
      toRead -= count;
    }

    _leftToRead -= nbRead;
    totalRead += nbRead;
    buffer += nbRead;
    buflen -= nbRead;
  }

  return totalRead;
}

int SBinetHttpStream::readNormal(VXIbyte *buffer, VXIulong buflen)
{
  int totalRead = 0;

  if (_leftToRead == 0) return totalRead;

  VXIulong toRead = (buflen > _leftToRead) ? _leftToRead : buflen;

  VXIulong nbRead = 0;
  while (toRead > 0)
  {
    // Check if fetch timeout has expired
    VXIinetResult rc = waitStreamReady();
    if (rc != VXIinet_RESULT_SUCCESS)
      return inetToStream(rc);

    int count = _channel->readData(_inputStream, &buffer[nbRead], toRead);

    // This is not an error if we get END OF FILE.
    if (count == SWIstream::END_OF_FILE)
    {
      _closeConnection = TRUE;
      _leftToRead = 0;
      break;
    }

    // Read error.
    if (count == 0) count = SWIstream::WOULD_BLOCK;
    if (count < 0) return count;

    nbRead += count;
    toRead -= count;
  }

  totalRead += nbRead;
  _leftToRead -= nbRead;
  return totalRead;
}



VXIinetResult SBinetHttpStream::Read(VXIbyte *buffer,
                                     VXIulong buflen,
                                     VXIulong *nread)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  int nbRead;

  if (_chunked)
    nbRead = readChunked(buffer, buflen);
  else
    nbRead = readNormal(buffer, buflen);

  if (nbRead >= 0)
  {
    if (nread != NULL) *nread = nbRead;
    if (((unsigned int) nbRead) < buflen ||
        (!_chunked && _leftToRead == 0))
      rc = VXIinet_RESULT_END_OF_STREAM;
  }
  else
  {
    _closeConnection = TRUE;

    switch (nbRead)
    {
     case SWIstream::END_OF_FILE:
       rc = VXIinet_RESULT_END_OF_STREAM;
       break;
     case SWIstream::WOULD_BLOCK:
       rc = VXIinet_RESULT_WOULD_BLOCK;
       break;
     case SWIstream::TIMED_OUT:
       rc = VXIinet_RESULT_FETCH_TIMEOUT;
       break;
     default:
       rc = VXIinet_RESULT_IO_ERROR;
    }
  }
  return rc;
}


VXIinetResult
SBinetHttpStream::MapError(int ht_error, const VXIchar **errorDesc)
{
  VXIinetResult rc;

  switch(ht_error)
  {
   case SBinetHttpUtils::HTTP_NO_ACCESS: /* Unauthorized */
      *errorDesc = L"Unauthorized";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_FORBIDDEN: /* Access forbidden */
      *errorDesc = L"Access forbidden";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_NOT_ACCEPTABLE:/* Not Acceptable */
      *errorDesc = L"Not Acceptable";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_NO_PROXY_ACCESS:    /* Proxy Authentication Failed */
      *errorDesc = L"Proxy Authentication Failed";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_CONFLICT:    /* Conflict */
      *errorDesc = L"Conflict";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_LENGTH_REQUIRED:    /* Length required */
      *errorDesc = L"Length required";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_PRECONDITION_FAILED:    /* Precondition failed */
      *errorDesc = L"Precondition failed";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_TOO_BIG:    /* Request entity too large */
      *errorDesc = L"Request entity too large";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_URI_TOO_BIG:    /* Request-URI too long */
      *errorDesc = L"Request-URI too long";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_UNSUPPORTED:    /* Unsupported */
      *errorDesc = L"Unsupported";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_BAD_RANGE:    /* Request Range not satisfiable */
      *errorDesc = L"Request Range not satisfiable";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_EXPECTATION_FAILED:    /* Expectation Failed */
      *errorDesc = L"Expectation Failed";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_REAUTH:    /* Reauthentication required */
      *errorDesc = L"Reauthentication required";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_PROXY_REAUTH:    /* Proxy Reauthentication required */
      *errorDesc = L"Proxy Reauthentication required";
      rc = VXIinet_RESULT_FETCH_ERROR; break;

   case SBinetHttpUtils::HTTP_SERVER_ERROR:	/* Internal server error */
      *errorDesc = L"Internal server error";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_NOT_IMPLEMENTED:	/* Not implemented (server error) */
      *errorDesc = L"Not implemented (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_BAD_GATEWAY:	/* Bad gateway (server error) */
      *errorDesc = L"Bad gateway (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_RETRY:	/* Service not available (server error) */
      *errorDesc = L"Service not available (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_GATEWAY_TIMEOUT:	/* Gateway timeout (server error) */
      *errorDesc = L"Gateway timeout (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_BAD_VERSION:	/* Bad protocol version (server error) */
      *errorDesc = L"Bad protocol version (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;
   case SBinetHttpUtils::HTTP_NO_PARTIAL_UPDATE:	/* Partial update not implemented (server error) */
      *errorDesc = L"Partial update not implemented (server error)";
      rc = VXIinet_RESULT_FETCH_ERROR; break;

    case SBinetHttpUtils::HTTP_INTERNAL:    /* Weird -- should never happen. */
      *errorDesc = L"Internal error";
      rc = VXIinet_RESULT_NON_FATAL_ERROR; break;

   case  SBinetHttpUtils::HTTP_WOULD_BLOCK:    /* If we are in a select */
      *errorDesc = L"Would block; not an error";
      rc = VXIinet_RESULT_WOULD_BLOCK; break;

   case SBinetHttpUtils::HTTP_INTERRUPTED:    /* Note the negative value! */
      *errorDesc = L"Interrupted; not an error";
      rc = VXIinet_RESULT_SUCCESS; break;
   case SBinetHttpUtils::HTTP_PAUSE:    /* If we want to pause a stream */
      *errorDesc = L"Stream paused; not an error";
      rc = VXIinet_RESULT_SUCCESS; break;
   case SBinetHttpUtils::HTTP_RECOVER_PIPE:    /* Recover pipe line */
      *errorDesc = L"Recover pipe line; not an error";
      rc = VXIinet_RESULT_SUCCESS; break;

   case SBinetHttpUtils::HTTP_TIMEOUT:    /* Connection timeout */
      *errorDesc = L"Connection timeout";
      rc = VXIinet_RESULT_FETCH_TIMEOUT; break;

   case SBinetHttpUtils::HTTP_NOT_FOUND: /* Not found */
      *errorDesc = L"Not found";
      rc = VXIinet_RESULT_NOT_FOUND; break;
   case SBinetHttpUtils::HTTP_METHOD_NOT_ALLOWED: /* Method not allowed */
      *errorDesc = L"Method not allowed";
      rc = VXIinet_RESULT_NON_FATAL_ERROR; break;
   case SBinetHttpUtils::HTTP_NO_HOST:    /* Can't locate host */
      *errorDesc = L"Can't locate host";
      rc = VXIinet_RESULT_NOT_FOUND; break;

    /* These should not be errors */
    case SBinetHttpUtils::HTTP_MULTIPLE_CHOICES:
      *errorDesc = L"Multiple choices";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_PERM_REDIRECT:
      *errorDesc = L"Permanent redirection";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_SEE_OTHER:
      *errorDesc = L"See other";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_USE_PROXY:
      *errorDesc = L"Use Proxy";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_PROXY_REDIRECT:
      *errorDesc = L"Proxy Redirect";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_TEMP_REDIRECT:
      *errorDesc = L"Temporary Redirect";
      rc = VXIinet_RESULT_NOT_FOUND; break;
    case SBinetHttpUtils::HTTP_FOUND:
      *errorDesc = L"Found";
      rc = VXIinet_RESULT_NON_FATAL_ERROR; break;

    default: {
      switch (ht_error / 100) {
      case 1: // informational, shouldn't fail due to this!
	*errorDesc = L"Unknown informational status";
	break;
      case 2: // successful, shouldn't fail due to this!
	*errorDesc = L"Unknown success status";
	break;
      case 3: // redirection
	*errorDesc = L"Unknown redirection error";
	break;
      case 4: // client error
	*errorDesc = L"Unknown client error";
	break;
      case 5: // server error
	*errorDesc = L"Unknown server error";
	break;
      default:
	*errorDesc = L"Unknown HTTP status code";
      }
      rc = VXIinet_RESULT_NON_FATAL_ERROR;
    }
  }

  return rc;
}

VXIinetResult SBinetHttpStream::Write(const VXIbyte*   pBuffer,
                                      VXIulong         nBuflen,
                                      VXIulong*        pnWritten)
{
  return VXIinet_RESULT_UNSUPPORTED;
}

VXIinetResult SBinetHttpStream::waitStreamReady()
{
  int delay = getDelay();

  if (delay == 0) {
    SWITimeStamp tstamp;
    getTimeOut(&tstamp);
    Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::waitStreamReady",
       L"%s%d", L"TimeOut occured, delay value ",tstamp.getAddedDelay());    
    Error(236, L"%s%d", L"waitStreamReady delay", delay);        
    return VXIinet_RESULT_FETCH_TIMEOUT;
  }
  
  switch (_inputStream->waitReady(delay))
  {
   case SWIstream::SUCCESS:
     return VXIinet_RESULT_SUCCESS;
     //no break: intentional
   case SWIstream::TIMED_OUT:
     Diag(MODULE_SBINET_CONNECTION_TAGID, L"SBinetHttpStream::waitStreamReady",
          L"%s%d", L"TimeOut occured for waitReady, delay value ", delay);     
     Error(236, L"%s%s", L"inputStream->waitReady", L"timeout");        
     return VXIinet_RESULT_FETCH_TIMEOUT;
     //no break: intentional
   default:
     Error(240, NULL);
     return VXIinet_RESULT_FETCH_ERROR;
  }
}

void SBinetHttpStream::writeDebugTimeStamp()
{
  // construct debug string
  char buffer[1400]; // total bytes needed + extra bytes
  char connId[200];
  char timeStamp[40];  

  // fill with NULL terminator chars
  memset(buffer, 0, 1400 * sizeof(char));   
  memset(connId, 0, 200 * sizeof(char));
  memset(timeStamp, 0, 40 * sizeof(char));  
  
  // we are sure the buffer is large enough since timeStamp's length is 40
  // connection id's is 200 and the buffer itself is 1400
  SBinetUtils::getTimeStampMsecStr(timeStamp);
  strncpy(connId, _connection->getId(), 200);
  sprintf(buffer, "%s, conn = %s, URL = ", timeStamp, connId);  
  // sprintf will automatically put \0 to output string, we can
  // safely call strlen() here.  Also, restrict the abs. uri to 1024 bytes
  strncpy(&buffer[strlen(buffer)], _url->getNAbsolute(), 1024);  
  // sprintf(buffer, "%s, URL = %s, conn = %s", timeStamp, absUri, connId);
  // send debug string          
  _channel->writeDebugString(CRLF "@ ");
  _channel->writeDebugString(buffer);
  _channel->writeDebugString(CRLF);
}
