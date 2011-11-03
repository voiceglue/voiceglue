
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

#define SBLOGMAPPER_EXPORTS
#include "SBlogMapper.h"

#include <map>                  // For STL map template class
#include <string>
#ifdef WIN32
#include <windows.h>
#endif

#if defined(__GNUC__) && (! defined(_GLIBCPP_USE_WCHAR_T))
namespace std
{
  typedef basic_string<wchar_t> wstring;
}
#endif

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif
#define MODULE_NAME MODULE_PREFIX L"SBlog"

#ifdef HAVE_XERCES
#include <util/PlatformUtils.hpp>       // Xerces headers
#include <parsers/SAXParser.hpp>
#include <sax/HandlerBase.hpp>
using namespace xercesc;
#else
#pragma message("WARNING: HAVE_XERCES not defined, need Apache Xerces to enable the XML error text lookup feature")
#endif

#include "SBlogOSUtils.h"

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

static const VXIchar *EMPTY_STRING = L"";

// platform independent util functions convert a wide char to int
static int wtoint(const VXIchar *strval)
{
  char mystr[128];
  if( strval == NULL ) return 0;
  SBlogWchar2Latin1(strval, mystr, 127);
  return atoi(mystr);
}

//=================================================================
//
//  ONLY COMPILE IF HAVE_XERCES IS DEFINED
//
//=================================================================
#ifdef HAVE_XERCES

// Error mapper
class SBlogErrorMapper {
public:
  SBlogErrorMapper() : xmlHandler(NULL), initialized(0), 
		       parser(NULL), errorLookUpTable(NULL) { }
  virtual ~SBlogErrorMapper();
  
  VXIlogResult ParseErrorMappingFiles(const VXIVector *ErrorMapFiles);

  const VXIchar *GetErrorText(VXIunsigned errorID, const VXIchar *moduleName,
			      int *severity);
  
  static void showExceptionMsg(const XMLException& exception);

  static VXIlogResult Error(VXIunsigned errorID, const VXIchar *errorIDText,
			    const VXIchar *format, ...);

private:
  VXIlogResult InitializeXMLParser(void);
  
  int moduleNameCmp(const VXIchar *modName, const VXIchar *pattern) const;
    
private:  
  // Internal string type
  typedef std::wstring MyString;

  // Data structure for error table entries
  typedef struct ErrorEntry {
    int severity;
    MyString message;
  } ErrorEntry;

  // Map of error strings
  typedef std::map<VXIunsigned, ErrorEntry> ErrorEntryMap;

  // Data structure for the module mapping table (linked list)
  typedef struct ModuleMappingTable {
    MyString moduleName;
    MyString errFileName;
    ErrorEntryMap errTable;
    struct ModuleMappingTable *next;
  } ModuleMappingTable;
  
private:
  class MySAXHandler;
  MySAXHandler *xmlHandler;

  VXIbool initialized;
  SAXParser *parser;
  ModuleMappingTable *errorLookUpTable;
};


// convert XMLCh char to wide char
static void  XMLCh2VXIchar(const XMLCh * x, VXIchar *output,
			   unsigned int maxlen)
{
    if (x == NULL) return;
    unsigned int len = XMLString::stringLen(x);
    if (output == NULL) return;
    for (unsigned int i = 0; i < len + 1 && i < maxlen; ++i)
      // We throw out anything above 0xFFFF
      output[i] = (x[i] != 0 && (x[i] & ~XMLCh(0xFFFF))) ? VXIchar(0xBE)
                                                       : VXIchar(x[i]);
}

class SBlogErrorMapper::MySAXHandler : public HandlerBase {
public:
  MySAXHandler();
  virtual ~MySAXHandler();

  void startElement(const XMLCh* const, AttributeList&);
  void characters(const XMLCh* const chars, const unsigned int length);
  void endElement(const XMLCh* const name);

  void warning(const SAXParseException& exception) 
    { processError(exception, L"Warning"); }
  void error(const SAXParseException& exception) 
    { processError(exception, L"Error"); }
  void fatalError(const SAXParseException& exception) 
    { processError(exception, L"Fatal Error"); }

  void resetData(void)
  {
    errorTable.erase(errorTable.begin(), errorTable.end());
  }
  
