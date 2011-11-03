
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

/***********************************************************************
 * This class performs several jobs.  Among them:
 *
 * 1) <grammar> contents are collapsed and blindly copied.
 *
 * 2) Explicit and implicit prompts are converted into <prompt> elements with
 *    full SSML headers and trailers.
 *    NOTE: <value>, <enumerate>, and <audio> don't contribute to the copy
 *          depth as they require special handling.
 *
 * 3) PCDATA consisting only of whitespace is discarded.
 *
 * 4) <error>, <help>, <noinput> and <nomatch> are converted into a <catch>
 *    with the appropriate event name.
 *
 * 5) <choice> elements may be assigned dtmf values.
 *
 * 6) Attribute conflicts are detected whenever possible.
 *
 * 7) <metadata> is ignored (except in <grammar> or <prompt>).
 *
 * 8) Assign xml:base and xml:lang to <grammar>, <prompt>, <field>, and <menu>.
 * 
 * NOTE: By performing these duties, the DocumentConverter greatly simplifies
 *       the rest of the interpreter.  Unfortunately, this means that this
 *       code is tricky to write and maintain.  PLEASE BE VERY CAREFUL THAT
 *       YOU UNDERSTAND WHAT IS HAPPENING HERE BEFORE MAKING ANY CHANGES.
 * 
 ***********************************************************************
 *
 * 1 Grammar processing
 *
 * The content of <grammar> elements is blindly copied.  This is controlled
 * by 'copyDepth' and 'inGrammar'.  This is possible because VoiceXML
 * grammars are entirely static.  Reducing them down to PCDATA at this stage
 * allows the interpreter to treat the entire grammar as a black box without
 * regard to supported types.
 *
 ***********************************************************************
 * 
 * 2 Prompt processing 
 *
 * A frequently noted defect of VoiceXML is that prompts may be both explicitly
 * declared (i.e. within <prompt>) or implicitly declared as raw CDATA.  In
 * this code, we convert all implicit prompts into explicit ones and collapse
 * most of the SSML into PCDATA.  At the end, prompts are composed only of 
 * those elements requiring special handling (i.e. <audio>, <enumerate>,  
 * <value>, and <foreach>) and PCDATA.
 *
 * In the case of <foreach>, there is extra logic for handling executable
 * content within a <foreach> element.
 *
 ***********************************************************************
 * 
 * 3 Whitespace stripping
 *
 * Because of implicit prompts, the XML parser is required to treat most
 * whitespace as significant information.  These 'empty prompts' are stripped
 * at this level to simplify later processing.
 *
 * Likewise, <choice> and <option> content is whitespace stripped (though this
 * task must be performed by the VXMLDocumentRep since Xerces may split PCDATA
 * across multiple calls to DocumentConverter::characters).
 *
 ***********************************************************************
 * 
 * 4 Event collapsing
 *
 * VoiceXML defines <error>, <help>, <noinput> and <nomatch> as aliases
 * for <catch event="...">.  These are converted at this layer (see
 * startElement) to simplify later processing.
 *
 ***********************************************************************
 * 
 * 5 Menu choice processing
 *
 * DTMF values may be assigned to <choice>s if necessary.
 *
 ***********************************************************************
 * 
 * 6 Attribute conflict detection
 *
 * There are many cases in the VoiceXML specification where conflicting
 * attributes result in error.badfetch.  These should (in future) be
 * detected by the XML schema and are caught at this level.
 *
 * One difficult case appears inside of <grammar> and <script> where either
 * the 'src' attribute or content is legal, but not both.  As grammars may
 * not appear inside scripts and vice versa, this is detected with a simple
 * scheme using the variables 'contentForbidden' and 'hasContent'.
 *
 ***********************************************************************
 *
 * 7 <metadata> stripped
 *
 * This content has no runtime impact.  It is stripped at this level
 * UNLESS it is inside a <grammar> or <prompt> tag.
 *
 ***********************************************************************
 *
 * 8 <prompt> & <grammar> always have xml:base and xml:lang attributes.
 *   <field>, <menu> always has xml:lang
 *
 * In an effort to keep these elements independent from the rest of VXML
 * this attribute is always set.  This allows the SRGS or SSML to
 * incorporate an xml:base in the root element which might be useful if
 * the contents are being sent to a seperate server.
 *
 * Likewise, a language is required for the type attribute on fields (used
 * for builtins) and for generating the CDATA grammars in menu choices.
 *
 ***********************************************************************/
 
#include "DocumentConverter.hpp"
#include <sax2/Attributes.hpp>
#include <sax/SAXParseException.hpp>
#include <sax/Locator.hpp>
#include <vector>
#include <algorithm>
#include <sstream>                     // by ProcessNodeFinal
#include "VXMLDocument.hpp"
#include "VXMLDocumentRep.hpp"
#include "XMLChConverter.hpp"
#include <iostream>
#include <algorithm>

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

//#############################################################################


enum {
  PRIV_ELEM_RangeStart    = 0x200,   // This should always be the first entry

  // Events
  PRIV_ELEM_ERROR,
  PRIV_ELEM_HELP,
  PRIV_ELEM_NOINPUT,
  PRIV_ELEM_NOMATCH,
  PRIV_ELEM_METADATA,

  // SSML
  PRIV_ELEM_BREAK,
  PRIV_ELEM_DESC,
  PRIV_ELEM_EMPHASIS,
  PRIV_ELEM_PARAGRAPH,
  PRIV_ELEM_PHONEME,
  PRIV_ELEM_PROSODY,
  PRIV_ELEM_SAYAS,
  PRIV_ELEM_SENTENCE,
  PRIV_ELEM_SUB,
  PRIV_ELEM_VOICE
};

enum {
  PRIV_ATTRIB_RangeStart  = 0x200,   // This should always be the first entry
  PRIV_ATTRIB_SCHEMALOC
};


struct VXMLElementInfo {
public:
  const VXIchar * key;
  int value;

  VXMLElementInfo(const VXIchar * k, int v)
    : key(k), value(v)  { }

  VXMLElementInfo(const VXMLElementInfo & k)
    : key(k.key), value(k.value)   { }

  VXMLElementInfo& operator=(const VXMLElementInfo & k)
    { key = k.key; value = k.value; return *this; }
};


bool operator< (const VXMLElementInfo & x, const VXMLElementInfo & y)
{ if (x.key == NULL || y.key == NULL) return false;
  return wcscmp(x.key, y.key) < 0; }


struct VXMLAttribute {
public:
  const VXIchar * key;
  int value;

  VXMLAttribute(const VXIchar * k, int v)
    : key(k), value(v)  { }

  VXMLAttribute(const VXMLAttribute & k)
    : key(k.key), value(k.value)       { }

  VXMLAttribute& operator=(const VXMLAttribute & k)
    { key = k.key; value = k.value; return *this; }
};


bool operator< (const VXMLAttribute & x, const VXMLAttribute & y)
{ if (x.key == NULL || y.key == NULL) return false;
  return wcscmp(x.key, y.key) < 0; }


typedef std::vector<VXMLAttribute>   TABLE_ATTRS;
typedef std::vector<VXMLElementInfo> TABLE_ELEMS;

TABLE_ATTRS  g_attrs;
TABLE_ELEMS  g_elems;

//#############################################################################
 
