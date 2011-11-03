
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

#include <sstream>

#include <VXIvalue.h>
#include <VXItrd.h>
#include "VXIrec_utils.h"
#include "XMLChConverter.hpp"
#include "LogBlock.hpp"

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

VXIunsigned VXIrecData::diagLogBase = 0;

/* Avoid locale dependant ctype.h macros */
static inline bool VXIrecIsSpace(const wchar_t c)
{
  return (((c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r')) ?
          true : false);
}

/******************************************
 * VXIrecWordList : A list of words
 ******************************************/
class VXIrecWordList : public VXIrecGrammar {
public:
  enum GTYPE {
    GTYPE_NONE,
    GTYPE_DTMF,
    GTYPE_SPEECH
  };

  GRAMMARINFOLIST * grammarInfoList;
  bool enabled;
  GTYPE gtype;

public:

public: // VXIrecGrammar fuctions...
  virtual void SetEnabled(bool e)    { enabled = e;    }
  virtual bool IsEnabled() const     { return enabled; }
  virtual GRAMMARINFOLIST * GetGrammarInfoList() const { return grammarInfoList; }
  virtual bool GetGrammarInfo(const VXIchar* input,
                              VXIrecGrammarInfo ** gramInfo) const;
  virtual VXIString * GetGrammarWords() const;
  virtual bool IsDtmf() const { return gtype == GTYPE_DTMF; }

public:
  VXIrecWordList();
  virtual ~VXIrecWordList();
};

VXIrecWordList::VXIrecWordList()
  : enabled(false), gtype(VXIrecWordList::GTYPE_NONE), grammarInfoList(NULL)
{ }


VXIrecWordList::~VXIrecWordList()
{
  delete grammarInfoList;
}

bool VXIrecWordList::GetGrammarInfo(const VXIchar* input,
                                    VXIrecGrammarInfo ** gramInfo) const
{
    const wchar_t *word_ptr;
    int matches;

    for( GRAMMARINFOLIST::iterator i = grammarInfoList->begin();
	 i != grammarInfoList->end(); ++i )
    {
	word_ptr = (*i).word.c_str();

	//  DEBUG
	printf ("Comparing input \"%ls\" to grammar word \"%ls\":",
		input, word_ptr);

	matches = 0;
	if ( input == (*i).word )
	{
	    printf (" success due to equality\n");
	    matches = 1;
	}
	else if (((*i).word.length() > 1) &&
		 ((*i).word[0] == L'0') &&
		 ((*i).word[1] == L'0'))
	{
	    printf (" success due to multi-digit spec\n");
	    matches = 1;
	};
	  
	if (matches)
	{
	    *gramInfo = &(*i);
	    return true;
	}

	//  DEBUG
	printf (" failure\n");

    }
    return false;
}

VXIString * VXIrecWordList::GetGrammarWords() const
{
    const wchar_t *word;
    int added_any = 0;
    vxistring accum;
    VXIString *result;
    accum = L"";

    for( GRAMMARINFOLIST::iterator i = grammarInfoList->begin();
	 i != grammarInfoList->end(); ++i )
    {
	word = (*i).word.c_str();

	/*
	//  DEBUG
	printf ("Rec active grammar word: \"%ls\"\n", word);
	*/

	if (added_any)
	{
	    accum += L"|";
	}
	accum += word;
	++added_any;
    };

    /*
    //  DEBUG
    printf ("Total of words: \"%ls\"\n", accum.c_str());
    */

    result = VXIStringCreate (accum.c_str());
    return (result);
}

/******************************************
 * GrammarSaxHandler : The grammar sax parser
 ******************************************/

//////////////////////////////////////////////////////
static VXItrdMutex* gblMutex = NULL;
static VXIunsigned  gblDiagBase = 0;
static VXIlogInterface* gblLog = NULL;
static VXIulong GRAM_ROOT_COUNTER = 0;
static const VXIchar * const GRAM_ROOT_PREFIX  = L"_GRAMROOT_";

// recursively replace all occurence of sstr with rstr
static vxistring::size_type ReplaceChar(vxistring &modstr,
                                        const vxistring &sstr,
                                        const vxistring &rstr,
                                        vxistring::size_type pos0)
{
  vxistring::size_type pos1;
  pos1 = modstr.find(sstr, pos0);
  if (pos1 == vxistring::npos) return pos1;
  modstr.replace(pos1, sstr.length(), rstr);
  return ReplaceChar(modstr, sstr, rstr, pos1 + rstr.length());
}


static void FixEscapeChar(vxistring & modstr)
{
  vxistring::size_type pos0 = 0;
  vxistring a_sym[6] = { L"&amp;", L"&lt;", L"&gt;", L"&", L"<", L">" };
  vxistring r_sym[6] = { L"and", L"less than", L"greater than", L"and", L"less than", L"greater than" };
  for (int i = 0; i < 6; i++)
    ReplaceChar(modstr, a_sym[i], r_sym[i], 0);
}

static void PruneWhitespace(vxistring & str)
{
  vxistring::size_type len = str.length();
  if (len == 0) return;

  // Convert all whitespace to spaces.
  unsigned int i;
  for (i = 0; i < len; ++i)
    if (str[i] == '\r' || str[i] == '\n' || str[i] == '\t')  str[i] = ' ';

  // Eliminate trailing and double spaces
  bool lastWasSpace = true;
  for (i = len; i > 0; --i) {
    if (str[i-1] != ' ') {
      lastWasSpace = false;
      continue;
    }
    if (lastWasSpace)
      str.erase(i-1, 1);
    else
      lastWasSpace = true;
  }

  // Eliminate space at very beginning.
  if (str[0] == ' ') str.erase(0, 1);
}

GrammarSaxHandler::GrammarSaxHandler(VXIlogInterface *l)
: log(l), grammarInfoList(NULL), processTag(false),
  nodeType(UNKNOWN_NODE), isDTMFGram(false)
{
}

GrammarSaxHandler::~GrammarSaxHandler()
{
}

void GrammarSaxHandler::startElement(const XMLCh* const name,
                                     AttributeList& attributes)
{
  const VXIchar* fnname = L"startElement";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);
  
  XMLChToVXIchar gName(name);
  logger.logDiag(DIAG_TAG_PARSE, L"%s%s", L"Element: ", gName.c_str());   
  
  // Show attributes
  if( logger.isEnabled(VXIrecData::diagLogBase+DIAG_TAG_PARSE) )
  {
    for(unsigned int i = 0; i < attributes.getLength(); i++)
    {
      XMLChToVXIchar gAttr(attributes.getName(i));
      XMLChToVXIchar gAttrVal(attributes.getValue(i));
      logger.logDiag(DIAG_TAG_PARSE, L"%s%s%s", gAttr.c_str(), 
                     L" = ", gAttrVal.c_str());
    }
  }
  
  if( wcscmp(GRAMMAR, gName.c_str()) == 0 ) {
    nodeType = GRAMMAR_NODE;    
    // Retrieve grammar attributes
    for(unsigned int i = 0; i < attributes.getLength(); i++)
    {
      XMLChToVXIchar gAttr(attributes.getName(i));
      XMLChToVXIchar gAttrVal(attributes.getValue(i));
      // Got DTMF grammar
      if( wcscmp(L"mode", gAttr.c_str()) == 0 &&
          wcscmp(L"dtmf", gAttrVal.c_str()) == 0 ) 
      {
        isDTMFGram = true;    
      }
    }
  }
  else if( wcscmp(ITEM, gName.c_str()) == 0 ) {
    // <item>
    nodeType = ITEM_NODE;
  }
  else if( wcscmp(TAG, gName.c_str()) == 0 ) {
    // <tag>
    nodeType = ITEM_NODE;
    processTag = true;
  }
  else if( wcscmp(META, gName.c_str()) == 0 ) {
    // <meta>
    nodeType = META_NODE;
    // Retrieve semantic interpretation
    for(unsigned int i = 0; i < attributes.getLength(); i++)
    {
      XMLChToVXIchar gAttr(attributes.getName(i));
      if( wcscmp(NAME, gAttr.c_str()) == 0 )
      {
        XMLChToVXIchar gAttrVal(attributes.getValue(i));
        if( wcscmp(L"swirec_simple_result_key", gAttrVal.c_str()) )
          break; // only know how to process SSFT's semantic interp.
      }
      else if( wcscmp(CONTENT, gAttr.c_str()) == 0 ) {
        // Copy the semantic meaning
        grammarInfo.semantic = XMLChToVXIchar(attributes.getValue(i)).c_str();
      }
    }
  }
}

void GrammarSaxHandler::characters(const XMLCh* const chars,
                                   const unsigned int length)
{
  const VXIchar* fnname = L"characters";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);

  XMLChToVXIchar gChars(chars);
  logger.logDiag(DIAG_TAG_PARSE, L"%s", gChars.c_str());
  
  switch( nodeType ) {
    case ITEM_NODE: {
      if( processTag ) {
        grammarInfo.tag = gChars.c_str();
      }
      else grammarInfo.word = gChars.c_str();
    } break;
  }
}


