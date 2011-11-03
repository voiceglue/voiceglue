
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#define SBLOGLISTENERS_EXPORTS
#include "SBlogListeners.h"

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>                // for STL string and wstring
#include <sstream>               // for STL basic_ostringstream( )
#include <limits.h>
#include <cerrno>                // For errno to report fopen( ) errors
#include <cstring>

#include <SBlog.h>
#include <VXItrd.h>
#include <VXItypes.h>
#include <SBlogMapper.h>

#include "SBlogOSUtils.h"

#include <vglue_ipc.h>

#ifdef WIN32
namespace std { };
using namespace std;
#define localtime_r(clock, result) \
        ((result)->tm_sec = 0, localtime(clock))
#endif

static const VXIunsigned MAX_LOG_BUFFER  = 4096;
#ifndef MAX_PATH
#define MAX_PATH        1024
#endif

#define SBLOG_ERR_OUT_OF_MEMORY 100, L"SBlogListeners: Out of memory", NULL
#define SBLOG_ERR_MUTEX_CREATE 101, L"SBlogListeners: Mutex create failed", NULL
#define SBLOG_ERR_MUTEX_LOCK 102, L"SBlogListeners: Mutex lock failed", NULL
#define SBLOG_ERR_MUTEX_UNLOCK 103, L"SBlogListeners: Mutex unlock failed", NULL
#define SBLOG_ERR_ALREADY_INITIALIZED 104, L"SBlogListeners: Not initialized", NULL
#define SBLOG_ERR_NOT_INITIALIZED 105, L"SBlogListeners: Not initialized", NULL

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif
#define MODULE_NAME MODULE_PREFIX     L"SBlog"


static VXIbool gblLogPerformanceCounter = TRUE;

/**********************************************************************/
// Internal class definitions
/**********************************************************************/
static const VXIchar KEY_SEP     = L'=';
static const char    N_KEY_SEP   =  '=';
static const VXIchar ENTRY_SEP   = L'|';
static const char    N_ENTRY_SEP =  '|';
static const VXIchar ESCAPE      = L'\\';
static const char    N_ESCAPE    =  '\\';

class SBlogLogEntry{
public:
  enum LogEscapes {
    ESCAPES_NONE,
    ESCAPES_STANDARD,
    ESCAPES_OSR_EVNT_LOG,
  };

public:
  SBlogLogEntry(LogEscapes escapes = ESCAPES_STANDARD);
  SBlogLogEntry(time_t, VXIunsigned);
  ~SBlogLogEntry() {}

  void SetEscapes(LogEscapes escapes) { escapes_ = escapes; }

  VXIlogResult Append(const VXIchar*, VXIunsigned n);
  VXIlogResult Append(const VXIVector *keys,
          const VXIVector *values);

  void operator+= (const VXIchar *val) { Append(val, 0); }
  void operator+= (const VXIchar val) {
    if (cur_size_ < MAX_LOG_BUFFER) {
      entry_[cur_size_++] =  val;
    }
    entry_[cur_size_]=L'\0';
    return ;
  }
  void operator+= (const char*);
  void operator+= (long int);
  void operator+= (VXIint32);
  void operator+= (VXIunsigned);
  void operator+= (VXIflt32);
  void operator+= (VXIptr);
  void operator+= (const VXIValue *);
  void operator+= (VXIulong);
  void operator+= (VXIflt64);

  void AddKeySep() {
    (*this) += KEY_SEP;
  }
  void AddEntrySep() {
    (*this) += ENTRY_SEP;
  }

  void Terminate(bool addCpuTimes = true, bool addNewline = true);

  const wchar_t *Entry()const {return entry_;}
  
  VXIunsigned size() const {
    return cur_size_;
  }

protected:
  LogEscapes escapes_;
  unsigned int cur_size_;
  wchar_t entry_[MAX_LOG_BUFFER+1];
};


class SBlogLogStream {
private:
  enum LogFormat {
    FORMAT_TEXT_LATIN1 = 0,
    FORMAT_TEXT_UTF8
  };

public:
  SBlogLogStream();
  virtual ~SBlogLogStream() {}
  VXIlogResult Init(
        const VXIchar   *filename,
        const VXIchar   *fileMimeType,
        VXIint32         maxLogSizeBytes,
        const VXIchar   *logContentDir,
        VXIint32         maxContentDirSize,
        VXIbool          logToStdout,
        VXIbool          keepLogFileOpen,
        VXIbool          reportErrorText,
        const VXIVector *errorMapFiles);
  VXIlogResult ShutDown();
  VXIlogResult WriteEntry(char level,
			  const SBlogLogEntry& entry, 
			  VXIbool logToStdout = TRUE);
  VXIlogResult CheckOpen();
  VXIlogResult Open(VXIunsigned nextblock_size);
  VXIlogResult Close();
  VXIbool Enabled() {
    if ((logfilename_ == NULL) && (! logToStdout_))
      return FALSE;
    return TRUE;
  }
  VXIbool ReportErrorText( ) { return reportErrorText_; }
  const VXIchar *GetErrorText(VXIunsigned errorID, const VXIchar *moduleName,
            const VXIchar **severity);
  const VXIchar *GetContentDir(void) const { return logContentDir_; }
  const char *GetNContentDir(void) const { return narrowLogContentDir_; }
  const VXIint32 GetContentDirMaxSize(void) const { return maxContentDirSize_;}

  // ----- Internal error logging functions
  static VXIlogResult GlobalError(VXIunsigned errorID, 
          const VXIchar *errorIDText,
          const VXIchar *format, ...);

protected:
  VXItrdResult Lock() { 
    VXItrdResult rc = VXItrdMutexLock(loglock_);
    if (rc != VXItrd_RESULT_SUCCESS) GlobalError(SBLOG_ERR_MUTEX_LOCK);
    return rc;
  }
  VXItrdResult Unlock() { 
    VXItrdResult rc = VXItrdMutexUnlock(loglock_); 
    if (rc != VXItrd_RESULT_SUCCESS) GlobalError(SBLOG_ERR_MUTEX_UNLOCK);
    return rc;
  }

private:
  SBlogErrorMapper *errorMapper_;
  VXIchar *logfilename_;
  VXIchar *logContentDir_;
  char narrowlogfilename_[MAX_PATH];
  char narrowLogContentDir_[MAX_PATH];
  FILE *logFp_;
  VXIint32 currentSize_;
  VXIint32 maxLogSizeBytes_;
  VXIint32 maxContentDirSize_;
  LogFormat logFormat_;
  VXIbool logToStdout_;
  VXIbool keepLogFileOpen_;
  VXIbool reportErrorText_;
  VXItrdMutex *loglock_;
  VXItrdMutex *internalErrorLoggingLock_;
  VXIthreadID internalErrorLoggingThread_;
};