static void InitializeTables()
{
  // (1) Elements

  // (1.1) VXML
  g_elems.push_back(VXMLElementInfo(L"assign"      , NODE_ASSIGN));
  g_elems.push_back(VXMLElementInfo(L"audio"       , NODE_AUDIO));
  g_elems.push_back(VXMLElementInfo(L"block"       , NODE_BLOCK));
  g_elems.push_back(VXMLElementInfo(L"cancel"      , NODE_CANCEL));
  g_elems.push_back(VXMLElementInfo(L"catch"       , NODE_CATCH));
  g_elems.push_back(VXMLElementInfo(L"choice"      , NODE_CHOICE));
  g_elems.push_back(VXMLElementInfo(L"clear"       , NODE_CLEAR));
  g_elems.push_back(VXMLElementInfo(L"data"        , NODE_DATA));
  g_elems.push_back(VXMLElementInfo(L"disconnect"  , NODE_DISCONNECT));
  g_elems.push_back(VXMLElementInfo(L"else"        , NODE_ELSE));
  g_elems.push_back(VXMLElementInfo(L"elseif"      , NODE_ELSEIF));
  g_elems.push_back(VXMLElementInfo(L"enumerate"   , NODE_ENUMERATE));
  g_elems.push_back(VXMLElementInfo(L"exit"        , NODE_EXIT));
  g_elems.push_back(VXMLElementInfo(L"field"       , NODE_FIELD));
  g_elems.push_back(VXMLElementInfo(L"filled"      , NODE_FILLED));
  g_elems.push_back(VXMLElementInfo(L"foreach"     , NODE_FOREACH));
  g_elems.push_back(VXMLElementInfo(L"form"        , NODE_FORM));
  g_elems.push_back(VXMLElementInfo(L"goto"        , NODE_GOTO));
  g_elems.push_back(VXMLElementInfo(L"grammar"     , NODE_GRAMMAR));
  g_elems.push_back(VXMLElementInfo(L"if"          , NODE_IF));
  g_elems.push_back(VXMLElementInfo(L"initial"     , NODE_INITIAL));
  g_elems.push_back(VXMLElementInfo(L"link"        , NODE_LINK));
  g_elems.push_back(VXMLElementInfo(L"log"         , NODE_LOG));
  g_elems.push_back(VXMLElementInfo(L"mark"        , NODE_MARK));
  g_elems.push_back(VXMLElementInfo(L"menu"        , NODE_MENU));
  g_elems.push_back(VXMLElementInfo(L"meta"        , NODE_META));
  g_elems.push_back(VXMLElementInfo(L"object"      , NODE_OBJECT));
  g_elems.push_back(VXMLElementInfo(L"option"      , NODE_OPTION));
  g_elems.push_back(VXMLElementInfo(L"param"       , NODE_PARAM));
  g_elems.push_back(VXMLElementInfo(L"prompt"      , NODE_PROMPT));
  g_elems.push_back(VXMLElementInfo(L"property"    , NODE_PROPERTY));
  g_elems.push_back(VXMLElementInfo(L"record"      , NODE_RECORD));
  g_elems.push_back(VXMLElementInfo(L"return"      , NODE_RETURN));
  g_elems.push_back(VXMLElementInfo(L"reprompt"    , NODE_REPROMPT));
  g_elems.push_back(VXMLElementInfo(L"script"      , NODE_SCRIPT));
  g_elems.push_back(VXMLElementInfo(L"subdialog"   , NODE_SUBDIALOG));
  g_elems.push_back(VXMLElementInfo(L"submit"      , NODE_SUBMIT));
  g_elems.push_back(VXMLElementInfo(L"throw"       , NODE_THROW));
  g_elems.push_back(VXMLElementInfo(L"transfer"    , NODE_TRANSFER));
  g_elems.push_back(VXMLElementInfo(L"value"       , NODE_VALUE));
  g_elems.push_back(VXMLElementInfo(L"var"         , NODE_VAR));
  g_elems.push_back(VXMLElementInfo(L"vxml"        , NODE_VXML));

  // (1.2) from Defaults document
  g_elems.push_back(VXMLElementInfo(L"defaults"    , DEFAULTS_ROOT));
  g_elems.push_back(VXMLElementInfo(L"language"    , DEFAULTS_LANGUAGE));

  // (1.3) Internals elements (these are converted to others)
  g_elems.push_back(VXMLElementInfo(L"error"       , PRIV_ELEM_ERROR));
  g_elems.push_back(VXMLElementInfo(L"help"        , PRIV_ELEM_HELP));
  g_elems.push_back(VXMLElementInfo(L"noinput"     , PRIV_ELEM_NOINPUT));
  g_elems.push_back(VXMLElementInfo(L"nomatch"     , PRIV_ELEM_NOMATCH));
  g_elems.push_back(VXMLElementInfo(L"metadata"    , PRIV_ELEM_METADATA));

  // (1.4) SSML
  g_elems.push_back(VXMLElementInfo(L"break"       , PRIV_ELEM_BREAK));
  g_elems.push_back(VXMLElementInfo(L"desc"        , PRIV_ELEM_DESC));
  g_elems.push_back(VXMLElementInfo(L"emphasis"    , PRIV_ELEM_EMPHASIS));
  g_elems.push_back(VXMLElementInfo(L"p"           , PRIV_ELEM_PARAGRAPH));
  g_elems.push_back(VXMLElementInfo(L"paragraph"   , PRIV_ELEM_PARAGRAPH));
  g_elems.push_back(VXMLElementInfo(L"phoneme"     , PRIV_ELEM_PHONEME));
  g_elems.push_back(VXMLElementInfo(L"prosody"     , PRIV_ELEM_PROSODY));
  g_elems.push_back(VXMLElementInfo(L"s"           , PRIV_ELEM_SENTENCE));
  g_elems.push_back(VXMLElementInfo(L"say-as"      , PRIV_ELEM_SAYAS));
  g_elems.push_back(VXMLElementInfo(L"sentence"    , PRIV_ELEM_SENTENCE));
  g_elems.push_back(VXMLElementInfo(L"sub"         , PRIV_ELEM_SUB));
  g_elems.push_back(VXMLElementInfo(L"voice"       , PRIV_ELEM_VOICE));

  // (2) Element attributes

  // (2.1) VXML 1.0
  g_attrs.push_back(VXMLAttribute(L"_itemname"     , ATTRIBUTE__ITEMNAME));
  g_attrs.push_back(VXMLAttribute(L"aai"           , ATTRIBUTE_AAI));
  g_attrs.push_back(VXMLAttribute(L"aaiexpr"       , ATTRIBUTE_AAIEXPR));
  g_attrs.push_back(VXMLAttribute(L"accept"        , ATTRIBUTE_ACCEPT));
  g_attrs.push_back(VXMLAttribute(L"application"   , ATTRIBUTE_APPLICATION));
  g_attrs.push_back(VXMLAttribute(L"archive"       , ATTRIBUTE_ARCHIVE));
  g_attrs.push_back(VXMLAttribute(L"array"         , ATTRIBUTE_ARRAY));
  g_attrs.push_back(VXMLAttribute(L"bargein"       , ATTRIBUTE_BARGEIN));
  g_attrs.push_back(VXMLAttribute(L"bargeintype"   , ATTRIBUTE_BARGEINTYPE));
  g_attrs.push_back(VXMLAttribute(L"base"          , ATTRIBUTE_BASE));
  g_attrs.push_back(VXMLAttribute(L"beep"          , ATTRIBUTE_BEEP));
  g_attrs.push_back(VXMLAttribute(L"bridge"        , ATTRIBUTE_BRIDGE));
  g_attrs.push_back(VXMLAttribute(L"charset"       , ATTRIBUTE_CHARSET));
  g_attrs.push_back(VXMLAttribute(L"classid"       , ATTRIBUTE_CLASSID));
  g_attrs.push_back(VXMLAttribute(L"codebase"      , ATTRIBUTE_CODEBASE));
  g_attrs.push_back(VXMLAttribute(L"codetype"      , ATTRIBUTE_CODETYPE));
  g_attrs.push_back(VXMLAttribute(L"cond"          , ATTRIBUTE_COND));
  g_attrs.push_back(VXMLAttribute(L"connecttimeout", ATTRIBUTE_CONNECTTIME));
  g_attrs.push_back(VXMLAttribute(L"content"       , ATTRIBUTE_CONTENT));
  g_attrs.push_back(VXMLAttribute(L"count"         , ATTRIBUTE_COUNT));
  g_attrs.push_back(VXMLAttribute(L"data"          , ATTRIBUTE_DATA));
  g_attrs.push_back(VXMLAttribute(L"dest"          , ATTRIBUTE_DEST));
  g_attrs.push_back(VXMLAttribute(L"destexpr"      , ATTRIBUTE_DESTEXPR));
  g_attrs.push_back(VXMLAttribute(L"dtmf"          , ATTRIBUTE_DTMF));
  g_attrs.push_back(VXMLAttribute(L"dtmfterm"      , ATTRIBUTE_DTMFTERM));
  g_attrs.push_back(VXMLAttribute(L"enctype"       , ATTRIBUTE_ENCTYPE));
  g_attrs.push_back(VXMLAttribute(L"event"         , ATTRIBUTE_EVENT));
  g_attrs.push_back(VXMLAttribute(L"eventexpr"     , ATTRIBUTE_EVENTEXPR));
  g_attrs.push_back(VXMLAttribute(L"expr"          , ATTRIBUTE_EXPR));
  g_attrs.push_back(VXMLAttribute(L"expritem"      , ATTRIBUTE_EXPRITEM));
  g_attrs.push_back(VXMLAttribute(L"fetchaudio"    , ATTRIBUTE_FETCHAUDIO));
  g_attrs.push_back(VXMLAttribute(L"fetchhint"     , ATTRIBUTE_FETCHHINT));
  g_attrs.push_back(VXMLAttribute(L"fetchtimeout"  , ATTRIBUTE_FETCHTIMEOUT));
  g_attrs.push_back(VXMLAttribute(L"finalsilence"  , ATTRIBUTE_FINALSILENCE));
  g_attrs.push_back(VXMLAttribute(L"http-equiv"    , ATTRIBUTE_HTTP_EQUIV));
  g_attrs.push_back(VXMLAttribute(L"id"            , ATTRIBUTE_ID));
  g_attrs.push_back(VXMLAttribute(L"item"          , ATTRIBUTE_ITEM));
  g_attrs.push_back(VXMLAttribute(L"label"         , ATTRIBUTE_LABEL));
  g_attrs.push_back(VXMLAttribute(L"maxage"        , ATTRIBUTE_MAXAGE));
  g_attrs.push_back(VXMLAttribute(L"maxstale"      , ATTRIBUTE_MAXSTALE));
  g_attrs.push_back(VXMLAttribute(L"maxtime"       , ATTRIBUTE_MAXTIME));
  g_attrs.push_back(VXMLAttribute(L"message"       , ATTRIBUTE_MESSAGE));
  g_attrs.push_back(VXMLAttribute(L"messageexpr"   , ATTRIBUTE_MESSAGEEXPR));
  g_attrs.push_back(VXMLAttribute(L"method"        , ATTRIBUTE_METHOD));
  g_attrs.push_back(VXMLAttribute(L"modal"         , ATTRIBUTE_MODAL));
  g_attrs.push_back(VXMLAttribute(L"mode"          , ATTRIBUTE_MODE));
  g_attrs.push_back(VXMLAttribute(L"name"          , ATTRIBUTE_NAME));
  g_attrs.push_back(VXMLAttribute(L"nameexpr"      , ATTRIBUTE_NAMEEXPR));
  g_attrs.push_back(VXMLAttribute(L"namelist"      , ATTRIBUTE_NAMELIST));
  g_attrs.push_back(VXMLAttribute(L"next"          , ATTRIBUTE_NEXT));
  g_attrs.push_back(VXMLAttribute(L"nextitem"      , ATTRIBUTE_NEXTITEM));
  g_attrs.push_back(VXMLAttribute(L"root"          , ATTRIBUTE_ROOT));
  g_attrs.push_back(VXMLAttribute(L"scope"         , ATTRIBUTE_SCOPE));
  g_attrs.push_back(VXMLAttribute(L"slot"          , ATTRIBUTE_SLOT));
  g_attrs.push_back(VXMLAttribute(L"src"           , ATTRIBUTE_SRC));
  g_attrs.push_back(VXMLAttribute(L"srcexpr"       , ATTRIBUTE_SRCEXPR));
  g_attrs.push_back(VXMLAttribute(L"tag-format"    , ATTRIBUTE_TAGFORMAT));
  g_attrs.push_back(VXMLAttribute(L"timeout"       , ATTRIBUTE_TIMEOUT));
  g_attrs.push_back(VXMLAttribute(L"transferaudio" , ATTRIBUTE_TRANSFERAUDIO));
  g_attrs.push_back(VXMLAttribute(L"type"          , ATTRIBUTE_TYPE));
  g_attrs.push_back(VXMLAttribute(L"value"         , ATTRIBUTE_VALUE));
  g_attrs.push_back(VXMLAttribute(L"valuetype"     , ATTRIBUTE_VALUETYPE));
  g_attrs.push_back(VXMLAttribute(L"version"       , ATTRIBUTE_VERSION));
  g_attrs.push_back(VXMLAttribute(L"weight"        , ATTRIBUTE_WEIGHT));
  g_attrs.push_back(VXMLAttribute(L"xml:lang"      , ATTRIBUTE_XMLLANG));

  // (2.2) Internals attributes (these are converted to others)
  g_attrs.push_back(VXMLAttribute(L"schemaLocation", PRIV_ATTRIB_SCHEMALOC));


  // (3) Final stuff.
  std::sort(g_attrs.begin(), g_attrs.end());
  std::sort(g_elems.begin(), g_elems.end());
}


