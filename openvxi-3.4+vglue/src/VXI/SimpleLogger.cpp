
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

#include "SimpleLogger.hpp"
#include "VXIlog.h"
#include <sstream>

static unsigned int MESSAGE_BASE;
static const VXIchar * const MODULE_NAME  = COMPANY_DOMAIN L".vxi";

// ------*---------*---------*---------*---------*---------*---------*---------

static const VXIchar * const TAG_ENCODING  = L"encoding";
static const VXIchar * const TAG_EXCEPTION = L"exception";
static const VXIchar * const TAG_MESSAGE   = L"message";
static const VXIchar * const TAG_RECORDING = L"recording";

static const VXIchar * const TAG_LABEL     = L"label";
static const VXIchar * const TAG_EXPR      = L"expr";
static const VXIchar * const TAG_CONTENT   = L"content";

inline const VXIchar * const GetInfoTagText(SimpleLogger::InfoTag t)
{
  switch (t) {
  case SimpleLogger::ENCODING:    return TAG_ENCODING;
  case SimpleLogger::EXCEPTION:   return TAG_EXCEPTION;
  case SimpleLogger::MESSAGE:     return TAG_MESSAGE;
  case SimpleLogger::RECORDING:   return TAG_RECORDING;
  default:                        return TAG_MESSAGE;
  }
}

// ------*---------*---------*---------*---------*---------*---------*---------

class SimpleLoggerImpl : public SimpleLogger {
public:
  SimpleLoggerImpl(VXIlogInterface * l) : log(l)
  {
    moduleName = MODULE_NAME;
    isVectorSupported = LOG_EVENT_VECTOR_SUPPORTED(l);
    isContentSupported = LOG_CONTENT_METHODS_SUPPORTED(l);
  }

  ~SimpleLoggerImpl() { }
  
  virtual void SetUri(const std::basic_string<wchar_t> & uri) 
  { mUri = uri; }
  
  // Diagnostic...
  typedef unsigned int TAGID;

  virtual bool IsLogging(TAGID tagID) const
  { return (log->DiagnosticIsEnabled(log, MESSAGE_BASE + tagID) == TRUE); }

  virtual std::basic_ostream<wchar_t> & StartDiagnostic(TAGID tagID) const
  { 
    id = tagID;
    buffer.str(L"");
    return buffer; }

  virtual void EndDiagnostic() const
  { buffer << std::ends;
    if (buffer.str().empty()) return;
    log->Diagnostic(log, MESSAGE_BASE + id, moduleName.c_str(), L"%s", buffer.str().c_str()); }

  virtual void LogDiagnostic(TAGID tagID, const wchar_t * text) const
  { log->Diagnostic(log, MESSAGE_BASE + tagID, moduleName.c_str() , L"%s", text); }
    
  virtual void LogDiagnostic(TAGID tagID, const wchar_t* subtag,
                             const wchar_t * format, ...)
  {
    va_list args;
    va_start(args, format);
    log->VDiagnostic(log, MESSAGE_BASE + tagID, subtag, format, args);
    va_end(args);  
  }

  // Error

  virtual void LogError(int errorNum,
                        const wchar_t * key, const wchar_t * txt) const
  { 
    if( mUri.empty() )
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s", key, txt); 
    else
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s%s%s", key, txt, L"URL", mUri.c_str());       
  }

  virtual void LogError(int errorNum,
                        SimpleLogger::InfoTag i1, const wchar_t * txt1,
                        SimpleLogger::InfoTag i2, const wchar_t * txt2) const
  { 
    if( mUri.empty() )
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s%s%s", 
               GetInfoTagText(i1), txt1, GetInfoTagText(i2), txt2); 
    else
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s%s%s%s%s", 
               GetInfoTagText(i1), txt1, GetInfoTagText(i2), txt2, L"URL", mUri.c_str()); 
  }

  virtual void LogError(int errorNum,
                        SimpleLogger::InfoTag i, const wchar_t * txt) const
  { 
    if( mUri.empty() )
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s", GetInfoTagText(i), txt); 
    else
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s%s%s", GetInfoTagText(i), txt, L"URL", mUri.c_str()); 
  }

  virtual void LogError(int errorNum) const
  { 
    if( mUri.empty() )
      log->Error(log, moduleName.c_str(), errorNum, NULL); 
    else
      log->Error(log, moduleName.c_str(), errorNum, L"%s%s", L"URL", mUri.c_str()); 
  }

  // Event
  virtual void LogEvent(int eventNum, const VXIVector * keys,
                        const VXIVector * values) const
  { if (isVectorSupported)
      log->EventVector(log, eventNum, keys, values); }

  // Content logging.
  virtual bool LogContent(const VXIchar *mimetype, 
			  const VXIbyte *content, 
			  VXIulong size, 
			  VXIString **key, 
			  VXIString **value) const;

  // Set Module Name
  virtual void SetModuleName(const wchar_t* modulename) 
  { if( modulename ) moduleName = modulename; }

private:
  VXIlogInterface * log;
  mutable TAGID id;
  mutable std::basic_string<wchar_t> mUri;
  mutable std::basic_ostringstream<wchar_t> buffer;
  mutable std::basic_string<wchar_t> moduleName;
  
  bool isVectorSupported;
  bool isContentSupported;
};


bool SimpleLoggerImpl::LogContent(const VXIchar *mimetype, 
				  const VXIbyte *content, 
				  VXIulong size, 
				  VXIString **key, 
				  VXIString **value) const 
{
  VXIlogResult ret = VXIlog_RESULT_SUCCESS;
  *key = NULL;
  *value = NULL;
  
  // Only dump the page if content logging is supported by the log
  // implementation
  if (isContentSupported) {
    VXIlogStream *stream = NULL;
    ret = log->ContentOpen(log, MODULE_NAME, mimetype, key, value, &stream);
    if (ret == VXIlog_RESULT_SUCCESS) {
      VXIulong nwrite = 0;
      ret = log->ContentWrite(log, content, size, &nwrite, stream);
      log->ContentClose(log, &stream);
    }
    
    if (ret != VXIlog_RESULT_SUCCESS)
      LogError(990);
  } else {
    ret = VXIlog_RESULT_FAILURE;
  }
  
  return (ret == VXIlog_RESULT_SUCCESS ? true : false);
}


SimpleLogger * SimpleLogger::CreateResource(VXIlogInterface * l)
{
  return new SimpleLoggerImpl(l);
}


void SimpleLogger::DestroyResource(SimpleLogger * & log)
{
  if (log == NULL) return;
  delete log;
  log = NULL;
}


void SimpleLogger::SetMessageBase(unsigned int base)
{
  MESSAGE_BASE = base;
}