void GrammarSaxHandler::endElement(const XMLCh* const name)
{
  const VXIchar* fnname = L"endElement";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);

  XMLChToVXIchar gName(name);

  if( wcscmp(ITEM, gName.c_str()) == 0 ) {
    // <item>
    // prune white spaces
    PruneWhitespace(grammarInfo.word);
    PruneWhitespace(grammarInfo.tag);
    PruneWhitespace(grammarInfo.semantic);
    logger.logDiag(DIAG_TAG_PARSE, L"%s%s%s%s%s%s", 
             L"word: ", grammarInfo.word.c_str(),
             L", tag: ", grammarInfo.tag.c_str(),
             L", semantic: ", grammarInfo.semantic.c_str());
    // store this item
    grammarInfoList->push_back(grammarInfo);
    // clear this item for next processing
    grammarInfo.tag = L"";
    grammarInfo.word = L"";
    // don't clear the semantic, it will remain the same for the rest of grammar
  }
  else if( wcscmp(TAG, gName.c_str()) == 0 ) {
    // <tag>
    processTag = false;
  }
}

void GrammarSaxHandler::processError(const SAXParseException& exception,
                                const VXIchar* errType)
{
  LogError(352, L"%s%s%s%s%s%s%s%u%s%u",
           L"errType", errType,
           L"errMsg", XMLChToVXIchar(exception.getMessage()).c_str(),
           L"file", XMLChToVXIchar(exception.getSystemId()).c_str(),
           L"line", exception.getLineNumber(),
           L"column", exception.getColumnNumber());
}