static void DeinitializeTables()
{
  g_attrs.clear();
  g_elems.clear();
}


static bool ConvertElement(const VXIchar * name, int & result)
{
  if (name == NULL) return false;

  TABLE_ELEMS::iterator i = std::lower_bound(g_elems.begin(), g_elems.end(),
                                             VXMLElementInfo(name, NODE_IF));

  if (wcscmp(name, (*i).key) != 0) return false;

  result = (*i).value;
  return true;
}


static bool ConvertAttribute(const VXIchar * name, int & result)
{
  if (name == NULL) return false;

  TABLE_ATTRS::iterator i = std::lower_bound(g_attrs.begin(), g_attrs.end(),
                                             VXMLAttribute(name,ATTRIBUTE_ID));

  if (wcscmp(name, (*i).key) != 0) return false;

  result = (*i).value;

  return true;
}

inline static void TrimUriBase(vxistring & uri)
{
  vxistring::size_type pos = uri.rfind(L'?');
  if( pos != vxistring::npos )
    uri.erase(pos);    
}

//#############################################################################

class DummyLocator : public Locator {
public:
  DummyLocator()  { }
  virtual ~DummyLocator() { }
  virtual const XMLCh* getPublicId() const    { return NULL; }
  virtual const XMLCh* getSystemId() const    { return NULL; }
  virtual XMLSSize_t getLineNumber() const    { return -1; }
  virtual XMLSSize_t getColumnNumber() const  { return -1; }
};

