
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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#ifdef OPENVXI

#include "OSBinet.h"

#define SBinetInit OSBinetInit
#define SBinetShutDown OSBinetShutDown
#define SBinetCreateResource OSBinetCreateResource
#define SBinetDestroyResource OSBinetDestroyResource

#else

#include "SBinet.h"

#endif

#include "SBlog.h"
#include "VXIinterpreter.h"
#include "VXIvalue.h"
#include <iostream>
#include <string>
#include <sstream>
#include "stdio.h"

#if defined(__GNUC__) && (! defined(_GLIBCPP_USE_WCHAR_T))

namespace std
{
  typedef basic_string<wchar_t> wstring;
}

#define wcout cout
using std::cout;
typedef char myCharType;
#define MY_EMPTY_STR ""

static std::ostream & operator<< (std::ostream & s, const wchar_t *str) {
  if ( str == NULL ) {
    s << "<null>";
  } else {
    unsigned int i, len = wcslen (str);
    for (i = 0; i < len; i++) 
      if ( str[i] & 0xFF00 )
	s << "<%" << (int) str[i] << ">";
      else
	s << (char) str[i];
  }
  return s;
}

static std::ostream & operator<< (std::ostream & s, const std::wstring &str) {
  s << str.c_str();
  return s;
}

#else  /* NOT defined(__GNUC__) && (! defined(_GLIBCPP_USE_WCHAR_T)) */

using std::wcout;
typedef VXIchar myCharType;
#define MY_EMPTY_STR L""

#endif /* defined(__GNUC__) && (! defined(_GLIBCPP_USE_WCHAR_T)) */

using std::endl;

const int ID_INTERPRETER = 8000;
const int ID_INET        = 2000;

static std::basic_ostringstream<myCharType> gblOs;

// ------*---------*---------*---------*---------*---------*---------*---------

static std::basic_ostream<myCharType>& 
operator<<(std::basic_ostream<myCharType>& os,
	   const VXIValue * & val)
{
  if (val == NULL)
    return os << L"NULL";
  VXIchar tmp[256];
  
  switch (VXIValueGetType(val)) {
  case VALUE_INTEGER:
    return os << VXIIntegerValue(reinterpret_cast<const VXIInteger*>(val));
  case VALUE_FLOAT: {
#ifdef WIN32
    _snwprintf(tmp, 256, L"%f", VXIFloatValue(reinterpret_cast<const VXIFloat*>(val)));
#else
    swprintf(tmp, 256, L"%f", VXIFloatValue(reinterpret_cast<const VXIFloat*>(val)));
#endif
    return os << tmp;
  }
#if (VXI_CURRENT_VERSION >= 0x00030000)
  case VALUE_DOUBLE:
#ifdef WIN32
    _snwprintf(tmp, 256, L"%f", VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val)));
#else
    swprintf(tmp, 256, L"%f", VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val)));
#endif
    return os << tmp;
  case VALUE_ULONG:
    return os << VXIULongValue(reinterpret_cast<const VXIULong*>(val));
#endif
  case VALUE_STRING:
    return os << VXIStringCStr(reinterpret_cast<const VXIString *>(val));
  case VALUE_VECTOR:
    {
      const VXIVector * v = reinterpret_cast<const VXIVector *>(val);
      const VXIunsigned len = VXIVectorLength(v);

      os << L"{ ";
      for (VXIunsigned i = 0; i < len; ++i) {
        if (i != 0) os << L", ";
        os << VXIVectorGetElement(v, i);
      }
      return os << L" }";
    }
  case VALUE_MAP:
    {
      const VXIMap * m = reinterpret_cast<const VXIMap *>(val);
      const VXIchar * key;
      const VXIValue * value;

      os << L"{ ";
      if (VXIMapNumProperties(m) != 0) {
        VXIMapIterator * i = VXIMapGetFirstProperty(m, &key, &value);
        os << L"(" << key << L", " << value << L")";
        
        while (VXIMapGetNextProperty(i, &key, &value)
               == VXIvalue_RESULT_SUCCESS)
          os << L", (" << key << L", " << value << L")";

        VXIMapIteratorDestroy(&i);
      }
      return os << L" }";
    }
  default:
    break;
  }

  return os << L"(unprintable result)";
}


void LoggerClear()
{
  gblOs.str(MY_EMPTY_STR);
}


void LoggerPrint()
{
  wcout << gblOs.str();
  wcout.flush();
}


void LoggerErrorListener(SBlogInterface *,
                         const VXIchar *moduleName,
                         VXIunsigned errorID,
                         time_t,
			 VXIunsigned,
                         const VXIVector *keys,
                         const VXIVector *values,
                         void *)
{
  if (!gblOs.good()) return;
  gblOs << L"Error (module='" << moduleName << L"' id='" << errorID;

  VXIunsigned len = VXIVectorLength(keys);
  if (len != VXIVectorLength(values) || len == 0) {
    gblOs << L"')" << endl;
  }
  else {
    gblOs << L"') : ";
    for (VXIunsigned i = 0; i < len; ++i) {
      const VXIValue * key = VXIVectorGetElement(keys, i);
      const VXIValue * val = VXIVectorGetElement(values, i);
      gblOs << key << L"='" << val << L"' ";
    }
    gblOs << endl;
  }
}