VXIlogResult GrammarSaxHandler::LogError(VXIunsigned errorID,
                               const VXIchar *format, ...) const
{
  VXIlogResult rc;
  va_list args;

  if (!log)
    return VXIlog_RESULT_NON_FATAL_ERROR;

  if (format) {
    va_start(args, format);
    rc = (*log->VError)(log, COMPANY_DOMAIN L".VXIrec", errorID, format, args);
    va_end(args);
  } else {
    rc = (*log->Error)(log, COMPANY_DOMAIN L".VXIrec", errorID, NULL);
  }

  return rc;
}

VXIlogResult GrammarSaxHandler::LogDiag(VXIunsigned offset, const VXIchar *subtag,
                              const VXIchar *format, ...) const
{
  VXIlogResult rc;
  va_list args;
  VXIunsigned tag = offset + VXIrecData::diagLogBase;

  if (!log)
    return VXIlog_RESULT_NON_FATAL_ERROR;

  if (format) {
    va_start(args, format);
    rc = (*log->VDiagnostic)(log, tag, subtag, format, args);
    va_end(args);
  } else {
    rc = (*log->Diagnostic)(log, tag, subtag, NULL);
  }

  return rc;
}


/******************************************
 * VXIrecData : The grammar container
 ******************************************/

// Initialize & Shutdown
int VXIrecData::Initialize(VXIlogInterface* log, VXIunsigned diagBase)
{
  gblLog = log;
  gblDiagBase = diagBase;
  diagLogBase = diagBase;
  if( gblMutex == NULL ) {
    VXItrdMutexCreate(&gblMutex);
  }

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "VXIrec_utils: XMLPlatformUtils::Initialize()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };

  // Initialize the SAX parser
  try {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& exception) {
    log->Error(log, L".VXIrec", 353, L"%s%s",
               L"grammar parser exception",
               XMLChToVXIchar(exception.getMessage()).c_str());
    return VXIlog_RESULT_FAILURE;
  }

  return 0;
}

int VXIrecData::ShutDown()
{
  if( gblMutex != NULL ) {
    VXItrdMutexDestroy(&gblMutex);
    gblMutex = NULL;
  }

  try {
      //  Log to voiceglue
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "VXIrec_utils: XMLPlatformUtils::Terminate()";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };

    XMLPlatformUtils::Terminate();
  }
  catch (const XMLException &) {
    // do nothing
  }
  return 0;
}

// C'ctor
VXIrecData::VXIrecData(VXIlogInterface *l,
           VXIinetInterface *i)
: log(l), inet(i), grammars(), 
  parser(NULL), xmlHandler(NULL)
{
  parser = new SAXParser();
  parser->setDoValidation(false);
  parser->setDoNamespaces(false);

  // Register our own handler class (callback)
  xmlHandler = new GrammarSaxHandler(l);
  ErrorHandler* errHandler = (ErrorHandler*) xmlHandler;
  parser->setDocumentHandler((DocumentHandler *)xmlHandler);
  parser->setErrorHandler(errHandler);
}

// D'ctor
VXIrecData::~VXIrecData()
{
  if( !grammars.empty() )
    for (GRAMMARS::iterator i = grammars.begin(); i != grammars.end(); ++i)
      delete *i;
  if (parser) delete parser;
  if (xmlHandler) delete xmlHandler;
}


void VXIrecData::ShowPropertyValue(const VXIchar *key,
           const VXIValue *value, VXIunsigned PROP_TAG)
{
  VXIchar subtag[512] = L"Property";
  if( value == 0 ) return;
  VXIvalueType type = VXIValueGetType(value);
  {
    switch( type )
    {
      case VALUE_INTEGER:
        wcscpy(subtag, L"Property:INT");
        LogDiag(PROP_TAG, subtag, 
             L"%s=%d", key, VXIIntegerValue((const VXIInteger*)value));
        break;
      case VALUE_FLOAT:
        wcscpy(subtag, L"Property:FLT");
        LogDiag(PROP_TAG, subtag, 
             L"%s=%f", key, VXIFloatValue((const VXIFloat*)value));
        break;
      case VALUE_BOOLEAN:     
        wcscpy(subtag, L"Property:BOOL");
        LogDiag(PROP_TAG, subtag, 
             L"%s=%d", key, VXIBooleanValue((const VXIBoolean*)value));
        break;
      case VALUE_STRING:
        wcscpy(subtag, L"Property:STR");
        LogDiag(PROP_TAG, subtag, 
             L"%s=%s", key, VXIStringCStr((const VXIString*)value));
        break;
      case VALUE_PTR:
        wcscpy(subtag, L"Property:PTR");
        LogDiag(PROP_TAG, subtag, 
             L"%s(ptr)=0x%p", key, VXIPtrValue((const VXIPtr*)value));
        break;
      case VALUE_CONTENT:
        wcscpy(subtag, L"Property:CNT");
        LogDiag(PROP_TAG, subtag, 
             L"%s(content)=0x%p", key, value);
        break;
      case VALUE_MAP:
        {
          VXIchar endtag[512];
          const VXIchar *mykey = key ? key : L"NULL";
          wcscpy(subtag, L"Property:MAP:BEG");
          wcscpy(endtag, L"Property:MAP:END");
          LogDiag(PROP_TAG, subtag, L"%s", mykey);
          const VXIchar *key = NULL;
          const VXIValue *gvalue = NULL;

          VXIMapIterator *it = VXIMapGetFirstProperty((const VXIMap*)value, &key, &gvalue);
          int ret = 0;
          while( ret == 0 && key && gvalue )
          {
            ShowPropertyValue(key, gvalue, PROP_TAG);
            ret = VXIMapGetNextProperty(it, &key, &gvalue);
          }
          VXIMapIteratorDestroy(&it);         
          LogDiag(PROP_TAG, endtag, L"%s", mykey);
        }   
        break;
      case VALUE_VECTOR:
        {
          VXIunsigned vlen = VXIVectorLength((const VXIVector*)value);
          for(VXIunsigned i = 0; i < vlen; ++i)
          {
            const VXIValue *gvalue = VXIVectorGetElement((const VXIVector*)value, i);
            ShowPropertyValue(L"Vector", gvalue, PROP_TAG);
          }
        }
        break;
      default:
        LogDiag(PROP_TAG, subtag, L"%s=%s", key, L"UNKOWN");
    }          
  }
  return;
}