static DummyLocator DUMMYLOCATOR;

//#############################################################################

bool DocumentConverter::Initialize()
{
  InitializeTables();
  return true;
}


void DocumentConverter::Deinitialize()
{
  DeinitializeTables();
}


DocumentConverter::DocumentConverter()
  : locator(&DUMMYLOCATOR), doc(NULL), documentLevel(DOCUMENT),
    copyDepth(0), pcdataImpliesPrompt(true), contentForbidden(false), hasContent(true),
    inCDATA(false), foreachImplicit(false)
{
}


DocumentConverter::~DocumentConverter()
{
  ResetDocument();
}

// remove all irrelavant part from the base uri
void DocumentConverter::SetBaseUri(const VXIchar* const base) 
{ 
  if( base != NULL ) {
    baseUri = base; 
  }
  else baseUri=L"";  
}

void DocumentConverter::RestoreBaseURLFromCache(const VXMLDocument & doc)
{
  vxistring baseURL;
  doc.GetBaseURL(baseURL);
  if( !baseURL.empty() ) SetBaseUri(baseURL.c_str());
}

// Restore default language id if read from cache
void DocumentConverter::RestoreDefaultLangFromCache(const VXMLDocument & doc)
{
  doc.GetDefaultLang(defaultLang);
}

void DocumentConverter::ParseException(const VXIchar * message) const
{
  if (message == NULL) throw SAXParseException(NULL, *locator);

  VXIcharToXMLCh text(message);
  throw SAXParseException(text.c_str(), *locator);
}


void DocumentConverter::startDocument()
{
  version = -1;
  choiceNumber = 0;
  inGrammar = false;
  implicitPrompt = false;
  explicitPrompt = 0;
  copyDepth = 0;
  ignoreDepth = 0;
  pcdataImpliesPrompt = true;
  
  // Don't know the language yet!
  documentLang=L"";

  if (doc != NULL)
  {
      VXMLDocumentRep::Release(doc);
      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "del VXMLDocumentRep "
		    << Pointer_to_Std_String((const void *) doc);
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };
  }
  doc = new VXMLDocumentRep();
  if (doc == NULL)
    ParseException(L"unable to allocate memory for element");
}


void DocumentConverter::endDocument()
{
  if ( !filledNameList.empty() && !namedControlItems.empty() ){
    while(!filledNameList.empty()){
      vxistring namelist = filledNameList.front();
      filledNameList.pop_front();

      std::basic_stringstream<VXIchar> namestream(namelist);
      while (namestream.good()) {
        vxistring temp;
        namestream >> temp;
        if (!temp.empty()){
          if ( std::find(
              namedControlItems.begin(),
              namedControlItems.end(),
              temp) != namedControlItems.end()){
              ParseException(L"filled namelist cannot contain control items");
            }
        }
      }
    }
  }
} 


void DocumentConverter::ResetDocument()
{
  if (doc != NULL)
  {
      VXMLDocumentRep::Release(doc);
            if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "del VXMLDocumentRep "
		    << Pointer_to_Std_String((const void *) doc);
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };
  };
  doc = NULL;
}


VXMLDocumentRep * DocumentConverter::GetDocument()
{
  VXMLDocumentRep * temp = doc;
  doc = NULL;
  return temp;
}


void DocumentConverter::setDocumentLocator(const Locator* const loc)
{
  if (loc == NULL)
    locator = &DUMMYLOCATOR;
  else
    locator = loc;
}


