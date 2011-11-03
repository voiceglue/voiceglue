
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

#if _MSC_VER >= 1100    // Visual C++ 5.x
#pragma warning( disable : 4786 4503 )
#endif

#include "SBinetTimedStream.hpp"
#include "SBinetStoppableStream.hpp"
#include "SBinetUtils.hpp"
#include "SBinetChannel.h"

// SBinetTimedStream::SBinetTimedStream
// Refer to SBinetTimedStream.hpp for doc.
SBinetTimedStream::SBinetTimedStream(SBinetStoppableStream *aStream,
                                     VXIlogInterface *log,
                                     VXIunsigned diagLogBase):
  SWIutilLogger(MODULE_SBINET, log, diagLogBase),
  _stream(aStream),
  _timeoutOpen(-1),
  _timeoutIO(-1),
  _timeoutDownload(-1),
  _finalTime(NULL)
{}

// SBinetTimedStream::~SBinetTimedStream
// Refer to SBinetTimedStream.hpp for doc.
SBinetTimedStream::~SBinetTimedStream()
{
  delete _finalTime;
  delete _stream;
}

VXIinetResult SBinetTimedStream::Open(VXIint flags,
                                      const VXIMap* properties,
                                      VXIMap* streamInfo)
{

  // For Cache Stream, allow only one channel to access the cache & validator at 
  // a time to avoid race-condition.  Must lock here to avoid time-out in case other
  // channel holds the lock just a bit longer. 
  const VXIchar* fnname = L"SBinetTimedStream::Open";
  VXItrdMutexRef* elock = NULL;

  if( _stream->getType() == SBinetStream_CACHE ) {
    elock = SBinetChannel::acquireEntryLock(_stream->getURL()->getAbsolute(), this);
  }
  
  if(!SBinetUtils::getInteger(properties,
                              INET_TIMEOUT_DOWNLOAD,
                              _timeoutDownload))
  {
    //_timeoutDownload = INET_TIMEOUT_DOWNLOAD_DEFAULT;
    _timeoutDownload = SBinetChannel::getPageLoadTimeout();
  }

  delete _finalTime;

  if (_timeoutDownload < 0)
  {
    _finalTime = NULL;
  }
  else
  {
    _finalTime = new SWITimeStamp;
    _finalTime->setTimeStamp();
    _finalTime->addDelay(_timeoutDownload);
  }
  setDelay(_timeoutOpen);

  VXIinetResult rc = _stream->Open(flags, properties, streamInfo);
  if (rc == VXIinet_RESULT_FETCH_TIMEOUT)
    Error(236, L"%s%i", L"Timeout", _timeoutDownload);

  _stream->setTimeOut(NULL);
  
  // Release the entry lock to allow other channel to gain access
  if( _stream->getType() == SBinetStream_CACHE ) {
    SBinetChannel::releaseEntryLock(elock, this);
    // Sometimes it may take a bit longer to open the cache sucessfully
    // that may result in timeout for read, but it is not correct, so reset it
    if( hasTimedOut() ) {
      _finalTime->setTimeStamp();
      _finalTime->addDelay(_timeoutDownload);        
    }    
  } 
  return rc;
}


VXIinetResult SBinetTimedStream::Read(/* [OUT] */ VXIbyte*         pBuffer,
                                      /* [IN]  */ VXIulong         nBuflen,
                                      /* [OUT] */ VXIulong*        pnRead )
{
  setDelay(_timeoutIO);
  VXIinetResult rc =  _stream->Read(pBuffer, nBuflen, pnRead);

  if (rc == VXIinet_RESULT_FETCH_TIMEOUT)
    Error(237, L"%s%i", L"Timeout", _timeoutDownload);

  _stream->setTimeOut(NULL);
  return rc;
}


VXIinetResult SBinetTimedStream::Write(/* [IN]  */ const VXIbyte*   pBuffer,
                                       /* [IN]  */ VXIulong         nBuflen,
                                       /* [OUT] */ VXIulong*        pnWritten)
{
  setDelay(_timeoutIO);
  VXIinetResult rc =  _stream->Write(pBuffer, nBuflen, pnWritten);

  if (rc == VXIinet_RESULT_FETCH_TIMEOUT)
    Error(238, L"%s%i", L"Timeout", _timeoutDownload);

  _stream->setTimeOut(NULL);
  return rc;
}


VXIinetResult SBinetTimedStream::Close()
{
  return _stream->Close();
}

void SBinetTimedStream::setDelay(int timeoutFromNow)
{
  if (timeoutFromNow < 0)
  {
    _stream->setTimeOut(_finalTime);
  }
  else
  {
    SWITimeStamp expirationTime;
    expirationTime.setTimeStamp();
    expirationTime.addDelay(timeoutFromNow);

    if (_finalTime == NULL ||
        _finalTime->compare(expirationTime) > 0)
      _stream->setTimeOut(&expirationTime);
    else
      _stream->setTimeOut(_finalTime);
  }
}

bool SBinetTimedStream::hasTimedOut() const
{
  if( !_finalTime ) return false;
  if (_finalTime->getSecs() == (time_t) 0)
    return false;

  SWITimeStamp now;
  now.setTimeStamp();

  return _finalTime->compare(now) <= 0;
}