void VXIrecData::ShowPropertyValue(const VXIMap *properties)
{
  const VXIchar *key = NULL;
  const VXIValue *gvalue = NULL;
  const VXIunsigned PROP_TAG = DIAG_TAG_PROPERTY;
  if( log && (log->DiagnosticIsEnabled(log, diagLogBase+PROP_TAG) == TRUE) 
      && properties ) 
  {
    VXIMapIterator *it = VXIMapGetFirstProperty(properties, &key, &gvalue);
    int ret = 0;
    while( ret == 0 && key && gvalue )
    {
      ShowPropertyValue(key, gvalue, PROP_TAG);
      ret = VXIMapGetNextProperty(it, &key, &gvalue);
    }
    VXIMapIteratorDestroy(&it);     
  }
  return;
}

// Pasre SRGS grammar
// NOTES: re-parse the same grammar seems to be slow, optimization is welcome!
//
VXIrecGrammar * VXIrecData::ParseSRGSGrammar(const vxistring & srgsgram, 
                                             const VXIMap    * properties,
                                             bool isdtmf)
{
  const VXIchar* fnname = L"ParseSRGSGrammar";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);

  VXIrecWordList * newGrammarPtr = new VXIrecWordList();

  /*
  try {
    xmlHandler->CreateGrammarInfoList();
    const VXIbyte* buffer = reinterpret_cast<const VXIbyte*>(srgsgram.c_str());
    VXIulong bufSize = srgsgram.length() * sizeof(VXIchar) / sizeof(VXIbyte);
    MemBufInputSource membuf(buffer, bufSize, "SRGSGRAM", false);
    parser->parse(membuf);
  }
  catch (const XMLException & exception) {
    LogError(352, L"%s%s", L"Parse Error", XMLChToVXIchar(exception.getMessage()).c_str());
    xmlHandler->DestroyGrammarInfoList();
    delete newGrammarPtr;
    return NULL;
  }
  catch (const SAXParseException & exception) {
    LogError(353, L"%s%s%s%d%s%d%s%s",
             L"ID", XMLChToVXIchar(exception.getSystemId()).c_str(),
             L"line", exception.getLineNumber(),
             L"col", exception.getColumnNumber(),
             L"msg", XMLChToVXIchar(exception.getMessage()).c_str());
    xmlHandler->DestroyGrammarInfoList();
    delete newGrammarPtr;
    return NULL;
  }
  */

  // Take the ownership of grammar info list
  /*
  if( !isdtmf ) isdtmf = xmlHandler->isDTMFGrammar();
  newGrammarPtr->gtype = ( isdtmf ? VXIrecWordList::GTYPE_DTMF : 
                           VXIrecWordList::GTYPE_SPEECH);
  newGrammarPtr->grammarInfoList = xmlHandler->AcquireGrammarInfoList();
  */
  return newGrammarPtr;
}

void VXIrecData::Clear()
{
  if( !grammars.empty() )
    for (GRAMMARS::iterator i = grammars.begin(); i != grammars.end(); ++i)
      delete *i;
  grammars.clear();
}

void VXIrecData::ActivateGrammar(VXIrecGrammar *g)
{
  g->SetEnabled(true);
  activeGrammars.push_back(g);
}

void VXIrecData::DeactivateGrammar(VXIrecGrammar *g)
{
  g->SetEnabled(false);
  activeGrammars.remove(g);
}

int VXIrecData::GetActiveCount() const
{
  /*int count = 0;
  for (GRAMMARS::const_iterator i = grammars.begin(); i != grammars.end(); ++i) {
    if ((*i)->IsEnabled()) count++;
  }
  return count;*/
  return activeGrammars.size();
}

VXIString *VXIrecData::GetActiveText() const
{
    VXIString *result;
    vxistring accum;
    int added_any = 0;

    accum = L"";

    //  Look through each grammar
    int count = 0;
    for (GRAMMARS::const_iterator i = grammars.begin();
	 i != grammars.end(); ++i)
    {
	if ((*i)->IsEnabled())
	{
	    //  Get list of words
	    result = ((*i)->GetGrammarWords());
	    if (VXIStringLength (result))
	    {
		if (added_any)
		{
		    accum += L"|";
		};
		accum += VXIStringCStr (result);
		++added_any;
		VXIStringDestroy (&result);
	    };
	};
    }
    
    result = VXIStringCreate (accum.c_str());

    /*
    //  DEBUG
    printf ("GetActiveText() returns \"%ls\"\n", VXIStringCStr (result));
    */

    return (result);
}