void DocumentConverter::startElement(const XMLCh* const uri,
                                     const XMLCh* const localname,
                                     const XMLCh* const qname,
                                     const Attributes & attrs)
{
  // (0) Just ignore this?
  if (ignoreDepth > 0) {
    ++ignoreDepth;
    return;
  }

  // (1) Copy this element or look it up?

  // (1.1) Inside a <grammar>, we can blindly copy this data.
  if (inGrammar) {
    CopyElementStart(localname, attrs);
    return;
  }

  // (1.2) Convert name string to integer value.
  XMLChToVXIchar elementName(localname);
  int rawElemType;
  bool conversionFailed = !ConvertElement(elementName.c_str(), rawElemType);

  // (1.3) Copy almost anything inside a <prompt>.
  if (explicitPrompt) {
    if (conversionFailed) {
      CopyElementStart(localname, attrs);
      return;
    }

    switch (rawElemType) {
    // elements that could be children of <foreach> but cannot 
    // be converted to SSML text
    case NODE_AUDIO:
    case NODE_VALUE:
    case NODE_ENUMERATE:
    case NODE_FOREACH:
    case NODE_MARK:
    case NODE_ASSIGN:
    case NODE_EXIT:
    case NODE_LOG:
    case NODE_IF:
    case NODE_ELSE:
    case NODE_ELSEIF:
    case NODE_CLEAR:
    case NODE_DATA:
    case NODE_DISCONNECT:
    case NODE_GOTO:
    case NODE_SUBMIT:
    case NODE_VAR:
    case NODE_RETURN:
    case NODE_SCRIPT:
    case NODE_REPROMPT:
    case NODE_THROW:
      break;

    // ignore nested prompts
    case NODE_PROMPT:
      explicitPrompt++;
      return;

    // all other elements in an explicit prompt get
    // converted to SSML text now
    default:
      CopyElementStart(localname, attrs);
      return;
    }
  }

  // (1.4) Print errors for all other conversion failures.
  if (conversionFailed) {
    vxistring temp(L"unrecognized element - ");
    temp += elementName.c_str();
    ParseException(temp.c_str());
  }

  // (2) Check for ignored nodes and do version number processing.

  if (rawElemType == PRIV_ELEM_METADATA) {
    ignoreDepth = 1;
    return;
  }

  // (2.1) Catch illegal nodes and do version number processing.  Prompt and
  //       grammar copying flags are set in the same pass.

  switch (rawElemType) {
  case DEFAULTS_ROOT:
    version = 2.0f;
    break;
  case NODE_VXML:
    {
      for (unsigned int index = 0; index < attrs.getLength(); ++index) {
        if (!Compare(attrs.getLocalName(index), L"version")) continue;
        const XMLCh * attributeValue = attrs.getValue(index);
        if (Compare(attributeValue, L"2.0")) version = 2.0f;
        else if (Compare(attributeValue, L"2.1")) version = 2.1f;
        else ParseException(L"unsupported vxml version");
        break;
      }
    }
    break;
  case NODE_GRAMMAR:      // Copy grammar contents.
    inGrammar = true;
    copyDepth = 0;
    break;

  case NODE_PROMPT:
    // if inside a foreach element, simply ignore prompt elements
    if (foreachImplicit) {
      return;
    }

    // keep track of the number of explicit prompt elements we
    // reach since a foreach element could follow, which in
    // turn could contain prompt elements.
    explicitPrompt++;
    if (explicitPrompt == 1)
      copyDepth = 0;
    break;

  case NODE_SCRIPT:
  case NODE_CHOICE:       // These may have PCDATA which is not a prompt
  case NODE_OPTION:
  case NODE_LOG:   
    pcdataImpliesPrompt = false;
    break;
  default:
    break;
  }

  // (2.2) Is this the beginning or end of an implicit prompt?

  switch (rawElemType) {
  // these will start an implicit prompt, but cannot be converted
  // to SSML text because they must be evaluated during runtime
  case NODE_AUDIO:
  case NODE_VALUE:        // NOTE: A <value> may also appear in <log>.
  case NODE_ENUMERATE:
  case NODE_MARK:
    if (pcdataImpliesPrompt && !implicitPrompt && !explicitPrompt)
      StartImplicitPrompt();
    break;

  // a foreach that isn't within a <prompt> will start an
  // implicit prompt
  case NODE_FOREACH:
    if (implicitPrompt)
      foreachImplicit = true;
    else if (!implicitPrompt && !explicitPrompt) {
      foreachImplicit = true;
      StartImplicitPrompt();
    }
    break;

  // SSML elements will trigger an implicit prompt, and get
  // converted to SSML text now
  case PRIV_ELEM_BREAK:   
  case PRIV_ELEM_EMPHASIS:
  case PRIV_ELEM_PARAGRAPH:
  case PRIV_ELEM_PHONEME:
  case PRIV_ELEM_PROSODY:
  case PRIV_ELEM_SAYAS:
  case PRIV_ELEM_SENTENCE:
  case PRIV_ELEM_VOICE:
    if (!implicitPrompt)
      StartImplicitPrompt();
    CopyElementStart(localname, attrs);
    return;

  // executable content that shouldn't end an implicit prompt if
  // if within a foreach
  case NODE_ASSIGN:
  case NODE_EXIT:
  case NODE_LOG:
  case NODE_IF:
  case NODE_ELSE:
  case NODE_ELSEIF:
  case NODE_CLEAR:
  case NODE_DATA:
  case NODE_DISCONNECT:
  case NODE_GOTO:
  case NODE_SUBMIT:
  case NODE_VAR:
  case NODE_RETURN:
  case NODE_SCRIPT:
  case NODE_REPROMPT:
  case NODE_THROW:
    if (foreachImplicit)
      break;
	// fall through intentional
  default:
    if (implicitPrompt) {
      if (copyDepth == 0)
        EndImplicitPrompt();
      else
        ParseException(L"illegal prompt content");
    }
    break;
  }

  // (2.4) Convert nodes as necessary.

  VXMLElementType elemType = VXMLElementType(rawElemType);

  switch (rawElemType) {
  case PRIV_ELEM_ERROR:
  case PRIV_ELEM_HELP:
  case PRIV_ELEM_NOINPUT:
  case PRIV_ELEM_NOMATCH:
    elemType = NODE_CATCH;
    break;
  default:
    break;
  }

  if (elemType > (enum VXMLElementType) PRIV_ELEM_RangeStart) {
    vxistring temp(L"internal error for element - ");
    temp += elementName.c_str();
    ParseException(temp.c_str());
  }

  // (3) Create new element.

  try {
    hasContent = (elemType != NODE_SCRIPT && elemType != NODE_GRAMMAR);
    doc->StartElement(elemType, documentLevel);
  }
  catch (const VXMLDocumentModel::OutOfMemory &) {
    ParseException(L"unable to allocate memory for element");
  }
  catch (const VXMLDocumentModel::InternalError &) {
    ParseException(L"corrupted document tree; unable to add element");
  }

  // (4) Add attributes to element.

  int attrGroup1 = 0; 
  int attrGroup2 = 0; 
  int attrGroup3 = 0; 

  // (4.1) Do special handling for converted nodes.
  switch (rawElemType) {
  case PRIV_ELEM_ERROR:
    ProcessNodeAttribute(elemType, ATTRIBUTE_EVENT, L"error");
    break;
  case PRIV_ELEM_HELP:
    ProcessNodeAttribute(elemType, ATTRIBUTE_EVENT, L"help");
    break;
  case PRIV_ELEM_NOINPUT:
    ProcessNodeAttribute(elemType, ATTRIBUTE_EVENT, L"noinput");
    break;
  case PRIV_ELEM_NOMATCH:
    ProcessNodeAttribute(elemType, ATTRIBUTE_EVENT, L"nomatch");
    break;
  default:
    break;
  }

  bool foundResourceBaseURI  = false;
  bool foundResourceBaseLang = false;

  for (unsigned int index = 0; index < attrs.getLength(); ++index) {
    // (4.2) Convert string to integer.

    int attrType;
    XMLChToVXIchar attributeName(attrs.getLocalName(index));

    if (Compare(attrs.getQName(index), L"xml:lang")) {
      attrType = ATTRIBUTE_XMLLANG;     
    }
    else if (!ConvertAttribute(attributeName.c_str(), attrType)) {
      vxistring temp(L"unrecognized attribute - ");
      temp += attributeName.c_str();
      ParseException(temp.c_str());
    }

    // (4.3) Impose attribute restrictions not representable in schema 1.0.

    switch (elemType) {
    case NODE_AUDIO:
      if (attrType == ATTRIBUTE_SRC)          ++attrGroup1;
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      break;
    case NODE_DATA:
      if (attrType == ATTRIBUTE_SRC)          ++attrGroup1;
      if (attrType == ATTRIBUTE_SRCEXPR)      ++attrGroup1;
      break;
    case NODE_CHOICE:
    case NODE_LINK:
      if (attrType == ATTRIBUTE_NEXT)         ++attrGroup1;
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      if (attrType == ATTRIBUTE_EVENT)        ++attrGroup1;
      if (attrType == ATTRIBUTE_EVENTEXPR)    ++attrGroup1;
      // and...
      if (attrType == ATTRIBUTE_MESSAGE)      ++attrGroup2;
      if (attrType == ATTRIBUTE_MESSAGEEXPR)  ++attrGroup2;
      break;
    case NODE_EXIT:
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      if (attrType == ATTRIBUTE_NAMELIST)     ++attrGroup1;
      break;
    case NODE_GOTO:
      if (attrType == ATTRIBUTE_NEXT)         ++attrGroup1;
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      if (attrType == ATTRIBUTE_NEXTITEM)     ++attrGroup1;
      if (attrType == ATTRIBUTE_EXPRITEM)     ++attrGroup1;
      break;
    case NODE_GRAMMAR:
      if (attrType == ATTRIBUTE_SRC){
         contentForbidden = true;
         ++attrGroup1;
      }
      if (attrType == ATTRIBUTE_SRCEXPR){
         contentForbidden = true;
         ++attrGroup1;
      }
      break;
    case NODE_MARK:
      if (attrType == ATTRIBUTE_NAME)         ++attrGroup1;
      if (attrType == ATTRIBUTE_NAMEEXPR)     ++attrGroup1;
      break;
    case NODE_META:
      if (attrType == ATTRIBUTE_NAME)         ++attrGroup1;
      if (attrType == ATTRIBUTE_HTTP_EQUIV)   ++attrGroup1;
      break;
    case NODE_PARAM:
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      if (attrType == ATTRIBUTE_VALUE)        ++attrGroup1;
      break;
    case NODE_RETURN:
      if (attrType == ATTRIBUTE_EVENT)        ++attrGroup1;
      if (attrType == ATTRIBUTE_EVENTEXPR)    ++attrGroup1;
      if (attrType == ATTRIBUTE_NAMELIST)     ++attrGroup1;
      // and...
      if (attrType == ATTRIBUTE_MESSAGE)      ++attrGroup2;
      if (attrType == ATTRIBUTE_MESSAGEEXPR)  ++attrGroup2;
      break;
    case NODE_SCRIPT:
      if (attrType == ATTRIBUTE_SRC){
         contentForbidden = true;
         ++attrGroup1;
      }
      if (attrType == ATTRIBUTE_SRCEXPR){
         contentForbidden = true;
         ++attrGroup1;
      }
      break;
    case NODE_SUBDIALOG:
      if (attrType == ATTRIBUTE_SRC)          ++attrGroup1;
      if (attrType == ATTRIBUTE_SRCEXPR)      ++attrGroup1;
      break;
    case NODE_SUBMIT:
      if (attrType == ATTRIBUTE_NEXT)         ++attrGroup1;
      if (attrType == ATTRIBUTE_EXPR)         ++attrGroup1;
      break;
    case NODE_THROW:
      if (attrType == ATTRIBUTE_EVENT)        ++attrGroup1;
      if (attrType == ATTRIBUTE_EVENTEXPR)    ++attrGroup1;
      // and...
      if (attrType == ATTRIBUTE_MESSAGE)      ++attrGroup2;
      if (attrType == ATTRIBUTE_MESSAGEEXPR)  ++attrGroup2;
      break;
    case NODE_TRANSFER:
      if (attrType == ATTRIBUTE_DEST)         ++attrGroup1;
      if (attrType == ATTRIBUTE_DESTEXPR)     ++attrGroup1;
      // and...
      if (attrType == ATTRIBUTE_AAI)          ++attrGroup2;
      if (attrType == ATTRIBUTE_AAIEXPR)      ++attrGroup2;
      // and...
      if (attrType == ATTRIBUTE_BRIDGE)       ++attrGroup3;
      if (attrType == ATTRIBUTE_TYPE)         ++attrGroup3;
      break;
    }

    // Handle base URI resolution and xml:lang for <prompt> & <grammar>
    switch (elemType) {
    case NODE_PROMPT:
    case NODE_GRAMMAR: 
      if (attrType == ATTRIBUTE_XMLLANG)   foundResourceBaseLang = true;
      else if (attrType == ATTRIBUTE_BASE) foundResourceBaseURI  = true;
      else if (attrType == ATTRIBUTE_TYPE) {
        // set grammar type for this grammar element
        XMLChToVXIchar attributeValue(attrs.getValue(index));
        SetGrammarType(attributeValue.c_str());
      }
      break;
    case NODE_VXML:
      if (attrType == ATTRIBUTE_BASE) {
        XMLChToVXIchar attributeValue(attrs.getValue(index));
        SetBaseUri(attributeValue.c_str());
        doc->rootBaseURL = attributeValue.c_str();
        continue;
      }
      else if (attrType == ATTRIBUTE_XMLLANG) {
        XMLChToVXIchar attributeValue(attrs.getValue(index));
        documentLang = attributeValue.c_str();
        continue;
      }
      break;
    case DEFAULTS_ROOT:
      if (attrType == ATTRIBUTE_XMLLANG) {
        XMLChToVXIchar attributeValue(attrs.getValue(index));
        defaultLang = attributeValue.c_str();
        documentLang = defaultLang;
        continue;
      }
      break;
    }
    
    // (4.5) Handle internal values.
    XMLChToVXIchar attributeValue(attrs.getValue(index));
    ProcessNodeAttribute(elemType, attrType, attributeValue.c_str());
  }

  // Use the default language for this document if necessary.
  if (elemType == NODE_VXML && documentLang.empty()) {
    documentLang = defaultLang;
  }
      
  // Assign an xml:base and/or xml:lang if not already specified for every
  // <grammar>, <prompt>, and <field> (because of builtins).
  if (elemType == NODE_PROMPT || elemType == NODE_GRAMMAR ||
      elemType == NODE_FIELD  || elemType == NODE_MENU)
  {
    if (!foundResourceBaseURI) 
      doc->AddAttribute(ATTRIBUTE_BASE, baseUri.c_str());
    if (!foundResourceBaseLang) {
      doc->AddAttribute(ATTRIBUTE_XMLLANG, documentLang.c_str());      
    }
  }

  // (5) Generate errors for attribute restrictions.
  switch (elemType) {
  case NODE_AUDIO:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'src' or 'expr' must be specified");
    break;
  case NODE_DATA:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'src' or 'srcexpr' must be specified");
    break;
  case NODE_CHOICE:
  case NODE_LINK:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'next', 'expr', 'event' or 'eventexpr' "
                     L"must be specified");
    if (attrGroup2 > 1)
      ParseException(L"exactly one of 'message' or 'messageexpr' "
                     L"may be specified");
    break;
  case NODE_EXIT:
    if (attrGroup1 > 1)
      ParseException(L"exactly one of 'expr' or 'namelist' "
                     L"may be specified");
    break;
  case NODE_GOTO:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'next', 'expr', 'nextitem' or 'expritem'"
                     L" must be specified");
    break;
  case NODE_MARK:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'name' or 'nameexpr' "
                     L"must be specified");
    break;
  case NODE_META:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'name' or 'http-equiv' "
                     L"must be specified");
    break;
  case NODE_PARAM:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'expr' or 'value' must be specified");
    break;
  case NODE_RETURN:
    if (attrGroup1 > 1)
      ParseException(L"exactly one of 'event', 'eventexpr' or 'namelist' "
                     L"may be specified");
    if (attrGroup2 > 1)
      ParseException(L"exactly one of 'message' or 'messageexpr' "
                     L"may be specified");
    break;
  case NODE_GRAMMAR:
  case NODE_SCRIPT:
    if (attrGroup1 > 1)
      ParseException(L"exactly one of 'src' or 'srcexpr' must be specified");
    break;
  case NODE_SUBDIALOG:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'src' or 'srcexpr' must be specified");
    break;
  case NODE_SUBMIT:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'next' or 'expr' must be specified");
    break;
  case NODE_THROW:
    if (attrGroup1 != 1)
      ParseException(L"exactly one of 'event' or 'eventexpr' "
                     L"must be specified");
    if (attrGroup2 > 1)
      ParseException(L"exactly one of 'message' or 'messageexpr' "
                     L"may be specified");
    break;
  case NODE_TRANSFER:
    if (attrGroup1 > 1)
      ParseException(L"exactly one of 'dest' or 'destexpr' may be specified");
    if (attrGroup2 > 1)
      ParseException(L"exactly one of 'aai' or 'aaiexpr' may be specified");
    if (attrGroup3 > 1)
      ParseException(L"exactly one of 'bridge' or 'type' may be specified");
    break;
  }

  // (6) Verify the node.
  ProcessNodeFinal(elemType);
}


