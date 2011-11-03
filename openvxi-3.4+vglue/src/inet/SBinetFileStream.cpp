
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

#include "SBinetFileStream.hpp"
#include "SBinetValidator.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
typedef struct _stat VXIStatStruct;
#define VXIStat(a,b) _stat(a,b)
#else
typedef struct stat  VXIStatStruct;
#define VXIStat(a,b) stat(a,b)
#endif

VXIinetResult
SBinetFileStream::Open(VXIint32                  flags,
		       const VXIMap             *properties,
		       VXIMap                   *streamInfo)
{
  VXIinetResult returnValue = VXIinet_RESULT_SUCCESS;

  /*
     Get the flag to determine whether to open local files. Also check
     the validator to see if it contains INET_INFO_VALIDATOR and if it
     does contain the modified time. */
  SBinetValidator validator(GetLog(), GetDiagBase());
  const VXIValue *validatorVal = NULL;
  if (properties)
  {
    const VXIValue *val = VXIMapGetProperty(properties, INET_OPEN_LOCAL_FILE);
    if (val != NULL)
    {
      if (VXIValueGetType(val) == VALUE_INTEGER)
      {
        if (VXIIntegerValue((const VXIInteger *) val) == FALSE)
          returnValue = VXIinet_RESULT_LOCAL_FILE;
      }
      else
      {
        Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetFileStream::Open",
             L"Passed invalid type for INET_OPEN_LOCAL_FILE property");
      }
    }

    // SBinetValidator::Create() logs errors for us
    validatorVal = VXIMapGetProperty(properties, INET_OPEN_IF_MODIFIED);
    if((validatorVal != NULL) &&
       (validator.Create(validatorVal) != VXIinet_RESULT_SUCCESS))
      validatorVal = NULL;
  }

  Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetFileStream::Open",
       _url->getAbsolute( ));

  // Stat the file to make sure it exists and to get the length and
  // last modified time
  VXIStatStruct statInfo;
  if (VXIStat(SBinetNString(_url->getPath()).c_str(), &statInfo) != 0) {
    Error(222, L"%s%s", L"File", _url->getPath());
    return(VXIinet_RESULT_NOT_FOUND);
  }

  _content_length = statInfo.st_size;

  // If there is a conditional open using a validator, see if the
  // validator is still valid (no need to re-read and re-process)
  if (validatorVal)
  {
    if (! validator.isModified(statInfo.st_mtime, statInfo.st_size))
    {
      Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetFileStream::Open",
           L"Validator OK, returning NOT_MODIFIED");
      returnValue = VXIinet_RESULT_NOT_MODIFIED;
    }
    else
    {
      Diag(MODULE_SBINET_STREAM_TAGID, L"SBinetFileStream::Open",
           L"Validator modified, must re-fetch");
    }
  }

  if (returnValue != VXIinet_RESULT_LOCAL_FILE)
  {
    // Use fopen for now. Note: Open support reads for now
    _pFile = ::fopen( SBinetNString(_url->getPath()).c_str(), "rb" );
    if(!_pFile){
      Error(223, L"%s%s", L"File", _url->getPath());
      return(VXIinet_RESULT_NOT_FOUND);
    }
    _ReadSoFar = 0;
  }

  if (streamInfo != NULL)
  {
    // Set FILE data source information   
    VXIMapSetProperty(streamInfo, INET_INFO_DATA_SOURCE, 
                     (VXIValue*) VXIIntegerCreate(INET_DATA_SOURCE_FILE));
    
    // Use extension mapping algorithm to determine MIME content type
    VXIString *guessContent = _url->getContentTypeFromUrl();
    if (guessContent == NULL) {
      guessContent = VXIStringCreate(_channel->getDefaultMimeType());
      // This error message is useful, but unfortunately it gets printed
      // for all the grammar files etc. whose extensions are unknown...
      //Error (303, L"%s%s", L"URL", _url->getPath());
    }
    VXIMapSetProperty(streamInfo, INET_INFO_MIME_TYPE, (VXIValue*) guessContent);

    // Set the absolute path, any file:// prefix and host name is
    // already stripped prior to this method being invoked
    VXIMapSetProperty(streamInfo, INET_INFO_ABSOLUTE_NAME,
		      (VXIValue*)VXIStringCreate( _url->getPath() ));

    // Set the validator property
    VXIValue *newValidator = NULL;
    if (returnValue == VXIinet_RESULT_NOT_MODIFIED) {
      newValidator = VXIValueClone(validatorVal);
    } else {
      SBinetValidator validator(GetLog(), GetDiagBase());
      if (validator.Create(_url->getPath(), statInfo.st_size,
                           statInfo.st_mtime) == VXIinet_RESULT_SUCCESS)
      {
	newValidator = (VXIValue *) validator.serialize();
      }
    }

    if (newValidator)
      VXIMapSetProperty(streamInfo, INET_INFO_VALIDATOR, newValidator);
    else {
      Error(103, NULL);
      returnValue = VXIinet_RESULT_OUT_OF_MEMORY;
    }

    // Set the size
    VXIMapSetProperty(streamInfo, INET_INFO_SIZE_BYTES,
		      (VXIValue*)VXIIntegerCreate( _content_length));
  }

  if (!validatorVal && (returnValue == VXIinet_RESULT_NOT_MODIFIED))
    return VXIinet_RESULT_SUCCESS;

  return returnValue;
}


//
// Read is the same for all File requests (GET and POST)
//
VXIinetResult
SBinetFileStream::Read(VXIbyte                 *buffer,
		       VXIulong                 buflen,
		       VXIulong                *nread)
{
  if(!buffer || !nread)
  {
    Error(200, L"%s%s", L"Operation", L"Read");
    return(VXIinet_RESULT_INVALID_ARGUMENT);
  }

  if(!_pFile) return(VXIinet_RESULT_FAILURE);
  int toRead = buflen;
  /*
   * Note: don't try to remember hos many byte are left in file and
   *   set toRead based on this, or you will never get EOF
   */

  int n = ::fread(buffer,sizeof(VXIbyte),toRead,_pFile);
  if(n <= 0)
  {
    // Check for EOF, otherwise ERROR
    //    printf("read %d eof = %d\n",n,feof(_pFile));
    if(feof(_pFile))
    {
      *nread = 0;
      return VXIinet_RESULT_END_OF_STREAM;
    }

    Close();
    Error(224, L"%s%s", L"File", _url->getPath());
    return VXIinet_RESULT_FAILURE;
  }

  *nread = n;
  if( feof(_pFile))
  {
    return VXIinet_RESULT_END_OF_STREAM;
  }

  return VXIinet_RESULT_SUCCESS;
}

VXIinetResult SBinetFileStream::Write(const VXIbyte*   pBuffer,
                                      VXIulong         nBuflen,
                                      VXIulong*        pnWritten)
{
  return VXIinet_RESULT_UNSUPPORTED;
}

VXIinetResult SBinetFileStream::Close()
{
  /* Clean up the request */
  if(_pFile)
  {
    ::fclose(_pFile);
    _pFile = NULL;
    return VXIinet_RESULT_SUCCESS;
  }
  else
  {
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }
}

SBinetFileStream::SBinetFileStream(SBinetURL* url,
				   SBinetChannel* ch,
				   VXIlogInterface *log,
				   VXIunsigned diagLogBase):
  SBinetStream(url, SBinetStream_FILE, log, diagLogBase), _channel(ch)
{
  _pFile = NULL;
  _content_length = 0;
  _ReadSoFar = 0;
}

SBinetFileStream::~SBinetFileStream()
{
  Close();
}