// ContentStream class
class SBlogContentStream : public SBlogStream {
public:
  static const char PATH_SEPARATOR;

public:
  SBlogContentStream();
  virtual ~SBlogContentStream();

  VXIlogResult Open(const VXIchar *moduleName, 
        const VXIchar *contentType,
        const char *baseDir,
        VXIint channel);

  const VXIchar *GetPath(void) const { return wpath_.c_str(); }
    
private:
  static VXIlogResult CallClose(struct SBlogStream  **stream);
  static VXIlogResult CallWrite(struct SBlogStream *stream, 
        const VXIbyte *buffer,
        VXIulong buflen,
        VXIulong *nwritten);
  
  VXIlogResult Close(void);
  VXIlogResult Write(const VXIbyte *buffer, VXIulong buflen,
         VXIulong *nwritten);

  VXIlogResult CreatePath(const VXIchar *moduleName, 
        const VXIchar *contentType,
        const char *baseDir,
        VXIint channel);
  VXIlogResult CreateDirectories(void);

private:
  FILE *fp_;
  std::string::size_type baseDirLen_;
  std::string npath_;
  std::basic_string<wchar_t> wpath_;
};

// define class static member
#ifdef WIN32
const char SBlogContentStream::PATH_SEPARATOR = '\\';
#else
const char SBlogContentStream::PATH_SEPARATOR = '/';
#endif

// return file extension based on the given mine type
// if the mine type is not recognized, no extension is generated.
static const char* GetExtension(const VXIchar *mimetype)
{
  static const char EXT_ALAW[] = ".alaw";
  static const char EXT_TXT[]  = ".txt";
  static const char EXT_ULAW[] = ".ulaw";
  static const char EXT_VOX[]  = ".vox";
  static const char EXT_WAV[]  = ".wav";
  static const char EXT_XML[]  = ".xml";
  static const char EXT_VXML[] = ".vxml";
  static const char EXT_L8[]   = ".L8";
  static const char EXT_L16[]  = ".L16";
  static const char EXT_UNKNOWN[] = "";

  if (wcscmp(VXI_MIME_TEXT, mimetype) == 0 ||
      wcscmp(VXI_MIME_UNICODE_TEXT, mimetype) == 0 ||
      wcscmp(VXI_MIME_UTF16_TEXT, mimetype) == 0)
  {
    return EXT_TXT;
  }
  else if (wcscmp(VXI_MIME_XML, mimetype) == 0 ||
     wcscmp(VXI_MIME_SSML, mimetype) == 0 ||
     wcscmp(VXI_MIME_SRGS, mimetype) == 0)
  {
    return EXT_XML;
  }
  else if (wcscmp(VXI_MIME_VXML, mimetype) == 0)
  {
    return EXT_VXML;
  }
  else if (wcscmp(VXI_MIME_ALAW, mimetype) == 0)
  {
    return EXT_ALAW;
  }
  else if (wcscmp(VXI_MIME_WAV, mimetype) == 0) 
  {
    return EXT_WAV;
  }
  else if (wcscmp(VXI_MIME_LINEAR, mimetype) == 0)
  {
    return EXT_L8;
  }
  else if (wcscmp(VXI_MIME_LINEAR_16, mimetype) == 0 ||
     wcscmp(VXI_MIME_LINEAR_16_16KHZ, mimetype) == 0)
  {
    return EXT_L16;
  }
  else if (wcscmp(VXI_MIME_ULAW, mimetype) == 0)
  {
    return EXT_ULAW;
  }
  else if (wcscmp(VXI_MIME_VOX, mimetype) == 0)
  {
    return EXT_VOX;
  }
  else
  {
    return EXT_UNKNOWN;  
  }
}

/**********************************************************************/
// SBlogContentStream class implementation
/**********************************************************************/
// c'tor
SBlogContentStream::SBlogContentStream() : 
  SBlogStream(), fp_(NULL), baseDirLen_(0), npath_(), wpath_()
{
  SBlogStream::Close = CallClose;
  SBlogStream::Write = CallWrite;
}

// d'tor
SBlogContentStream::~SBlogContentStream()
{
  if (fp_)
    fclose(fp_);
}

// create path
VXIlogResult SBlogContentStream::CreatePath(const VXIchar *moduleName, 
              const VXIchar *contentType,
              const char *baseDir,
              VXIint channel)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  // Create the content path, with the file name based on a timestamp
  // to millisecond granularity
  std::basic_ostringstream<char> pathStream;
  if (( baseDir ) && ( *baseDir )) {
    baseDirLen_ = strlen (baseDir);
    pathStream << baseDir << PATH_SEPARATOR;
  }

  if (( moduleName ) && ( *moduleName )) {
    for (const VXIchar *ptr = moduleName; *ptr; ptr++) {
      if ((( *ptr >= L'0' ) && ( *ptr <= L'9' )) ||
          (( *ptr >= L'A' ) && ( *ptr <= L'Z' )) ||
          (( *ptr >= L'a' ) && ( *ptr <= L'z' )) ||
          ( *ptr == L'.'  ) || ( *ptr == PATH_SEPARATOR)
          )
        pathStream << static_cast<char>(*ptr);
      else
        pathStream << '_';
    }
  } else {
    pathStream << "unknown_module";
  }
  pathStream << PATH_SEPARATOR;
  
  // Construct the file name
  char logNFileName[32];
  time_t tnow;
  VXIunsigned mSec;
  SBlogGetTime(&tnow, &mSec);
  struct tm *ts, ts_r = { 0 };
  ts = localtime_r(&tnow, &ts_r);
  ts->tm_year = (ts->tm_year < 1900 ? ts->tm_year + 1900 : ts->tm_year); // y2k
  ts->tm_mon++;  // tm_mon is 0 base
  sprintf(logNFileName, "%03d-%d%02d%02d%02d%02d%02d%03d", 
          channel, ts->tm_year, ts->tm_mon, ts->tm_mday,
          ts->tm_hour, ts->tm_min, ts->tm_sec, mSec);

  // Construct the final path
  pathStream << logNFileName << GetExtension(contentType);
  npath_ = pathStream.str();

  // Convert back to wide characters
  for (std::string::size_type i = 0; i < npath_.size( ); i++)
    wpath_ += static_cast<wchar_t>(npath_[i]);

  return rc;
}