void VXIrecData::AddGrammar(VXIrecGrammar * l)
{
  if (l == NULL) return;
  grammars.push_front(l);
}


void VXIrecData::FreeGrammar(VXIrecGrammar * l)
{
  if (l == NULL) return;
  if( !grammars.empty() )
    grammars.remove(l);
  delete l;
}


bool VXIrecData::ProcessSemanticInterp(vxistring & result, const VXIrecGrammarInfo *ginfo, const VXIchar *input)
{
  const VXIchar* fnname = L"ProcessSemanticInterp";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);

  result = L"";
  if( !ginfo ) return false;
  if( ginfo->tag.empty() ) {
    // there is no tag
    result = L"<instance></instance>";
    return true;
  }

  // Processin <tag>
  // this is only a partial impl.
  // given a tag of:
  //    x=new Object(); x.a='valueA'; x.b='valueB'; y='valueY'; z='valueZ'; 
  // the following should be created:
  //    <instance>
  //       <x>
  //          <a>valueA</a>
  //          <b>valueB</b>
  //       </x>
  //       <y>valueY</y>
  //       <z>valueZ</z>
  //    </instance>
  //
  // but... this code won't do that.
  vxistring temp = ginfo->tag;
  vxistring::size_type pos = temp.find(L"=");

  // handle simple result.  ex.: <tag>frog</tag> would
  // return <instance>frog</instance>
  if( pos == vxistring::npos ){
      if ((ginfo->tag.length() > 1) &&
	  ((ginfo->tag)[0] == L'0') &&
	  ((ginfo->tag)[1] == L'0'))
      {
	  printf ("ginfo->tag is \"%ls\", is a builtin for xlation\n",
		  ((ginfo->tag).c_str()));
	  vxistring inputstring (input);
	  result = L"<instance>" + inputstring + L"</instance>";
      }
      else
      {
	  printf ("ginfo->tag is \"%ls\", not a builtin\n",
		  ((ginfo->tag).c_str()));
	  result = L"<instance>" + ginfo->tag + L"</instance>";
      };
      return true;
  }

  result =  L"<instance>";

  bool gotit = false;
  while (!temp.empty() && pos != vxistring::npos ) {
    // retrieve var.
    vxistring semvar = temp.substr(0, pos);
    PruneWhitespace(semvar);

    temp.erase(0, pos+1);
    PruneWhitespace(temp);
   
    // find value
    vxistring value;
    pos = temp.find(L";");
    if( pos == vxistring::npos ) {
      if( temp.empty() ) return false;
      value = temp;
      temp = L"";
    }
    else {
      value = temp.substr(0, pos);
      temp.erase(0, pos+1);
    }

    if( !value.empty() ) {
      PruneWhitespace(value);
      // remove single or double quotes
      if( value[0] == L'\"' ) value.erase(0,1);
      if( value[value.length()-1] == L'\"' ) value.erase(value.length()-1);
      if( value[0] == L'\'' ) value.erase(0,1);
      if( value[value.length()-1] == L'\'' ) value.erase(value.length()-1);
    }
    else {
      result = L"";
      return false;
    }

    // construct semantic
    logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%s%s%s%s%s", L"tag: ", semvar.c_str(),
               L", value: ", value.c_str(),
               L", semantic: ", ginfo->semantic.c_str());
               
    if( semvar == ginfo->semantic ) {
      if( gotit ) {
        result = L"";
        return false;
      }
      gotit = true;

      if ((value.length() > 1) &&
	  (value[0] == L'0') &&
	  (value[1] == L'0'))
      {
	  printf ("value of \"%ls\", is a builtin, replacing with %ls\n",
		  value.c_str(), input);
	  value = input;
      }
      else
      {
	  printf ("value of \"%ls\", not a builtin\n",
		  value.c_str());
      };

      result += value;
    }
    else {
      result += L"<" + semvar;
      result += L" confidence='100'>";
      result += value;
      result += L"</" + semvar + L">";
    }

    // Search for next var.
    pos = temp.find(L"=");
  }

  result +=  L"</instance>";
  return true;
}

bool VXIrecData::ConstructNLSMLForInput(const VXIchar* input, vxistring & nlsmlresult)
{
  const VXIchar* fnname = L"ConstructNLSMLForInput";
  LogBlock logger(log, VXIrecData::diagLogBase, fnname, VXIREC_MODULE);

  VXIrecGrammar * match = NULL;
  VXIrecGrammarInfo * gInfo = NULL;
  VXIchar gramid[64];
  nlsmlresult = L"";

  if( !input || input[0] == L'\0' ) return false;
  for (GRAMMARS::iterator i = activeGrammars.begin(); i != activeGrammars.end(); ++i) {
    if ((*i)->IsEnabled() && (*i)->GetGrammarInfo(input, &gInfo)) {

      logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%p%s%s",
           (match ? L"Multiple grammars matched " : L"Found matched grammar "),
           (*i), L" for ", gInfo->word.c_str());

      match = *i;

      // Constructing NLSML result
      if( nlsmlresult.empty() ) {
        // header
        nlsmlresult = L"<?xml version='1.0'?>"
                      L"<result>";
      }

      // Create grammar ID
      SWIswprintf(gramid, 64, L"%p", match);

      // interpretation
      nlsmlresult += L"<interpretation grammar='";
      nlsmlresult += gramid;
      nlsmlresult += L"' confidence='99'><input mode='";
      nlsmlresult += ( match->IsDtmf() ? L"dtmf'>" : L"speech'>");
//      nlsmlresult += gInfo->word;
      nlsmlresult += input;
      nlsmlresult += L"</input>";

      // Semantic interp.
      vxistring temp(L"");
      if( !ProcessSemanticInterp(temp, gInfo, input) )
      {
        nlsmlresult = L"";
        break;
      }

      nlsmlresult += temp.c_str();

      // close interpretation
      nlsmlresult += L"</interpretation>";
    }
  }

  if( !nlsmlresult.empty() ) {
    nlsmlresult += L"</result>";

    //  DEBUG
    printf ("NLSML result = \"%ls\"\n", nlsmlresult.c_str());

    return true;
  }
  return false;
}