void DocumentConverter::endElement(const XMLCh* const uri,
                                   const XMLCh* const localname,
                                   const XMLCh* const qname)
{
  // (0) Just ignore this?
  if (ignoreDepth > 0) {
    --ignoreDepth;
    return;
  }

  // (1) Copy this element or look it up?

  // (1.1) Inside a <grammar>, we can blindly copy this data.
  if (inGrammar && copyDepth == 0) inGrammar = false;
  if (inGrammar) {
    CopyElementClose(localname);
    return;
  }

  // (1.2) Convert name string to integer value.
  XMLChToVXIchar elementName(localname);
  int elemType;
  bool conversionFailed = !ConvertElement(elementName.c_str(), elemType);

  // (1.3) Handle elements inside a <prompt>.
  if (explicitPrompt) {
    if (conversionFailed) {
      CopyElementClose(localname);
      return;
    }

    switch (elemType) {
    // these elements don't get copied as text within a prompt
    case NODE_FOREACH:
    case NODE_AUDIO:
    case NODE_VALUE:
    case NODE_ENUMERATE:
    case NODE_MARK:
    case NODE_ASSIGN:
    case NODE_EXIT:
    case NODE_LOG:
    case NODE_IF:
    case NODE_ELSE:
    case NODE_ELSEIF:
    case NODE_CLEAR:
    case NODE_DATA:
    case NODE_DISCONNECT:
    case NODE_GOTO:
    case NODE_SUBMIT:
    case NODE_VAR:
    case NODE_RETURN:
    case NODE_SCRIPT:
    case NODE_REPROMPT:
    case NODE_THROW:
      break;

    case NODE_PROMPT:
      if (foreachImplicit) {
        return;
      }

      explicitPrompt--;
      if (explicitPrompt > 0) {
        return;
      }
      doc->AddContent(L"</speak>", 8);
      break;

    // since we are in an explicit prompt, this handles
    // closing SSML elements
    default:
      CopyElementClose(localname);
      return;
    }
  }

  // (1.4) Print errors for all other conversion failures.
  if (conversionFailed) {
    vxistring temp(L"unrecognized element - ");
    temp += elementName.c_str();
    ParseException(temp.c_str());
  }

  // (2) Check for ending of implicit prompts.

  if (implicitPrompt) {
    switch (elemType) {
    // none of these elements should end an implicit prompt
    // because we don't know what's to follow
    case NODE_AUDIO:
    case NODE_VALUE:
    case NODE_ENUMERATE:
      break;

    // These implicitly imply a prompt and the contents
    // are done converting to text
    case PRIV_ELEM_BREAK:  
    case PRIV_ELEM_EMPHASIS:
    case PRIV_ELEM_PARAGRAPH:
    case PRIV_ELEM_PHONEME:
    case PRIV_ELEM_PROSODY:
    case PRIV_ELEM_SAYAS:
    case PRIV_ELEM_SENTENCE:
    case PRIV_ELEM_VOICE:
      CopyElementClose(localname);
      return;

    // if we're in an implicit prompt and encounter a prompt element,
    // it's because we're withing a foreach... so we just ignore
    // the prompt element.
	case NODE_PROMPT:
		return;

    // we're out of the foreach implicit prompt
    case NODE_FOREACH:
      foreachImplicit = false;
	  break;

	// executable content that could be within a foreach
    case NODE_ASSIGN:
    case NODE_EXIT:
    case NODE_LOG:
    case NODE_IF:
    case NODE_ELSE:
    case NODE_ELSEIF:
    case NODE_CLEAR:
    case NODE_DATA:
    case NODE_DISCONNECT:
    case NODE_GOTO:
    case NODE_SUBMIT:
    case NODE_MARK:
    case NODE_VAR:
    case NODE_RETURN:
    case NODE_SCRIPT:
    case NODE_REPROMPT:
    case NODE_THROW:
      if (foreachImplicit)
        break;
      // fall through intentional
    default:
      EndImplicitPrompt();
      break;
    }
  }

  // (3) Reverse special handling from startElement()

  switch (elemType) {
  case NODE_CHOICE:          // These may have PCDATA which is not a prompt
  case NODE_OPTION:
    doc->PruneWhitespace();
    pcdataImpliesPrompt = true;
  case NODE_LOG:
    pcdataImpliesPrompt = true;
    break;
  case NODE_SCRIPT:
    pcdataImpliesPrompt = true;
    // Intentional fall through!
  case NODE_GRAMMAR:
    if (contentForbidden ^ hasContent)
      contentForbidden = false;
    else
      ParseException(L"either the 'src' attribute or inlined content "
                     L"may be provided, not both");
    break;
  default:
    break;
  }

  // (4) This element is complete.

  try {
    doc->EndElement();
  }
  catch (const VXMLDocumentModel::InternalError &) {
    ParseException(L"corrupted document tree; unable to complete element");
  }
}