  const ErrorEntryMap &getErrorTable(void) const { return errorTable; }
  const MyString &getModuleName(void) const { return moduleName; }

private:
  void processError(const SAXParseException& exception, 
		    const VXIchar* errType);
  
private:
  ErrorEntryMap errorTable;

  VXIbool isProcessing;
  VXIunsigned errorNumber;
  int errorSeverity;
  MyString moduleName;
  MyString errorMessage;
  VXIchar tempString[4096];
};

SBlogErrorMapper::MySAXHandler::MySAXHandler() :
  HandlerBase(), errorTable(), moduleName(), errorMessage()
{
  isProcessing = 0;
  errorNumber = 0;
  errorSeverity = -1;
}

SBlogErrorMapper::MySAXHandler::~MySAXHandler()
{

}

void 
SBlogErrorMapper::MySAXHandler::startElement(const XMLCh* const name,
					     AttributeList& attributes)
{
  // 1. parse module name from the root
  XMLCh2VXIchar(name, tempString, 4096); 
  if(wcscmp(tempString, L"ErrorMessages") == 0)
  {
    for(unsigned int i = 0; i < attributes.getLength(); i++)
    {
      XMLCh2VXIchar(attributes.getName(i), tempString, 4096); 
      if(wcscmp(tempString, L"moduleName") == 0)
      {
        XMLCh2VXIchar(attributes.getValue(i), tempString, 4096);
        moduleName = tempString;
      }
    }
  }
  // 2. only care about <error> elements
  else if(wcscmp(tempString, L"error") == 0) {
    isProcessing = 1;
    errorMessage = L"";

    // Find the error number and severity (tag attributes)
    for (unsigned int i = 0; i < attributes.getLength(); i++) {
      XMLCh2VXIchar(attributes.getName(i), tempString, 4096); 
      if(wcscmp(tempString, L"num") == 0) {
        XMLCh2VXIchar(attributes.getValue(i), tempString, 4096); 
        errorNumber = wtoint(tempString);
      }
      else if(wcscmp(tempString, L"severity") == 0) {
        XMLCh2VXIchar(attributes.getValue(i), tempString, 4096); 
        errorSeverity = wtoint(tempString);
      }
    }
  }
  else if(wcscmp(tempString, L"advice") == 0) {
    // ignore the advice tag, just concate the advice's message    
  }
  else if(isProcessing)
    errorMessage += L"???"; // Runtime-determined value, unknown
}

void 
SBlogErrorMapper::MySAXHandler::characters(const XMLCh* const chars, 
					   const unsigned int length)
{
  // This is the real error message text
  if(isProcessing)
  {
    XMLCh2VXIchar(chars, tempString, 4096); 
    errorMessage += tempString;
  }
}

void SBlogErrorMapper::MySAXHandler::endElement(const XMLCh* const name)
{
  // We only care about <error> elements
  XMLCh2VXIchar(name, tempString, 4096); 
  if(wcscmp(tempString, L"error") == 0) {
    isProcessing = 0;
    // Add error entry to error mapping table (linked list)
    ErrorEntry newEntry;
    newEntry.severity = errorSeverity;
    const VXIchar *sptr = errorMessage.c_str();
    // cutting extra white spaces inside the text
    bool extraSpace = false;
    while( *sptr != L'\0' )
    {
      if( SBlogIsSpace(*sptr) )
      {
        if( !extraSpace )
        {
          newEntry.message += L' ';
          extraSpace = true;
        }
      }
      else
      {
        newEntry.message += *sptr;
        extraSpace = false;  
      }  
      sptr++;
    } 

    // Add to linked list
    errorTable.insert(ErrorEntryMap::value_type(errorNumber, newEntry));
  }
}

void 
SBlogErrorMapper::MySAXHandler::processError(
					   const SAXParseException& exception,
					   const VXIchar* errType)
{
  VXIchar ws1[2048];
  VXIchar ws2[2048];
  XMLCh2VXIchar(exception.getMessage() ? exception.getMessage() : (XMLCh*)"NONE", ws1, 2048);
  XMLCh2VXIchar(exception.getSystemId() ? exception.getSystemId() : (XMLCh*)"NONE", ws2, 2048);
  
  Error(352, L"SBlogListeners: XML error lookup file parse failed",
	L"%s%s%s%s%s%s%s%u%s%u", L"errType", errType, L"errMsg", ws1,
	L"file", ws2,
	L"line", exception.getLineNumber(), 
	L"column", exception.getColumnNumber());
}