//////////////////////////////////////////////////////
// These routines process JSGF-lite grammars.       //
//////////////////////////////////////////////////////

static const int FINISH_STAGE = 3;
void VXIrecData::ConvertJSGFType(const vxistring & dataIn, JSGFInfo & info)
{
  // search tokens
  const vxistring fEqualToken(L"=");
  const vxistring fCommaToken(L";");
  const vxistring fHeaderToken(L"#JSG");
  vxistring data(dataIn);
  vxistring fVersion;
  vxistring fName;
  vxistring fPublic;

  // search
  vxistring::size_type pos = data.find(fCommaToken);
  int stage = 0;
  while( pos != vxistring::npos && stage < FINISH_STAGE) {
    vxistring temp = data.substr(0, pos);
    switch( stage++ ) {
      case 0: // version
        // confirm JSGF header
        if( temp.find(fHeaderToken) == vxistring::npos) {
          // quit parsing,
          stage = FINISH_STAGE;
          break;
        }
        info.versionStr = temp;
        data.erase(0, pos+1);
        break;
      case 1: // grammar name
        info.nameStr = temp;
        data.erase(0, pos+1);
        break;
      case 2: // public type
      {
        // remove the found token
        data.erase(pos);
        // search for "=" token
        vxistring::size_type tpos = data.find(fEqualToken);
        if( tpos != vxistring::npos ) {
          info.publicStr = data.substr(0, tpos);
          // remove the public string
          data.erase(0, tpos+1);
        }
      } break;
    }
    // Continue searching
    if( stage != FINISH_STAGE ) pos = data.find(fCommaToken);
  }
  // finally copy the content string
  info.contentStr = data;
}


static void GetNextToken(const vxistring & grammar,
                         vxistring::size_type pos,
                         vxistring::size_type end,
                         vxistring & token)
{
  // Find first real character
  while (pos != grammar.length() && VXIrecIsSpace(grammar[pos])) ++pos;
  if (pos == grammar.length()) {
    token.erase();
    return;
  }

  // Extract wordRaw; we know there is at least one character
  while (VXIrecIsSpace(grammar[end - 1])) --end;
  token = grammar.substr(pos, end - pos);
}

//
// This conversion function is tied to ScanSoft's semantic interpretation
//
bool VXIrecData::JSGFToSRGS(const vxistring & incoming,
                            vxistring & result,
                            const VXIMap* props)
{
  // These define the symbols used in the JSGF-lite that defines these grammars
  const wchar_t NEXT      = L'|';
  const wchar_t BEGIN_VAL = L'{';
  const wchar_t END_VAL   = L'}';

  // retrieve language id
  vxistring xmllang(L"");
  const VXIValue *v = VXIMapGetProperty(props, REC_LANGUAGE);
  if( v && VXIValueGetType(v) == VALUE_STRING )
    xmllang = VXIStringCStr((const VXIString*)v);

  // Unable to detect language id
  if( xmllang.empty() ) return false;

  // retrieve dtmf id
  bool isDTMF = false;
  v = VXIMapGetProperty(props, REC_INPUT_MODES);
  if( v && VXIValueGetType(v) == VALUE_INTEGER)
  {
    isDTMF = (VXIIntegerValue((const VXIInteger*)v) == REC_INPUT_MODE_DTMF);
  }

  // create ROOT id, potential bug anyone care???
  VXItrdMutexLock(gblMutex);
  VXIulong cnt = GRAM_ROOT_COUNTER++;
  VXItrdMutexUnlock(gblMutex);
  std::basic_stringstream<VXIchar> ROOT_ID;
  ROOT_ID << GRAM_ROOT_PREFIX << cnt;


  result = L"<?xml version='1.0'?>"
           L"<grammar xmlns='http://www.w3.org/2001/06/grammar' "
           L"version='1.0' root='";
  result += ROOT_ID.str();
  result += L"' tag-format='swi-semantics/1.0' xml:lang='";
  result += xmllang;
  result += L"'";
  if (isDTMF)
    result += L" mode='dtmf'";
  result += L">\n"
            L"<meta name=\"swirec_simple_result_key\" content=\"MEANING\"/>"
            L"<rule id='";
  result += ROOT_ID.str();
  result += L"' scope='public'>\n"
            L"<one-of>\n";

  // These define the units from which we'll build new rules.
  const vxistring RULE_BEGIN  = L"<item>";
  const vxistring RULE_MIDDLE = L"<tag>MEANING=\"";
  const vxistring RULE_END    = L"\";</tag></item>\n";
  // TODO: Must escape values in tags???

  // These are positions within the string that we're parsing.
  vxistring::size_type pos = 0;
  vxistring::size_type next;
  vxistring::size_type valmark;

  vxistring wordRaw;
  vxistring wordValue;

  while (pos < incoming.length()) {
    // Find the next item
    next = incoming.find(NEXT, pos);
    if (next == vxistring::npos) next = incoming.length();

    // Find the start of the value
    valmark = incoming.find(BEGIN_VAL, pos);
    if (valmark == vxistring::npos || valmark > next) valmark = next;

    // Extract left hand side (raw text)
    GetNextToken(incoming, pos, valmark, wordRaw);
    if (wordRaw.empty()) {
      pos = next + 1;
      continue;
    }
    pos = valmark + 1;

    // Extract right hand side (value)
    if (valmark != next && pos < incoming.length()) {
      valmark = incoming.find(END_VAL, pos);
      if (valmark == vxistring::npos || valmark > next) {
        LogError(280, L"%s%s", L"Error", L"Mismatched { } pair");
        return false;
      }
      GetNextToken(incoming, pos, valmark, wordValue);
      if (wordValue.empty()) {
        return false;
      }
      pos = next + 1;
    }
    else
      wordValue.erase();

    // Add word to grammar
    FixEscapeChar(wordRaw);
    if (!wordValue.empty()) {
      // Got tag value
      FixEscapeChar(wordValue);
      result += RULE_BEGIN + wordRaw + RULE_MIDDLE + wordValue + RULE_END;
    } else {
      // Didn't get tag value
      result += RULE_BEGIN + wordRaw + RULE_MIDDLE + wordRaw + RULE_END;
    }
  }
  // add end tag
  result += L"</one-of></rule></grammar>";
  return true;
}