void DocumentConverter::characters(const XMLCh* const chars,
                                   const unsigned int length)
{
  // (0) Just ignore this?
  if (chars == NULL || length == 0 || ignoreDepth > 0) return;

  // (1) Ignore non-empty prompts consisting of pure whitespace
  unsigned int l;
  for (l = 0; l < length; ++l) {
    XMLCh c = chars[l];
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
    break;
  }
  if (l == length) return;

  // (2) Is this an implicit prompt?
  if (pcdataImpliesPrompt && !explicitPrompt && !implicitPrompt && !inGrammar)
    StartImplicitPrompt();

  // (3) Add content to the document model.
  try {
    XMLChToVXIchar data(chars);
    hasContent = true;
    
    if( (inGrammar && (GetGrammarType().empty() || GetGrammarType() == L"application/srgs+xml")) ||
        (explicitPrompt || implicitPrompt)
        ) 
    {
      if (!inCDATA)
        doc->AddContent(data.c_str(), wcslen(data.c_str()));
      else {
        doc->AddContent(L"<![CDATA[", 9);
        doc->AddContent(data.c_str(), wcslen(data.c_str()));
        doc->AddContent(L"]]>", 3);
      }
    } else {
      doc->AddContent(data.c_str(), wcslen(data.c_str()));
    }
  }
  catch (const VXMLDocumentModel::OutOfMemory &) {
    ParseException(L"unable to allocate memory for content");
  }
  catch (const VXMLDocumentModel::InternalError &) {
    ParseException(L"corrupted document tree; unable to add content");
  }
}


void DocumentConverter::ignorableWhitespace(const XMLCh* const chars,
                                            const unsigned int l)
{ }


void DocumentConverter::processingInstruction(const XMLCh* const target,
                                              const XMLCh* const data)
{ }

//#############################################################################

void DocumentConverter::ProcessNodeAttribute(VXMLElementType elemType,
                                             int attrType,
                                             const VXIchar* const value)
{
  // This shouldn't ever happen, if it does, we'll just ignore it.
  if (value == NULL) return;

  switch (elemType) {
  case NODE_BLOCK:
  case NODE_INITIAL:
    if (attrType == ATTRIBUTE_NAME)
      namedControlItems.push_back(value);
  case NODE_FIELD:
  case NODE_OBJECT:
  case NODE_RECORD:
  case NODE_TRANSFER:
    // We associate a hidden interal name with this element.
    if (attrType == ATTRIBUTE_NAME) attrType = ATTRIBUTE__ITEMNAME;
    break;
  case NODE_CLEAR:
    if (attrType == ATTRIBUTE_NAMELIST && !value[0])
      ParseException(L"the namelist attribute on clear cannot be empty");
    break;
  case NODE_FORM:
  case NODE_MENU:
    // We associate a hidden interal name with this element.
    if (attrType == ATTRIBUTE_ID) attrType = ATTRIBUTE__ITEMNAME;
    break;
  case NODE_FILLED:
    if (doc->GetParentType() != NODE_FORM)
      ParseException(L"attributes valid on filled only at form level");
    if (attrType == ATTRIBUTE_NAMELIST && !value[0])
      ParseException(L"the namelist attribute on filled cannot be empty");
    if (attrType == ATTRIBUTE_NAMELIST){
      if ( vxistring( value ) != L"any")
        filledNameList.push_back(value);
    }
    break;
  case NODE_GRAMMAR:
    if (doc->GetParentType() != NODE_FORM && attrType == ATTRIBUTE_SCOPE)
      ParseException(L"the scope attribute is valid only on grammars at form "
                     L"level");
    break;
  case NODE_SUBDIALOG:
    // We associate a hidden interal name with this element.
    if (attrType == ATTRIBUTE_NAME) attrType = ATTRIBUTE__ITEMNAME;
    // modal was dropped in 2.0.
    if (attrType == ATTRIBUTE_MODAL && version != 1.0f)
      ParseException(L"the modal attribute on subdialog was dropped after "
                     L"1.0");
    break;
  case NODE_VALUE:
    if (attrType == ATTRIBUTE_EXPR) break;

    if (doc->GetParentType() == NODE_LOG)
      ParseException(L"only the expr attribute is valid on value elements "
                     L"within a log element");
    break;
  case NODE_VXML:
    // version is processed elsewhere and may be ignored.
    if (attrType == ATTRIBUTE_VERSION) return;
    if (attrType == PRIV_ATTRIB_SCHEMALOC) return;
    break;
  default:
    break;
  }

  // (4.3) Add the attribute to the element.

  if (attrType > PRIV_ATTRIB_RangeStart)
    ParseException(L"internal error during attribute processing");

  doc->AddAttribute(VXMLAttributeType(attrType), value);
}