void LoggerDiagnosticListener(SBlogInterface *,
                             VXIunsigned,
                             const VXIchar *,
                             time_t,
			     VXIunsigned,
                             const VXIchar* printmsg,
                             void *)
{
  if (!gblOs.good()) return;
  if (printmsg == NULL) printmsg = L"";

  gblOs << L"Info: " << printmsg << endl;
}

// ------*---------*---------*---------*---------*---------*---------*---------

void Expand(std::wstring & output, const char * const input)
{
  output.erase();
  if (input == NULL || *input == '\0') return;

  for (const char * c = input; *c != '\0'; ++c)
    output += *c;
}

#ifdef WIN32
#include "Windows.h"
#else
  typedef unsigned long DWORD;
  #define GetTickCount clock
#endif

class ElapsedTime
{
public:
ElapsedTime()
{
    start = GetTickCount();
}

~ElapsedTime()
{
    printf( "ElapsedTime = %d", (int) (GetTickCount() - start) );
}

DWORD start;
};

int main(int argc, char *argv[])
{
    ElapsedTime elapsed;
    DWORD start = GetTickCount();
  // (0) Get program name and provide usage info.
  std::wstring PROGRAM;
  Expand(PROGRAM, argv[0]);

  if (argc < 2) {
    wcout << PROGRAM << 
      L": Usage - this program will attempt to open and validate "
      L"VXML documents.  You may specifiy one or more URIs on the command line"
      << endl;
    return 1;
  }

  VXIlogInterface  *log = NULL;

  // (1) Initialize a log object.
  if (SBlogInit() != VXIlog_RESULT_SUCCESS) {
    wcout << PROGRAM << L": Unable to initialize SBlog." << endl;
    return -1;
  }
  if (SBlogCreateResource(&log) != VXIlog_RESULT_SUCCESS || log == NULL) {
    wcout << PROGRAM << L": Unable to create SBlog instance." << endl;
    return -1;
  }

  SBlogInterface * temp = reinterpret_cast<SBlogInterface *>(log);
  temp->RegisterErrorListener(temp, LoggerErrorListener, NULL);
  temp->RegisterDiagnosticListener(temp, LoggerDiagnosticListener, NULL);
  temp->ControlDiagnosticTag(temp, ID_INTERPRETER, TRUE);

  // (2) Do other one time initialization:
  VXIMapHolder extensionRules;
  if (SBinetInit(log, ID_INET, NULL, 100, 2, 3600, NULL, 0, NULL,
                 extensionRules.GetValue(), NULL) != VXIinet_RESULT_SUCCESS)
  {
    wcout << PROGRAM << L": Unable to initialize SBinet." << endl;
    return -1;
  }

  if (VXIinterpreterInit(log, ID_INTERPRETER, NULL) != VXIinterp_RESULT_SUCCESS)
  {
    wcout << PROGRAM << L": Unable to initialize VXIinterpreter." << endl;
    return -1;
  }

  // (3) Create resources.
  VXIinetInterface * inet = NULL;
  if (SBinetCreateResource(log, NULL, &inet) != VXIinet_RESULT_SUCCESS ||
      inet == NULL)
  {
    wcout << PROGRAM << L": Unable to create SBinet instance." << endl;
    return -1;
  }

  VXIresources resources;
  resources.cache  = NULL;
  resources.inet   = inet;
  resources.jsi    = NULL;
  resources.log    = log;
  resources.object = NULL;
  resources.prompt = NULL;
  resources.rec    = NULL;
  resources.tel    = NULL;

  VXIinterpreterInterface * interpreter = NULL;
  if (VXIinterpreterCreateResource(&resources, &interpreter)
      != VXIinterp_RESULT_SUCCESS || interpreter == NULL)
  {
    wcout << PROGRAM << L": Unable to create VXIinterpreter instance." << endl;
    return -1;
  }

  // (4) Validate document.
  bool failure = false;
  std::wstring filename;

  for (int i = 1; i < argc; ++i) {
    wcout << L"-----------------------------------------------------------"
             L"-------------------" << endl;

    Expand(filename, argv[i]);
    VXIinterpreterResult result
      = interpreter->Validate(interpreter, filename.c_str());

    if (result == VXIinterp_RESULT_SUCCESS) {
      wcout << L"VALID: " << filename << endl;
      LoggerClear();
      continue;
    }

    switch (result) {
    case VXIinterp_RESULT_FETCH_TIMEOUT:   // document not retrieved in time
      wcout << L"FAILED: Retrieval failed due to timeout - "
            << filename << endl;
      break;
    case VXIinterp_RESULT_FETCH_ERROR: // document retrieval failed
      wcout << L"FAILED: Retrieval failed - " << filename << endl;
      break;
    case VXIinterp_RESULT_FAILURE:     // document is invalid VXML
      wcout << L"INVALID: " << filename << endl;
      break;
    default:
      wcout << L"FAILED: Unexpected error while processing " << filename<<endl;
      break;
    }
    LoggerPrint();
    LoggerClear();
    failure = true;
  }


  // (5) Destroy resources.
  VXIinterpreterDestroyResource(&interpreter);
  SBinetDestroyResource(&inet);
  
  // (6) Final shutdown.
  VXIinterpreterShutDown(log);
  SBinetShutDown(log);

  // (7) Clean up log.
  SBlogDestroyResource(&log);
  SBlogShutDown();

  // (8) 
  if (failure) return 1;
  return 0;
}