bool VXIrecData::OptionToSRGS(const VXIMap    * props,
                  const VXIVector * choices,
                  const VXIVector * values,
                  const VXIVector * acceptances,
                  const VXIbool     isDTMF,
                  vxistring & srgs)
{
  static const VXIchar* fnname = L"OptionToSRGS";
  srgs = L"";

  // (1) Validate vectors.
  if (choices == NULL || values == NULL || acceptances == NULL) {
    LogError(281, L"%s%s", L"A Vector", L"NULL");
    return false;
  }

  unsigned int len = VXIVectorLength(choices);
  if (len != VXIVectorLength(values)) {
    LogError(281, L"%s%s", L"Vectors", L"Mismatched vector lengths");
    return false;
  }

  for (unsigned int j = 0; j < len; ++j) {
    const VXIValue * c = VXIVectorGetElement(choices, j);
    const VXIValue * v = VXIVectorGetElement(values, j);
    const VXIValue * a = VXIVectorGetElement(acceptances, j);

    if (c == NULL || v == NULL || VXIValueGetType(c) != VALUE_STRING ||
        VXIValueGetType(v) != VALUE_STRING)
    {
      LogError(281, L"%s%s", L"Options", L"Contents not all strings");
      return false;
    }

    LogDiag(4, fnname, L"%s%s%s%s%s%d",
            L"choice: ", VXIStringCStr(reinterpret_cast<const VXIString *>(c)),
            L", value: ", VXIStringCStr(reinterpret_cast<const VXIString *>(v)),
            L", accept: ", VXIIntegerValue(reinterpret_cast<const VXIInteger *>(a)) );
  }

  // (2) extracting weight (does not meaning anything for simulator ???)
  VXIulong weight = 1;
  const VXIValue* val = VXIMapGetProperty(props, REC_GRAMMAR_WEIGHT);
  if( val && VXIValueGetType(val) == VALUE_FLOAT )
    weight *= (VXIulong)VXIFloatValue((const VXIFloat*)val);


  // (3) retrieve language id
  vxistring xmllang(L"");
  const VXIValue *v = VXIMapGetProperty(props, REC_LANGUAGE);
  if( v && VXIValueGetType(v) == VALUE_STRING )
    xmllang = VXIStringCStr((const VXIString*)v);
  // Unable to detect language id
  if( xmllang.empty() ) return false;

  // (4) create ROOT id, wrap-round potential bug, anyone care???
  VXItrdMutexLock(gblMutex);
  VXIulong cnt = GRAM_ROOT_COUNTER++;
  VXItrdMutexUnlock(gblMutex);
  std::basic_stringstream<VXIchar> ROOT_ID;
  ROOT_ID << GRAM_ROOT_PREFIX << cnt;

  // (4) construct srgs -- NOTES: this construction is tied to SSFT's semantic
  srgs = L"<?xml version='1.0'?><grammar "
         L"xmlns='http://www.w3.org/2001/06/grammar' "
         L"version='1.0' root='";
  srgs += ROOT_ID.str();
  srgs += L"' tag-format='swi-semantics/1.0' xml:lang='";
  srgs += xmllang;
  srgs += L"'";
  if (isDTMF) srgs += L" mode='dtmf'";
  srgs += L"><meta name='swirec_simple_result_key' content='MEANING'/>"
          L"<rule id='";
  srgs += ROOT_ID.str();
  srgs += L"' scope='public'><one-of>";

  // (5) Add item for each <option>.

  // These define the units from which we'll build new rules.
  const vxistring RULE_BEGIN  = L"<item>";
  const vxistring RULE_MIDDLE = L"<tag>MEANING='";
  const vxistring RULE_END    = L"';</tag></item>";

  for (unsigned int i = 0; i < len; ++i) {
    const VXIValue * c = VXIVectorGetElement(choices, i);
    const VXIValue * v = VXIVectorGetElement(values, i);
    const VXIValue * a = VXIVectorGetElement(acceptances, i);
    vxistring aitem( VXIStringCStr(reinterpret_cast<const VXIString *>(c)));
    vxistring avalue(VXIStringCStr(reinterpret_cast<const VXIString *>(v)));
    // normalize text
    FixEscapeChar(aitem);

    if( !isDTMF && VXIIntegerValue(reinterpret_cast<const VXIInteger *>(a)) > 0 )
    {
      // If Recongizer does not support approximate recognition we need to
      // split the string into single item based on single space
      bool done = false;
      vxistring tmpStr(aitem);
      vxistring::size_type idx;
      VXIchar aWord[1024];
      const VXIchar *ptr = tmpStr.c_str();

      // search for the 1st white space
      // TODO: should take care about tab, multiple spaces!!!
      idx = tmpStr.find(L" ", 0);
      do
      {
        // create single item separated by space
        if( idx == vxistring::npos )
        {
          idx = tmpStr.length();
          done = true;
        }
        // create separate <item> for each word
        wcsncpy(aWord, ptr, idx);
        aWord[idx] = L'\0';
        srgs += RULE_BEGIN;
        srgs += aWord;
        srgs += RULE_MIDDLE;
        srgs += avalue;
        srgs += RULE_END;

        // search for the next word
        if( !done )
        {
          ptr += (idx+1);
          tmpStr = ptr;
          idx = tmpStr.find(L" ", idx + 1);
        }
      } while( !done );

    } else {
      srgs += RULE_BEGIN;
      srgs += aitem;
      srgs += RULE_MIDDLE;
      srgs += avalue;
      srgs += RULE_END;
    }
  }

  // (6) Finish everything and add grammar.

  srgs += L"</one-of></rule></grammar>";
  LogDiag(DIAG_TAG_RECOGNITION, fnname, L"%s%s", L"OPTION GRAMMAR: ", srgs.c_str());
  return true;
}