void DocumentConverter::ProcessNodeFinal(VXMLElementType elemType)
{
  // Convert attributes.
  vxistring attr;

  switch (elemType) {
  case NODE_BLOCK:
  case NODE_FIELD:
  case NODE_FORM:
  case NODE_INITIAL:
  case NODE_MENU:
  case NODE_OBJECT:
  case NODE_RECORD:
  case NODE_SUBDIALOG:
  case NODE_TRANSFER:
    // Name the 'unnamed' elements as neccessary.
    if (!doc->GetAttribute(ATTRIBUTE__ITEMNAME, attr)) {
      vxistring variable;
      VXMLDocumentModel::CreateHiddenVariable(variable);
      doc->AddAttribute(ATTRIBUTE__ITEMNAME, variable);
    }
    break;
  case NODE_FILLED:
    if (!doc->GetAttribute(ATTRIBUTE_MODE, attr))
      doc->AddAttribute(ATTRIBUTE_MODE, L"all");
    break;
  case NODE_VXML:
  case DEFAULTS_ROOT:
    VXMLDocumentModel::CreateHiddenVariable(attr);
    doc->AddAttribute(ATTRIBUTE__ITEMNAME, attr);
    break;
  default:
    break;
  }

  // Generate DTMF sequences for <choice> elements in <menu> if necessary.
  if (elemType == NODE_MENU) {
    if (doc->GetAttribute(ATTRIBUTE_DTMF, attr) && attr == L"true")
      choiceNumber = 1;
    else
      choiceNumber = 0;
  }

  if (elemType == NODE_CHOICE && choiceNumber > 0 && choiceNumber < 10 &&
      !doc->GetAttribute(ATTRIBUTE_DTMF, attr))
  {
    std::basic_stringstream<VXIchar> countString;
    countString << choiceNumber;
    doc->AddAttribute(ATTRIBUTE_DTMF, countString.str());
    ++choiceNumber;
  }

  if (elemType == NODE_PROMPT) {
    vxistring base, lang;
    doc->GetAttribute(ATTRIBUTE_BASE, base);
    doc->GetAttribute(ATTRIBUTE_XMLLANG, lang);
    // remove all irrelavant part from the base uri
    TrimUriBase(base);
    
    doc->AddContent(L"<?xml version='1.0'?><speak version='1.0' xmlns="
                    L"'http://www.w3.org/2001/10/synthesis' xml:base='", 96);
    doc->AddContent(base.c_str(), base.length());
    doc->AddContent(L"' xml:lang='", 12);
    doc->AddContent(lang.c_str(), lang.length());
    doc->AddContent(L"'>", 2);
    doc->MarkSSMLHeader();
  }
}


void DocumentConverter::CopyElementStart(const XMLCh* const localname,
                                         const Attributes &attrs)
{
  ++copyDepth;

  doc->AddContent(L"<", 1);

  XMLChToVXIchar data(localname);
  doc->AddContent(data.c_str(), wcslen(data.c_str()));

  for (unsigned int index = 0; index < attrs.getLength(); ++index) {
    doc->AddContent(L" ", 1);

    // Determine the attribute name
    if (Compare(attrs.getQName(index), L"xml:lang"))
      doc->AddContent(L"xml:lang", 8); 
    else {
      XMLChToVXIchar name(attrs.getLocalName(index));
      doc->AddContent(name.c_str(), wcslen(name.c_str()));
    }
      
    doc->AddContent(L"=\"", 2);

    XMLChToVXIchar value(attrs.getValue(index));

    unsigned int start = 0;
    unsigned int pos = 0;
    VXIchar c;
    // We need to escape three characters: (<,>,&) -> (&lt;,&gt;,&amp;)
    do {
      c = *(value.c_str() + pos);
      if (c == L'<') {
        if (start != pos)
           doc->AddContent(value.c_str() + start, pos - start);
        start = pos + 1;
        doc->AddContent(L"&lt;", 4);
      }
      else if (c == L'>') {
        if (start != pos)
           doc->AddContent(value.c_str() + start, pos - start);
        start = pos + 1;
        doc->AddContent(L"&gt;", 4);
      }
      else if (c == L'&') {
        if (start != pos)
           doc->AddContent(value.c_str() + start, pos - start);
        start = pos + 1;
        doc->AddContent(L"&amp;", 5);
      }
      
      ++pos;
    } while (c != L'\0');

    if (pos > start)
      doc->AddContent(value.c_str() + start, pos - start - 1); // -1 to kill off final \0

    doc->AddContent(L"\"", 1);
  }

  doc->AddContent(L">", 1);
}


void DocumentConverter::CopyElementClose(const XMLCh* const localname)
{
  doc->AddContent(L"</", 2);

  XMLChToVXIchar data(localname);
  doc->AddContent(data.c_str(), wcslen(data.c_str()));

  doc->AddContent(L">", 1);

  --copyDepth;
}


void DocumentConverter::StartImplicitPrompt()
{
  implicitPrompt = true;
  doc->StartElement(NODE_PROMPT, documentLevel);
  
  vxistring tmpBaseUri = baseUri;
  TrimUriBase(tmpBaseUri);
  doc->AddContent(L"<?xml version='1.0'?><speak version='1.0' xmlns="
                  L"'http://www.w3.org/2001/10/synthesis' xml:base='", 96);
  doc->AddContent(tmpBaseUri.c_str(), tmpBaseUri.length());
  doc->AddContent(L"' xml:lang='", 12);
  doc->AddContent(documentLang.c_str(), documentLang.length());
  doc->AddContent(L"'>", 2);
  doc->MarkSSMLHeader();
  
  copyDepth = 0;
}


void DocumentConverter::EndImplicitPrompt()
{
  doc->AddContent(L"</speak>", 8);
  doc->EndElement();
  implicitPrompt = false;
  foreachImplicit = false;
}



void DocumentConverter::comment( const XMLCh* const chars, const unsigned int length )
{
}

void DocumentConverter::startCDATA ()
{
  inCDATA = true;
}

void DocumentConverter::endCDATA ()
{
  inCDATA = false;
}

void DocumentConverter::startEntity (const XMLCh* const name)
{
}

void DocumentConverter::endEntity (const XMLCh* const name)
{
}

void DocumentConverter::startDTD
(
    const   XMLCh* const    name
    , const   XMLCh* const    publicId
    , const   XMLCh* const    systemId
)
{
}

void DocumentConverter::endDTD ()
{
}