// Create the directory tree for the path
VXIlogResult SBlogContentStream::CreateDirectories( ) 
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  // Go through the directories for the relative portion and create them
  std::string::size_type componentLen = baseDirLen_;
  while (componentLen != std::string::npos) {
    std::string tempPath (npath_, 0, componentLen);
    SBlogMkDir (tempPath.c_str());
    componentLen = npath_.find_first_of ("/\\", componentLen + 1);
  }

  return rc;
}

// open content stream
VXIlogResult SBlogContentStream::Open(const VXIchar *moduleName, 
              const VXIchar *contentType,
              const char *baseDir,
              VXIint channel)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  rc = CreatePath(moduleName, contentType, baseDir, channel);
  if ( rc == VXIlog_RESULT_SUCCESS )
    rc = CreateDirectories();
  if ( rc == VXIlog_RESULT_SUCCESS ) {
    fp_ = fopen(npath_.c_str(), "wb");
    if ( fp_ == NULL ) {
      SBlogLogStream::GlobalError(252, 
                  L"SBlogListeners: Content log file open failed", 
      L"%s%S%s%S%s%d", L"file", npath_.c_str(),
      L"errnoStr", strerror(errno), L"errno", errno);
      return VXIlog_RESULT_IO_ERROR;
    }
  }
  return rc;
}

// close content stream
VXIlogResult SBlogContentStream::Close()
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if ( fp_ ) {
    fclose(fp_);
    fp_ = NULL;
  }
  return rc;
}

VXIlogResult SBlogContentStream::CallClose(struct SBlogStream **stream)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if ( stream == NULL || *stream == NULL ) 
    return VXIlog_RESULT_INVALID_ARGUMENT;

  SBlogContentStream *myStream = static_cast<SBlogContentStream *>(*stream);
  rc = myStream->Close();
  delete myStream;
  *stream = NULL;
  return rc;
}

// write content stream
VXIlogResult SBlogContentStream::Write( const VXIbyte *buffer, 
          VXIulong buflen, VXIulong *nwritten)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if ( fp_ == NULL ) return VXIlog_RESULT_FAILURE;
  *nwritten =  fwrite (buffer, 1, buflen, fp_);
  if ( *nwritten == 0 ) {
    if ( ferror (fp_) ) {
      SBlogLogStream::GlobalError(253, 
                  L"SBlogListeners: Content logging file write failed", 
      L"%s%S%s%S%s%d", L"file", npath_.c_str(),
      L"errnoStr", strerror(errno), L"errno", errno);
      rc = VXIlog_RESULT_IO_ERROR;
    } else {
      rc = VXIlog_RESULT_NON_FATAL_ERROR;
    } 
  }
  return rc;
}

VXIlogResult SBlogContentStream::CallWrite(struct SBlogStream *stream, 
             const VXIbyte *buffer,
             VXIulong buflen,
             VXIulong *nwritten)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if ( stream == NULL ) return VXIlog_RESULT_INVALID_ARGUMENT;
  SBlogContentStream *myStream = static_cast<SBlogContentStream *>(stream);
  rc = myStream->Write(buffer, buflen, nwritten);
  return rc;
}

/**********************************************************************/
// globals to the file
/**********************************************************************/
static SBlogLogStream *g_LogStream=NULL;

/**********************************************************************/
// Logging class implementation
/**********************************************************************/
SBlogLogEntry::SBlogLogEntry(LogEscapes escapes)
{
  escapes_ = escapes;
  cur_size_ = 0;
}

SBlogLogEntry::SBlogLogEntry(time_t timestamp, VXIunsigned timestampMsec)
{
  escapes_ = ESCAPES_STANDARD;
  cur_size_ = 0;
  char timestampStr[128];
  if (voiceglue_loglevel() == -1)
  {
      SBlogGetTimeStampStr(timestamp,timestampMsec,timestampStr);
      (*this) += timestampStr;
  };
}

VXIlogResult
SBlogLogEntry::Append (const VXIchar* val, VXIunsigned n)
{
  unsigned int i = 0;
  if (n <= 0)
    n = wcslen(val);

  // strip leading and trailing whitespace
  while ((n > 0) && (SBlogIsSpace(val[n-1]))) n--;
  while ((i < n) && (SBlogIsSpace(val[i]))) i++;

  while((cur_size_ < MAX_LOG_BUFFER) && (i < n)) {
    if (SBlogIsSpace(val[i])) {
      entry_[cur_size_++] = L' ';
    } else {
      bool suppress = false;

      switch(escapes_) {
        case ESCAPES_STANDARD:
          // CC: Remove \\ 
          //if ((val[i] == KEY_SEP) || (val[i] == ENTRY_SEP) || (val[i] == ESCAPE))
          //  entry_[cur_size_++] = ESCAPE;
          break;
        case ESCAPES_OSR_EVNT_LOG:
          if (val[i] == ENTRY_SEP) {
            // In OSR event log, escape for | is putting 2 in a row
            entry_[cur_size_++] = ENTRY_SEP;
          } else if (val[i] == KEY_SEP) {
            // In OSR event log, no escape defined for =
            suppress = true;
            entry_[cur_size_++] = L'-';
            entry_[cur_size_++] = L'>';
          }
          break;
        default:
          break;
      }

      if (! suppress)
        entry_[cur_size_++] = val[i];
    }
    i++;
  }
  entry_[cur_size_]=L'\0';

  return VXIlog_RESULT_SUCCESS;
}

