
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

#ifndef SBINETHTTPSTREAM_HPP
#define SBINETHTTPSTREAM_HPP

#include "SBinetStoppableStream.hpp"

class SBinetChannel;
class SBinetHttpConnection;
class SWIinputStream;
class SWIoutputStream;
class SBinetValidator;

/**
 * This class is responsible to fetch HTTP documents.
 **/
class SBinetHttpStream : public SBinetStoppableStream
{
 public:
  enum SubmitMethod { GET_METHOD = 1, POST_METHOD = 2 };
  enum EncodingType { TYPE_URL_ENCODED = 1, TYPE_MULTIPART = 2 };

 public:
  SBinetHttpStream(SBinetURL* url, SubmitMethod method,
                   SBinetChannel* ch, SBinetValidator* cacheValidator,
                   VXIlogInterface *log, VXIunsigned diagLogBase);

  ~SBinetHttpStream();

  VXIinetResult Open(VXIint flags,
		     const VXIMap* properties,
		     VXIMap* streamInfo);

  VXIinetResult Read(/* [OUT] */ VXIbyte*         pBuffer,
                     /* [IN]  */ VXIulong         nBuflen,
                     /* [OUT] */ VXIulong*        pnRead );

  VXIinetResult Write(/* [IN]  */ const VXIbyte*   pBuffer,
                      /* [IN]  */ VXIulong         nBuflen,
                      /* [OUT] */ VXIulong*        pnWritten);

  VXIinetResult Close();

  SBinetChannel* getChannel()
  {
    return _channel;
  }

 public:
  int getHttpStatus() const
  {
    return _HttpStatus;
  }

 public:
  const SBinetValidator *getValidator() const
  {
    return _validator;
  }

 private:
  struct HeaderInfo;

  typedef void (*HeaderHandler)(HeaderInfo *,
                                const char *value,
                                SBinetHttpStream *,
                                VXIMap *);

  struct HeaderInfo
  {
    const char *header;
    const VXIchar *inet_property;
    int value_type;
    HeaderHandler handler;
  };

  VXIinetResult getStatus();

  VXIinetResult parseHeaders(VXIMap *streamInfo);

  void processHeader(const char *header,
                     const char *value,
                     VXIMap *streamInfo);

  static void connectionHandler(HeaderInfo *headerInfo,
                                const char *value,
                                SBinetHttpStream *stream,
                                VXIMap *streamInfo);

  static void setCookieHandler(HeaderInfo *headerInfo,
                               const char *value,
                               SBinetHttpStream *stream,
                               VXIMap *streamInfo);

  static void setCookie2Handler(HeaderInfo *headerInfo,
                                const char *value,
                                SBinetHttpStream *stream,
                                VXIMap *streamInfo);

  static void defaultHeaderHandler(HeaderInfo *headerInfo,
                                   const char *value,
                                   SBinetHttpStream *stream,
                                   VXIMap *streamInfo);

  static void transferEncodingHandler(HeaderInfo *headerInfo,
                                      const char *value,
                                      SBinetHttpStream *stream,
                                      VXIMap *streamInfo);

  static void contentLengthHandler(HeaderInfo *headerInfo,
                                   const char *value,
                                   SBinetHttpStream *stream,
                                   VXIMap *streamInfo);

  VXIinetResult doGet(VXIint32 flags,
                      const VXIMap *properties,
                      VXIMap *streamInfo);

  VXIinetResult doPost(VXIint32 flags,
                       const VXIMap *properties,
                       VXIMap *streamInfo);

  VXIinetResult flushAndCheckConnection();
  VXIinetResult getHeaderInfo(VXIMap* streamInfo);
  void processHeaderInfo(VXIMap *streamInfo);
  void setValidatorInfo(VXIMap *streamInfo, time_t requestTime);
  void setValidatorInfo(VXIMap *streamInfo, time_t requestTime, const VXIulong sizeBytes);

  bool handleRedirect(VXIMap *streamInfo);
  void sendCookies();

  VXIinetResult MapError(int ht_error, const VXIchar **errorDesc);


  VXIinetResult getChunkSize(VXIulong& chunkSize);
  int readChunked(/* [OUT] */ VXIbyte*         buffer,
                  /* [IN]  */ VXIulong         buflen);

  int readNormal(/* [OUT] */ VXIbyte*         buffer,
                 /* [IN]  */ VXIulong         buflen);

 private:
  VXIinetResult initSocket(SubmitMethod method,
                           const VXIMap *properties,
                           VXIMap *streamInfo);

  void writeDebugTimeStamp();

  int getValue(char *&value, unsigned long mask);
  int getHeader(char *&header, char delimiter);
  int getHeaderValue(char *&header, char *&value);

  int _HttpStatus;

  // The number of bytes to read in the current chunk if _chunked is true or
  // in the stream if _chunked is falsed.
  VXIulong _leftToRead;
  bool _chunked;

  SubmitMethod _method;

  SBinetChannel* _channel;

  SBinetHttpConnection *_connection;
  SWIinputStream *_inputStream;
  SWIoutputStream *_outputStream;

  VXIint32 _closeConnection;
  VXIint32 _connectionAborted;

  static HeaderInfo headerInfoMap[];
  static HeaderInfo* findHeaderInfo(const char *header);

  VXIinetResult waitStreamReady();
  EncodingType getEncodingType(const VXIMap *properties);

  VXIinetResult skipBody();

  SBinetValidator* _validator;
  SBinetValidator* _cacheValidator;
};

#endif