void 
SBlogErrorMapper::showExceptionMsg(const XMLException& exception)
{
  VXIchar ws1[4096];
  XMLCh2VXIchar(exception.getMessage(), ws1, 4096);
  Error(353, L"SBlogListeners: XML parser exception", L"%s%s",
	L"exception", ws1);
}

VXIlogResult 
SBlogErrorMapper::Error(VXIunsigned errorID, const VXIchar *errorIDText,
			const VXIchar *format, ...)
{
  va_list args;
  va_start(args, format);
  VXIlogResult rc = SBlogVLogErrorToConsole(MODULE_NAME, errorID, errorIDText,
					    format, args);
  va_end(args);
  return rc;
}

VXIlogResult SBlogErrorMapper::InitializeXMLParser()
{
  if( initialized ) return VXIlog_RESULT_SUCCESS;

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBlogMapper: XMLPlatformUtils::Initialize()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };

  // Initialize the SAX parser
  try {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& exception) {
    showExceptionMsg(exception);
    return VXIlog_RESULT_FAILURE;
  }
  initialized = 1;
  parser = new SAXParser();
  parser->setDoValidation(false);
  parser->setDoNamespaces(false);

  // Register our own handler class (callback)
  xmlHandler = new MySAXHandler();
  ErrorHandler* errHandler = (ErrorHandler*) xmlHandler;
  parser->setDocumentHandler((DocumentHandler *)xmlHandler);
  parser->setErrorHandler(errHandler);  
  
  return VXIlog_RESULT_SUCCESS;
}

VXIlogResult
SBlogErrorMapper::ParseErrorMappingFiles(const VXIVector *errorMapFiles)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if ( errorMapFiles == NULL ) return rc;
  InitializeXMLParser();
  
  char narrowFileToParse[4096];
  VXIunsigned len = VXIVectorLength(errorMapFiles);
  const VXIValue *xmlFilePtr = NULL;
  const VXIchar  *xmlFile = NULL;
  for(VXIunsigned i = 0; i < len; ++i)
  {
    xmlFilePtr = VXIVectorGetElement(errorMapFiles, i); 
    if( VXIValueGetType(xmlFilePtr) == VALUE_STRING )
    {
      xmlFile = VXIStringCStr((const VXIString *)xmlFilePtr);
      SBlogWchar2Latin1(xmlFile, narrowFileToParse, 4095);
      // Let the SAX parser do its job, it will call our handler callbacks
      try 
      {
        xmlHandler->resetData();
        parser->parse(narrowFileToParse);
      }
      catch (const XMLException& exception) 
      {
        showExceptionMsg(exception);
        // skip and processing next file
        continue;
      }  

      if( xmlHandler->getErrorTable().size() > 0 )
      {
        ModuleMappingTable *map = new ModuleMappingTable;
        if( map )
        {
          map->next = NULL;
          map->moduleName = xmlHandler->getModuleName();
          map->errFileName = xmlFile; 
          map->errTable = xmlHandler->getErrorTable();
          if( errorLookUpTable == NULL )
          {
            errorLookUpTable = map;
          }
          else
          {
            map->next = errorLookUpTable;
            errorLookUpTable = map;
          }
        }
        else
        {
          return VXIlog_RESULT_FAILURE;  
        }
      } // end-if getErrorTable()
    } 
  }  
  return VXIlog_RESULT_SUCCESS;  
}

int 
SBlogErrorMapper::moduleNameCmp(const VXIchar *modName, 
				const VXIchar *pattern) const
{
  if( modName == NULL || pattern == NULL ) return 1;
  if( modName == NULL && pattern == NULL ) return 0;

  // if it is wildcard, search for identical pattern
  if( *pattern == L'*' )
  {
    unsigned int len1 = wcslen(modName), len2 = wcslen(pattern);
    if( len1 < len2-1) return 1;
    while( len2 > 0 )
    {
      if( modName[len1] != pattern[len2] ) return 1;
      len2--;
      len1--;
    }
  }
  else
  {
    // do an exact match comparison
    return wcscmp(modName, pattern);
  }
  return 0;
}