VXIlogResult 
SBlogLogEntry::Append(const VXIVector *keys,
          const VXIVector *values)
{
  if ((! keys) || (! values))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  VXIunsigned index = VXIVectorLength(keys);
  for(VXIunsigned i = 0; i < index; i++) {
    const VXIValue *key = VXIVectorGetElement(keys, i);
    const VXIValue *value = VXIVectorGetElement(values, i);
    if (!key || !value)
      break;

    *this += key;
    AddKeySep();
    *this += value;
    if (i < index - 1) AddEntrySep();
  }
  return VXIlog_RESULT_SUCCESS;
}

void 
SBlogLogEntry::operator+= (const char* val)
{
  unsigned int i = 0;
  unsigned int n = strlen(val);

  // strip leading and trailing whitespace
  while ((n > 0) && (SBlogIsSpace(val[n-1]))) n--;
  while ((i < n) && (SBlogIsSpace(val[i]))) i++;

  while((cur_size_ < MAX_LOG_BUFFER) && (i < n)) {
    if (SBlogIsSpace(val[i])) {
      entry_[cur_size_++] = L' ';
    } else {
      bool suppress = false;

      switch(escapes_) {
        case ESCAPES_STANDARD:
          // CC: Remove \\
          //if ((val[i] == KEY_SEP) || (val[i] == ENTRY_SEP) || (val[i] == ESCAPE))
          //entry_[cur_size_++] = ESCAPE;
          break;
        case ESCAPES_OSR_EVNT_LOG:
          if (val[i] == ENTRY_SEP) {
            // In OSR event log, escape for | is putting 2 in a row
            entry_[cur_size_++] = ENTRY_SEP;
          } else if (val[i] == KEY_SEP) {
            // In OSR event log, no escape defined for =
            suppress = true;
            entry_[cur_size_++] = L'-';
            entry_[cur_size_++] = L'>';
          }
          break;
        default:
          break;
      }

      if (! suppress)
        entry_[cur_size_++] = (VXIchar) val[i];
    }
    i++;
  }
  entry_[cur_size_]=L'\0';
}