VXIlogResult VXIrecData::LogError(VXIunsigned errorID,
                               const VXIchar *format, ...) const
{
  VXIlogResult rc;
  va_list args;

  if (!log)
    return VXIlog_RESULT_NON_FATAL_ERROR;

  if (format) {
    va_start(args, format);
    rc = (*log->VError)(log, COMPANY_DOMAIN L".VXIrec", errorID, format, args);
    va_end(args);
  } else {
    rc = (*log->Error)(log, COMPANY_DOMAIN L".VXIrec", errorID, NULL);
  }

  return rc;
}

VXIlogResult VXIrecData::LogDiag(VXIunsigned tag, const VXIchar *subtag,
                              const VXIchar *format, ...) const
{
  VXIlogResult rc;
  va_list args;

  if (!log)
    return VXIlog_RESULT_NON_FATAL_ERROR;

  if (format) {
    va_start(args, format);
    rc = (*log->VDiagnostic)(log, tag + diagLogBase, subtag, format, args);
    va_end(args);
  } else {
    rc = (*log->Diagnostic)(log, tag + diagLogBase, subtag, NULL);
  }

  return rc;
}

bool VXIrecData::FetchContent(
    const VXIchar *  name, 
    const VXIMap  *  properties,
    VXIbyte       ** result,
    VXIulong      &  read )
{
  *result = NULL;
  read = 0;

  VXIinetStream *stream;
  VXIMapHolder streamInfo;
		 
  if (inet->Open(inet,
		L"Rec",
		name,
		INET_MODE_READ,
		0,
		properties,
		streamInfo.GetValue(),
		&stream ) != VXIinet_RESULT_SUCCESS ) return false;

  const VXIValue * tempSize = NULL;
  tempSize = VXIMapGetProperty(streamInfo.GetValue(), INET_INFO_SIZE_BYTES);
  if (tempSize == NULL || VXIValueGetType(tempSize) != VALUE_INTEGER) {
    inet->Close(inet, &stream);
	return false;
  }

  VXIint32 bufSize
    = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(tempSize));

  if (bufSize < 2047)
    bufSize = 2047;

  ++bufSize;
  VXIbyte * buffer = new VXIbyte[bufSize];
  if(buffer == NULL){
    inet->Close(inet, &stream);
    return false;
  }

  bool reachedEnd = false; 
  while (!reachedEnd) {
    VXIulong bytesRead = 0;
    switch (inet->Read(inet, buffer+read, bufSize-read, &bytesRead, stream)) {
    case VXIinet_RESULT_SUCCESS:
      read += bytesRead;
      break;
    case VXIinet_RESULT_END_OF_STREAM:
      read += bytesRead;
      reachedEnd = true;  // exit while
      break;
    case VXIinet_RESULT_WOULD_BLOCK:
      VXItrdThreadYield();
      break;
    default:
      inet->Close(inet, &stream);
      delete[] buffer;
      return false;
    }

    if (read == static_cast<VXIulong>(bufSize)) {
      // The number of bytes read exceeds the number expected.  Double the
      // size and keep reading.
      VXIbyte * temp = new VXIbyte[2*bufSize];
      if(temp == NULL) {
        delete [] buffer;
        return false;
      }
      memcpy(static_cast<void *>(temp), static_cast<void *>(buffer),
             bufSize * sizeof(VXIbyte));
      delete[] buffer;
      buffer = temp;
      bufSize *= 2;
    }
  }

  inet->Close(inet, &stream);
  *result = buffer;

  return true;
}