const VXIchar *
SBlogErrorMapper::GetErrorText(VXIunsigned errorID,
			       const VXIchar *moduleName,
			       int *severity)
{
  if( errorLookUpTable )
  {
    ModuleMappingTable *p = errorLookUpTable;
    while( p )
    { 
      // search for matched module name, wildcard is supported
      if( moduleNameCmp(moduleName, p->moduleName.c_str()) == 0 )
      {
        // found matched module name, find the error number
	ErrorEntryMap::const_iterator vi = p->errTable.find(errorID);
	if ( vi != p->errTable.end() )
        {
	  if( severity ) *severity = (*vi).second.severity;
	  return (*vi).second.message.c_str();
	}
	else
	{
	  return EMPTY_STRING;
	}
      }
      p = p->next;
    }
  }  
  return EMPTY_STRING;
}

// destructor
SBlogErrorMapper::~SBlogErrorMapper()
{
  if( xmlHandler ) delete xmlHandler;
  if( parser ) delete parser;
  if( errorLookUpTable )
  {
    ModuleMappingTable *p = errorLookUpTable, *d = NULL;
    while( p )
    {
      d = p;
      p = p->next;
      delete d;
    }
    errorLookUpTable = NULL;
  }
}

//=================================================================
//
//  ONLY COMPILE IF HAVE_XERCES IS DEFINED
//
//=================================================================
#endif  // HAVE_XERCES


/**
 * Create a new XML error mapper
 *
 * @param errorMapFiles   [IN] VXIVector of local OpenSpeech Browser PIK
 *                             XML error mapping files
 * @param mapper          [OUT] Handle to the error mapper
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult 
SBlogErrorMapperCreate(const VXIVector    *errorMapFiles,
		       SBlogErrorMapper  **mapper)
{
  if (! mapper)
    return VXIlog_RESULT_INVALID_ARGUMENT;

#ifdef HAVE_XERCES
  *mapper = new SBlogErrorMapper;
  if (! *mapper)
    return VXIlog_RESULT_OUT_OF_MEMORY;

  VXIlogResult rc = (*mapper)->ParseErrorMappingFiles(errorMapFiles);
  if (rc != VXIlog_RESULT_SUCCESS) {
    delete *mapper;
    *mapper = NULL;
  }

  return VXIlog_RESULT_SUCCESS;
#else
  return VXIlog_RESULT_UNSUPPORTED;
#endif
}


/**
 * Destroy an XML error mapper
 *
 * @param mapper          [IN/OUT] Handle to the error mapper, set
 *                                 to NULL on success
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult
SBlogErrorMapperDestroy(SBlogErrorMapper **mapper)
{
  if ((! mapper) || (! *mapper))
    return VXIlog_RESULT_INVALID_ARGUMENT;

#ifdef HAVE_XERCES
  delete *mapper;
  *mapper = NULL;

  return VXIlog_RESULT_SUCCESS;
#else
  return VXIlog_RESULT_UNSUPPORTED;
#endif
}


/**
 * Map an error ID to text and a severity
 *
 * @param mapper      [IN] Handle to the error mapper
 * @param errorID     [IN] Error ID to map as passed to VXIlog::Error( )
 * @param moduleName  [IN] Module name reporting the error as passed to
 *                         VXIlog::Error( )
 * @param errorText   [OUT] Error text as defined in the error mapping
 *                          file. Owned by the error text mapper, must
 *                          not be modified or freed.
 * @param severity    [OUT] Severity identifier as defined in the error
 *                          mapping file. Owned by the error text mapper,
 *                          must not be modified or freed. Typically one
 *                          of the following:
 *                            0 -> UNKNOWN (no mapping found)
 *                            1 -> CRITICAL
 *                            2 -> SEVERE
 *                            3 -> WARNING
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult
SBlogErrorMapperGetErrorInfo(SBlogErrorMapper  *mapper,
			     VXIunsigned        errorID,
			     const VXIchar     *moduleName,
			     const VXIchar    **errorText,
			     VXIint            *severityLevel)
{
#ifdef HAVE_XERCES
  *errorText = mapper->GetErrorText(errorID, moduleName, severityLevel);
#else
  *errorText = EMPTY_STRING;
#endif

  return (*errorText ? VXIlog_RESULT_SUCCESS : VXIlog_RESULT_NON_FATAL_ERROR);
}