void
SBlogLogEntry::operator+= (VXIint32 val)
{
  char temp[128];
  sprintf(temp,"%d",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (long int val)
{
  char temp[128];
  sprintf(temp,"%ld",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (VXIunsigned val)
{
  char temp[128];
  sprintf(temp,"%d",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (VXIflt32 val)
{
  char temp[128];
  sprintf(temp,"%f",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (VXIulong val)
{
  char temp[128];
  sprintf(temp,"%lu",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (VXIflt64 val)
{
  char temp[128];
  sprintf(temp,"%f",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+= (VXIptr val)
{
  char temp[128];
  sprintf(temp,"0x%p",val);
  (*this) += temp;
  return ;
}

void
SBlogLogEntry::operator+=(const VXIValue *value)
{
  switch (VXIValueGetType(value)) {
  case VALUE_STRING:
    (*this) += VXIStringCStr((const VXIString *) value);
    break;

  case VALUE_INTEGER:
    (*this) += VXIIntegerValue((const VXIInteger *) value);
    break;

  case VALUE_FLOAT:
    (*this) += VXIFloatValue((const VXIFloat *) value);
    break;

  case VALUE_DOUBLE:
    (*this) += VXIDoubleValue((const VXIDouble *) value);
    break;

  case VALUE_ULONG:
    (*this) += VXIULongValue((const VXIULong *) value);
    break;

  case VALUE_PTR:
    (*this) += VXIPtrValue((const VXIPtr *) value);
    break;

  default:
    (*this) += L"unsupported type ";
    (*this) += VXIValueGetType(value);
  }
}

void SBlogLogEntry::Terminate(bool addCpuTimes, bool addNewline)
{
  // overwrite addCpuTime param
  addCpuTimes = (gblLogPerformanceCounter == TRUE ? true : false);
  if (addCpuTimes) {
    long userTime;
    long kernelTime;
    SBlogGetCPUTimes(&userTime,&kernelTime);
    this->AddEntrySep();
    (*this) += "UCPU";
    this->AddKeySep();
    (*this) += userTime;
    
    this->AddEntrySep();
    (*this) += "SCPU";
    this->AddKeySep();
    (*this) += kernelTime;
  }
  
  if (addNewline) {
    if (cur_size_ == MAX_LOG_BUFFER)
      cur_size_--;
    entry_[cur_size_++] ='\n';
  }
  entry_[cur_size_] = '\0';
}

SBlogLogStream::SBlogLogStream() :
  errorMapper_(NULL),
  logfilename_(NULL),
  logContentDir_(NULL),
  logFp_(NULL),
  currentSize_(0),
  maxLogSizeBytes_(0),
  maxContentDirSize_(0),
  logFormat_(FORMAT_TEXT_LATIN1),
  logToStdout_(TRUE),
  keepLogFileOpen_(TRUE),
  reportErrorText_(FALSE),
  loglock_(NULL),
  internalErrorLoggingLock_(NULL),
  internalErrorLoggingThread_((VXIthreadID) -1)
{
  narrowlogfilename_[0] = '\0';
  narrowLogContentDir_[0] = '\0';
}

VXIlogResult
SBlogLogStream::Init(
         const VXIchar   *filename,
         const VXIchar   *fileMimeType,
         VXIint32         maxLogSizeBytes,
         const VXIchar   *logContentDir,
         VXIint32         maxContentDirSize,
         VXIbool          logToStdout,
         VXIbool          keepLogFileOpen,
         VXIbool          reportErrorText,
         const VXIVector *errorMapFiles)
{
  int n;
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  // check the inputs
  if (maxLogSizeBytes < 0) {
    GlobalError(152, L"SBlogListeners: Invalid system log file size "
                L"limit configured", L"%s%ld", L"configured",
                (long) maxLogSizeBytes);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  } else if (maxContentDirSize < 0) {
    GlobalError(153, L"SBlogListeners: Invalid content logging file "
                L"size limit configured", L"%s%ld", L"configured",
                (long) maxContentDirSize);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  if ((! fileMimeType) || (! fileMimeType[0]) ||
      (wcscmp(fileMimeType, SBLOG_MIME_TEXT_LATIN1) == 0))
    logFormat_ = FORMAT_TEXT_LATIN1;
  else if (wcscmp(fileMimeType, SBLOG_MIME_TEXT_UTF8) == 0)
    logFormat_ = FORMAT_TEXT_UTF8;
  else {
    GlobalError(150, L"SBlogListeners: Invalid system log file format "
                L"configured", L"%s%s", L"format", fileMimeType);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  maxLogSizeBytes_ = maxLogSizeBytes;
  maxContentDirSize_ = maxContentDirSize;
  logToStdout_ = logToStdout;
  keepLogFileOpen_ = keepLogFileOpen;
  reportErrorText_ = reportErrorText;

  if (logfilename_) {
    delete [] logfilename_;
    logfilename_ = NULL;
  }
  if (logContentDir_) {
    delete [] logContentDir_;
    logContentDir_ = NULL;
  }

  if (filename != NULL) { // write to a file otherwise only stdout
    // copy the filename
    n = wcslen(filename);
    logfilename_ = new VXIchar[n+1];
    if (!logfilename_) {
      GlobalError(SBLOG_ERR_OUT_OF_MEMORY);
      return VXIlog_RESULT_OUT_OF_MEMORY;
    }

    wcscpy(logfilename_, filename);
    
    // convert the file name path
    SBlogWchar2Latin1(logfilename_, narrowlogfilename_, MAX_PATH);

    rc = CheckOpen();
  }

  if (logContentDir != NULL) {
    // copy the filename
    n = wcslen(logContentDir);
    logContentDir_ = new VXIchar[n+1];
    if (!logContentDir_) {
      GlobalError(SBLOG_ERR_OUT_OF_MEMORY);
      return VXIlog_RESULT_OUT_OF_MEMORY;
    }

    wcscpy(logContentDir_, logContentDir);
    
    // convert the file name path
    SBlogWchar2Latin1(logContentDir_, narrowLogContentDir_, MAX_PATH);

    // reate the content directory
    SBlogMkDir (narrowLogContentDir_);
  }

  // Create the mutexes
  if ((VXItrdMutexCreate(&loglock_) != VXItrd_RESULT_SUCCESS) ||
      (VXItrdMutexCreate(&internalErrorLoggingLock_) !=
       VXItrd_RESULT_SUCCESS)) {
    GlobalError(SBLOG_ERR_MUTEX_CREATE);
    rc = VXIlog_RESULT_SYSTEM_ERROR;
  }

  // load the XML error files into a VXIMap member here
  if (errorMapFiles)
  {
    VXIlogResult rc2 = SBlogErrorMapperCreate(errorMapFiles, &errorMapper_);

    // write to log file if unable to display error text
    if (rc2 != VXIlog_RESULT_SUCCESS && Enabled())
    {
      if( rc2 == VXIlog_RESULT_UNSUPPORTED )
        GlobalError(350, L"XML error text lookup feature disabled because the "
                    L"listener is compiled without defining HAVE_XERCES", NULL);
      else
        GlobalError(351, L"XML error text lookup feature disabled due to an "
                    L"error in parsing the configured XML error files", NULL);
    }
  }

  return rc;
}

VXIlogResult
SBlogLogStream::ShutDown()
{
  if (errorMapper_) {
    SBlogErrorMapperDestroy(&errorMapper_);
    errorMapper_ = NULL;
  }

  if (logfilename_) {
    delete [] logfilename_;
    logfilename_ = NULL;
  }

  if (logContentDir_) {
    delete [] logContentDir_;
    logContentDir_ = NULL;
  }

  if (logFp_) {
    fclose(logFp_);
    logFp_ = NULL;
  }

  if ((loglock_) && (VXItrdMutexDestroy(&loglock_) != VXItrd_RESULT_SUCCESS))
    return VXIlog_RESULT_SYSTEM_ERROR;

  if ((internalErrorLoggingLock_) && 
      (VXItrdMutexDestroy(&internalErrorLoggingLock_) != 
       VXItrd_RESULT_SUCCESS))
    return VXIlog_RESULT_SYSTEM_ERROR;

  return VXIlog_RESULT_SUCCESS;
}

/*
  Make sure the current file can be opened - safety during the intialize
  */
VXIlogResult
SBlogLogStream::CheckOpen()
{
    if (voiceglue_loglevel() != -1)
    {
	return VXIlog_RESULT_SUCCESS;
    };
  if (narrowlogfilename_[0] == '\0') {
    GlobalError(154, L"SBlogListeners: Invalid system log file name "
                L"configured", NULL);
    return VXIlog_RESULT_PLATFORM_ERROR;
  }

  // See if the file exists and open it, getting the size
  SBlogFileStats info;
  if (SBlogGetFileStats(narrowlogfilename_, &info) == 0) {
    currentSize_ = info.st_size;
    logFp_ = fopen(narrowlogfilename_, "a");
  } else {
    currentSize_ = 0;
    logFp_ = fopen(narrowlogfilename_, "w");
  }
  if (! logFp_) {
    GlobalError(151, L"SBlogListeners: System log file open failed", 
                L"%s%S%s%S%s%d", L"file", narrowlogfilename_,
                L"errnoStr", strerror(errno), L"errno", errno);
    return VXIlog_RESULT_IO_ERROR;
  }
  if (! keepLogFileOpen_) {
    fclose(logFp_);
    logFp_ = NULL;
  }
  return VXIlog_RESULT_SUCCESS;
}

VXIlogResult
SBlogLogStream::Open(VXIunsigned nextblock_size)
{
    if (voiceglue_loglevel() != -1)
    {
	return VXIlog_RESULT_SUCCESS;
    };
  if (narrowlogfilename_[0] == '\0') {
    GlobalError(154, L"SBlogListeners: Invalid system log file name "
                L"configured", NULL);
    return VXIlog_RESULT_PLATFORM_ERROR;
  }

  if (! logFp_) {
    // See if the file exists and open it, getting the size
    SBlogFileStats info;
    if (SBlogGetFileStats(narrowlogfilename_, &info) == 0) {
      currentSize_ = info.st_size;
    } else {
      currentSize_ = 0;
    }
  }

  if ((currentSize_ + nextblock_size) > (VXIunsigned) maxLogSizeBytes_) {
    if (logFp_) {
      fclose(logFp_);
      logFp_ = NULL;
    }

    currentSize_ = 0;
    char overflowName[MAX_PATH+64];
    strcpy(overflowName,narrowlogfilename_);
    strcat(overflowName,".old");
    remove(overflowName);
    if (rename(narrowlogfilename_,overflowName)) {
      GlobalError(251, L"SBlogListeners: System log file rollover failed",
                  L"%s%S%s%S%s%S%s%d", L"logFile", narrowlogfilename_, 
                  L"rolloverFile", overflowName, L"errnoStr", strerror(errno),
                  L"errno", errno);
      return VXIlog_RESULT_IO_ERROR;
    }
    logFp_ = fopen(narrowlogfilename_, "w");
  } else if (! logFp_) {
    logFp_ = fopen(narrowlogfilename_, "a");
  }
  if (! logFp_) {
    GlobalError(151, L"SBlogListeners: System log file open failed", 
                L"%s%S%s%S%s%d", L"file", narrowlogfilename_,
                L"errnoStr", strerror(errno), L"errno", errno);
    return VXIlog_RESULT_IO_ERROR;
  }
  return VXIlog_RESULT_SUCCESS;
}

VXIlogResult
SBlogLogStream::Close()
{
    if (voiceglue_loglevel() != -1)
    {
	return VXIlog_RESULT_SUCCESS;
    };
  if (!logFp_)
    return VXIlog_RESULT_IO_ERROR;
  if (! keepLogFileOpen_) {
    fclose(logFp_);
    logFp_ = NULL;
  } else {
    fflush(logFp_);
  }
  return VXIlog_RESULT_SUCCESS;
}

VXIlogResult
SBlogLogStream::WriteEntry(char level,
			   const SBlogLogEntry &entry,
			   VXIbool logToStdout)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  unsigned int nFileBytes = 0;
  char latin1OutBuf[MAX_LOG_BUFFER + 1]; // make sure enough room
  char utf8OutBuf[MAX_LOG_BUFFER * 4 + 1];
  const char *fileBuf = NULL;
  latin1OutBuf[0] = 0;
  utf8OutBuf[0] = 0;

  // Exit if there is nothing to write, should never happen
  if (entry.size() == 0)
    return VXIlog_RESULT_FATAL_ERROR;

  if ((logfilename_ != NULL) || (logToStdout && logToStdout_)) {
    // Generate Latin-1 and/or UTF-8 output as required. Note that
    // Latin-1 is used for console output even when the log file
    // format is not Latin-1 because under many operating
    // system/compiler/console combinations wide character output
    // doesn't work right.
    switch (logFormat_) {
    case FORMAT_TEXT_UTF8:
      SBlogWchar2UTF8(entry.Entry(), utf8OutBuf, MAX_LOG_BUFFER * 4,
          &nFileBytes);
      fileBuf = utf8OutBuf;
      break;
    case FORMAT_TEXT_LATIN1:
      SBlogWchar2Latin1(entry.Entry(), latin1OutBuf, MAX_LOG_BUFFER);
      fileBuf = latin1OutBuf;
      nFileBytes = entry.size();
      break;
    default:
      GlobalError(150, L"SBlogListeners: Invalid system log file format "
                  L"configured", NULL);
      return VXIlog_RESULT_FATAL_ERROR;
    }

#ifndef WCHART_CONSOLE_OUTPUT
    if ((logToStdout) && (logToStdout_) && (! latin1OutBuf[0]))
      SBlogWchar2Latin1(entry.Entry(), latin1OutBuf, MAX_LOG_BUFFER);
#endif
  }
  
  if (nFileBytes == 0)
    return VXIlog_RESULT_SUCCESS;

  if (Lock() != VXItrd_RESULT_SUCCESS)
    return VXIlog_RESULT_SYSTEM_ERROR;

  if (logfilename_ != NULL) {
    // proceed to log to stdout if enabled, but return the error code
    rc = Open(entry.size());
    if (rc == VXIlog_RESULT_SUCCESS) {
	int nwrite;
	if (voiceglue_loglevel() != -1)
	{
	    voiceglue_log (level, fileBuf);
	    nwrite = 1;
	}
	else
	{
	    nwrite = fwrite(fileBuf, 1, nFileBytes, logFp_);
	};
      if (nwrite == 0) {
        // should disable logging.
        GlobalError(250, L"SBlogListeners: System log file write failed", 
                    L"%s%S%s%S%s%d", L"file", narrowlogfilename_,
                    L"errnoStr", strerror(errno), L"errno", errno);
        rc = VXIlog_RESULT_IO_ERROR;
      } else {
	  if (voiceglue_loglevel() == -1)
	  {
	      currentSize_ += nwrite;
	      // to ensure we don't lose log lines on a crash/abort
	      fflush(logFp_);
	  };
      }
    }

    VXIlogResult rc2 = Close();
    if (rc2 != VXIlog_RESULT_SUCCESS)
      rc = rc2;
  }

  if ((logToStdout) && (logToStdout_) && (voiceglue_loglevel() == -1)) {
    VXIlogResult rc2 = VXIlog_RESULT_SUCCESS;
#ifdef WCHART_CONSOLE_OUTPUT
    if (fputws(entry.Entry(), stdout) <= 0)
      rc2 = VXIlog_RESULT_IO_ERROR;
#else
    if (fwrite(latin1OutBuf, 1, entry.size(), stdout) <= 0)
      rc2 = VXIlog_RESULT_IO_ERROR;
#endif

    if (fflush(stdout) != 0)
      rc = VXIlog_RESULT_IO_ERROR;

    if (rc2 != VXIlog_RESULT_SUCCESS)
      rc = rc2;
  }

  if (Unlock() != VXItrd_RESULT_SUCCESS)
    rc = VXIlog_RESULT_SYSTEM_ERROR;

  return rc;
}


const VXIchar *
SBlogLogStream::GetErrorText(VXIunsigned errorID, const VXIchar *moduleName,
           const VXIchar **severity)
{
  static const VXIchar *severityStr[4] = {
    L"", L"CRITICAL", L"SEVERE", L"WARNING" };

  VXIint severityNum = -1;
  const VXIchar *errText;
  
  // mapping here
  if (errorMapper_)
    SBlogErrorMapperGetErrorInfo(errorMapper_, errorID, moduleName,
         &errText, &severityNum);
  else
    errText = severityStr[0];

  if ((severityNum >= 1) && (severityNum <= 3))
    *severity = severityStr[severityNum];
  else
    *severity = severityStr[0];
  
  // If the mapping failed, this returned an empty string
  return errText;
}

VXIlogResult SBlogLogStream::GlobalError(VXIunsigned errorID, 
           const VXIchar *errorIDText,
           const VXIchar *format, ...)
{
  va_list args;
  va_start(args, format);
  VXIlogResult rc = SBlogVLogErrorToConsole(MODULE_NAME, errorID, errorIDText,
                                            format, args);
  va_end(args);
  return rc;
}


SBLOGLISTENERS_API void DiagnosticListener
(
  SBlogInterface *pThis,
  VXIunsigned tagID,
  const VXIchar* subtag,
  time_t timestamp,
  VXIunsigned timestampMsec,
  const VXIchar* printmsg,
  void *userdata
)
{
  // casting userdata
  SBlogListenerChannelData *data = (SBlogListenerChannelData *)userdata;
  if ( data == NULL ) return;
  // Exit immediately if no logging enabled
  if (!g_LogStream || !g_LogStream->Enabled())
    return;

  SBlogLogEntry entry(timestamp, timestampMsec);
  entry.AddEntrySep();
  entry += (long) VXItrdThreadGetID();
  // entry.AddEntrySep();
  entry.AddEntrySep();
  entry += data->channel;
  entry.AddEntrySep();
  entry += tagID;
  entry.AddEntrySep();
  entry += subtag;
  entry.AddEntrySep();
  entry += printmsg;
  entry.Terminate();
  g_LogStream->WriteEntry((char) 7, entry);
  return;
}


SBLOGLISTENERS_API void ErrorListener
(
  SBlogInterface *pThis,
  const VXIchar *moduleName,
  VXIunsigned errorID,
  time_t timestamp,
  VXIunsigned timestampMsec,
  const VXIVector *keys,
  const VXIVector *values,
  void *userdata
)
{
  // casting userdata, even if we get NULL for pThis and userdata
  // proceed since we rely on that in SBlogStream::Error() above
  SBlogListenerChannelData *data = (SBlogListenerChannelData *)userdata;

  // Exit immediately if no logging enabled
  if (!g_LogStream || !g_LogStream->Enabled())
    return;

  // Lookup error text
  const VXIchar *errorText = L"", *severity = L"";
  if ( g_LogStream->ReportErrorText( ) )
    errorText = g_LogStream->GetErrorText (errorID, moduleName, &severity);

  SBlogLogEntry entry(timestamp, timestampMsec);
  entry.AddEntrySep();
  entry += (long) VXItrdThreadGetID();
  // entry.AddEntrySep();
  entry.AddEntrySep();
  if (data)
    entry += data->channel;
  else
    entry += -1;
  entry.AddEntrySep();
  entry += severity;
  entry.AddEntrySep();
  entry += moduleName;
  entry.AddEntrySep();
  entry += errorID;
  entry.AddEntrySep();
  entry += errorText;
  entry.AddEntrySep();
  entry.Append(keys, values);
  entry.Terminate();
  g_LogStream->WriteEntry((char) 3, entry);
}


SBLOGLISTENERS_API void EventListener
(
  SBlogInterface *pThis,
  VXIunsigned eventID,
  time_t timestamp,
  VXIunsigned timestampMsec,
  const VXIVector *keys,
  const VXIVector *values,
  void *userdata
)
{
  // casting userdata
  SBlogListenerChannelData *data = (SBlogListenerChannelData *)userdata;
  if ( data == NULL ) return;
  // Exit immediately if no logging enabled
  if (!g_LogStream || !g_LogStream->Enabled())
    return;
  SBlogLogEntry entry(timestamp, timestampMsec);
  entry.AddEntrySep();
  entry += (long) VXItrdThreadGetID();
  entry.AddEntrySep();
  // entry.AddEntrySep();
  entry += data->channel;
  entry.AddEntrySep();
  entry += L"EVENT";
  entry.AddEntrySep();
  entry += eventID;
  entry.AddEntrySep();
  entry.Append(keys, values);
  entry.Terminate();
  g_LogStream->WriteEntry((char) 5, entry, FALSE);
}


SBLOGLISTENERS_API VXIlogResult ContentListener
(
  SBlogInterface *pThis,
  const VXIchar  *moduleName,
  const VXIchar  *contentType,
  void           *userdata,
  VXIString     **logKey,
  VXIString     **logValue,
  SBlogStream   **stream
)
{
  // casting userdata
  SBlogListenerChannelData *data = (SBlogListenerChannelData *)userdata;
  if ( data == NULL ) return VXIlog_RESULT_INVALID_ARGUMENT;
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  *logKey = NULL;
  *logValue = NULL;
  *stream = NULL;

  // Exit immediately if no logging enabled
  if (!g_LogStream || !g_LogStream->Enabled())
    return VXIlog_RESULT_FAILURE;

  const char *baseDir = g_LogStream->GetNContentDir();
  VXIulong maxSize = g_LogStream->GetContentDirMaxSize();

  // only do open if content dir. and size is valid
  if (baseDir && *baseDir != 0 && maxSize > 0 )
  {
    SBlogContentStream *openedStream = new SBlogContentStream;
    rc = openedStream->Open(moduleName, contentType, baseDir, data->channel);
    if ( rc != VXIlog_RESULT_SUCCESS ) {
      delete openedStream;
    } else {  
      // create key & value for cross-reference
      VXIString *logKey_ = VXIStringCreate(L"contentURL");
      VXIString *logValue_ = VXIStringCreate(openedStream->GetPath());
      *logKey = logKey_;
      *logValue = logValue_;
      *stream = static_cast<SBlogStream *>(openedStream);
    }
  } else {
    rc = VXIlog_RESULT_FAILURE;
  }

  return rc;
}


/**********************************************************************/
// Public Interface
/**********************************************************************/

SBLOGLISTENERS_API VXIlogResult SBlogListenerInit
(
 const VXIchar   *filename,
 VXIint32         maxLogSizeBytes,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen
)
{
  return SBlogListenerInitEx2(filename, NULL, maxLogSizeBytes, NULL, 0, 
                              logToStdout, keepLogFileOpen, FALSE, NULL);
}

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx
(
 const VXIchar   *filename,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
)
{
  return SBlogListenerInitEx2(filename, NULL, maxLogSizeBytes, logContentDir,
                              maxContentDirSize, logToStdout, keepLogFileOpen,
                              reportErrorText, errorMapFiles);
}

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx2
(
 const VXIchar   *filename,
 const VXIchar   *fileMimeType,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
)
{
  return SBlogListenerInitEx3(filename, fileMimeType, maxLogSizeBytes, 
                              logContentDir, maxContentDirSize, logToStdout, keepLogFileOpen,
                              TRUE, reportErrorText, errorMapFiles);
}

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx3
(
 const VXIchar   *filename,
 const VXIchar   *fileMimeType,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen,
 VXIbool          logPerformanceCounter,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
)
{
  if (g_LogStream != NULL) {
    SBlogLogStream::GlobalError(SBLOG_ERR_NOT_INITIALIZED);
    return VXIlog_RESULT_NON_FATAL_ERROR;
  }
  g_LogStream = new SBlogLogStream();
  if (!g_LogStream) {
    SBlogLogStream::GlobalError(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
  gblLogPerformanceCounter = logPerformanceCounter;
  return g_LogStream->Init(filename, fileMimeType, maxLogSizeBytes, 
                           logContentDir, maxContentDirSize, logToStdout,
                           keepLogFileOpen, reportErrorText, errorMapFiles);
}

SBLOGLISTENERS_API VXIlogResult SBlogListenerShutDown()
{
  if (g_LogStream == NULL) {
    SBlogLogStream::GlobalError(SBLOG_ERR_NOT_INITIALIZED);
    return VXIlog_RESULT_NON_FATAL_ERROR;
  }
  VXIlogResult ret = g_LogStream->ShutDown();
  if (ret) {
    return ret;
  }
  delete g_LogStream;
  g_LogStream = NULL;
  return ret;
}


SBLOGLISTENERS_API VXIlogResult ControlDiagnosticTag
(
 VXIlogInterface *pThis,
 VXIunsigned tagID,
 VXIbool     state
)
{
  if ( pThis == NULL ) {
    SBlogLogStream::GlobalError(254, 
                           L"SBlogListeners: Invalid argument to API function",
                           L"%s%s", L"function", L"ControlDiagnosticTag");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return( logIntf->ControlDiagnosticTag(logIntf, tagID, state) );
}

SBLOGLISTENERS_API VXIlogResult RegisterErrorListener
(
  VXIlogInterface           *pThis,
  SBlogErrorListener        *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254, 
                           L"SBlogListeners: Invalid argument to API function",
                           L"%s%s", L"function", L"RegisterErrorListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return ( logIntf->RegisterErrorListener(logIntf,alistener,channelData) );
}

SBLOGLISTENERS_API VXIlogResult UnregisterErrorListener
(
  VXIlogInterface           *pThis,
  SBlogErrorListener        *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254, 
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"UnregisterErrorListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return ( logIntf->UnregisterErrorListener(logIntf,alistener,channelData) );

}

SBLOGLISTENERS_API VXIlogResult RegisterDiagnosticListener
(
  VXIlogInterface           *pThis,
  SBlogDiagnosticListener   *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254, 
                L"SBlogListeners: Invalid argument to API function",
                L"%s%s", L"function", L"RegisterDiagnosticListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return (logIntf->RegisterDiagnosticListener(logIntf,alistener,channelData));
}

SBLOGLISTENERS_API VXIlogResult UnregisterDiagnosticListener
(
  VXIlogInterface           *pThis,
  SBlogDiagnosticListener   *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254,
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"UnregisterDiagnosticListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return (logIntf->UnregisterDiagnosticListener(logIntf,alistener,channelData));
}

SBLOGLISTENERS_API VXIlogResult RegisterEventListener
(
  VXIlogInterface           *pThis,
  SBlogEventListener        *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254,
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"RegisterEventListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return(logIntf->RegisterEventListener(logIntf,alistener,channelData));
} 
 
SBLOGLISTENERS_API VXIlogResult UnregisterEventListener
(
  VXIlogInterface           *pThis,
  SBlogEventListener        *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254,
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"UnregisterEventListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return(logIntf->UnregisterEventListener(logIntf,alistener,channelData));  
}

SBLOGLISTENERS_API VXIlogResult RegisterContentListener
(
  VXIlogInterface           *pThis,
  SBlogContentListener      *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254,
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"RegisterContentListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return(logIntf->RegisterContentListener(logIntf,alistener,channelData));
} 
 
SBLOGLISTENERS_API VXIlogResult UnregisterContentListener
(
  VXIlogInterface           *pThis,
  SBlogContentListener      *alistener,
  SBlogListenerChannelData  *channelData
)
{
  if ((pThis == NULL) || (channelData == NULL)) {
    SBlogLogStream::GlobalError(254,
          L"SBlogListeners: Invalid argument to API function",
          L"%s%s", L"function", L"UnregisterContentListener");
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  SBlogInterface *logIntf = (SBlogInterface *)pThis;
  return(logIntf->UnregisterContentListener(logIntf,alistener,channelData));  
}
