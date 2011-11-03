
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

#ifndef _VXIREC_UTILS
#define _VXIREC_UTILS
#include <util/PlatformUtils.hpp>       // Xerces headers
#include <util/TransService.hpp>
#include <parsers/SAXParser.hpp>
#include <framework/MemBufInputSource.hpp>
#include <sax2/XMLReaderFactory.hpp>
#include <sax/HandlerBase.hpp>
#include <sax/SAXParseException.hpp> // by DOMTreeErrorReporter
#include <sax/EntityResolver.hpp>    // by DTDResolver
#include <sax/ErrorHandler.hpp>      // by DOMTreeErrorReporter

using namespace xercesc;

#include <SWIprintf.h>
#include <VXIrecAPI.h>
#include <VXIvalue.h>
#include <list>
#include <string>
typedef std::basic_string<VXIchar> vxistring;

#ifdef __cplusplus
extern "C" {
#endif

// Constants for diagnostic logging tags
//
static const VXIunsigned DIAG_TAG_API         = 0;
static const VXIunsigned DIAG_TAG_RECOGNITION = 1;
static const VXIunsigned DIAG_TAG_GRAMMARS    = 2;
static const VXIunsigned DIAG_TAG_PARSE       = 4;
static const VXIunsigned DIAG_TAG_PROPERTY    = 5;

#define VXIREC_MODULE L"VXIrec"

class VXIrecWordList;
class GrammarSaxHandler;
class GrammarWordList;

typedef std::list<vxistring> STRINGLIST;
typedef struct VXIrecGrammarInfo {
  vxistring word;
  vxistring tag; 
  STRINGLIST taglist;   
  vxistring semantic;
} VXIrecGrammarInfo;

// Grammar info list
typedef std::list<VXIrecGrammarInfo> GRAMMARINFOLIST;

// Information after cracking JSGF grammar, may be more???
typedef struct JSGFInfo {
  vxistring versionStr;
  vxistring nameStr;
  vxistring publicStr;
  vxistring contentStr;
} JSGFInfo;

class VXIrecGrammar {
public:
  VXIrecGrammar() { }
  virtual ~VXIrecGrammar() { }

  virtual void SetEnabled(bool) = 0;
  // Sets the flag indicating whether or not the grammar is enabled for the
  // next recognition.

  virtual bool IsEnabled() const = 0;
  // Returns current enabled state of this grammar.
  
  virtual GRAMMARINFOLIST * GetGrammarInfoList() const = 0;
  
  virtual bool GetGrammarInfo(const VXIchar* input, 
                              VXIrecGrammarInfo ** gramInfo) const = 0;
  virtual VXIString * GetGrammarWords() const = 0;
  virtual bool IsDtmf() const = 0;

};


class VXIrecData {
public:
  VXIrecData(VXIlogInterface *log, VXIinetInterface *inet);
  virtual ~VXIrecData();

  static VXIunsigned diagLogBase;
  static int Initialize(VXIlogInterface* log, VXIunsigned diagBase);
  static int ShutDown();

  // Release all existing grammars.
  void Clear();

  // Add a new grammar.  The VXIrecData assumes ownership of the grammar.
  void AddGrammar(VXIrecGrammar *g);

  // Removed an exising grammar.  The corresponding grammar is destroyed.
  void FreeGrammar(VXIrecGrammar *g);

  void ActivateGrammar(VXIrecGrammar *g);
  void DeactivateGrammar(VXIrecGrammar *g);

  int GetActiveCount() const;
  VXIString *GetActiveText() const;
  
  // Parse SRGS grammar: very simple srgs grammar processing: i.e: no ref rule, etc.
  VXIrecGrammar * ParseSRGSGrammar(const vxistring & srgsgram, 
                                   const VXIMap    * properties,
                                   bool isdtmf = false);

  // Return true if grammar found for the input and the nlsml is constructed
  bool ConstructNLSMLForInput(const VXIchar* input, vxistring & nlsmlresult);

  
  // Conversion functions
  bool JSGFToSRGS(const vxistring & incoming,
                  vxistring & result, 
                  const VXIMap* props);
  void ConvertJSGFType(const vxistring & dataIn, JSGFInfo & info);

  bool OptionToSRGS(const VXIMap    * properties,
                    const VXIVector * gramChoice,
                    const VXIVector * gramValue,
                    const VXIVector * gramAcceptance,
                    const VXIbool     isDTMF,
                    vxistring & srgs);
  
  // Process semantic interpretation based on specific implementation, i.e: SSFT
    bool ProcessSemanticInterp(vxistring & result, const VXIrecGrammarInfo *ginfo, const VXIchar *input); 

  // Logging functions
  VXIlogResult LogError(VXIunsigned errorID, const VXIchar *format, ...) const;
  VXIlogResult LogDiag(VXIunsigned tag, const VXIchar *subtag, const VXIchar *format, ...) const;

  VXIlogInterface * GetLog() const { return log; }
  void ShowPropertyValue(const VXIMap *properties);
  void ShowPropertyValue(const VXIchar *key,
                         const VXIValue *value, VXIunsigned PROP_TAG);     

  bool FetchContent(
    const VXIchar *  name, 
    const VXIMap  *  properties,
    VXIbyte       ** result,
    VXIulong      &  read );

private:
  VXIlogInterface *log;
  VXIinetInterface *inet;
  typedef std::list<VXIrecGrammar *> GRAMMARS;
  GRAMMARS grammars;
  GRAMMARS activeGrammars;
  
  GrammarSaxHandler *xmlHandler;
  SAXParser         *parser;  

};

typedef enum NODE_TYPE {
  GRAMMAR_NODE,
  META_NODE,
  ITEM_NODE,
  UNKNOWN_NODE      /* always last */   
} NODE_TYPE;

#define GRAMMAR       L"grammar"
#define META          L"meta"
#define ITEM          L"item"
#define TAG           L"tag"
#define NAME          L"name"
#define CONTENT       L"content"

class GrammarSaxHandler : public HandlerBase 
{
public:
  GrammarSaxHandler(VXIlogInterface *l);
  virtual ~GrammarSaxHandler();
  void startElement(const XMLCh* const, AttributeList&);
  void characters(const XMLCh* const chars, const unsigned int length);
  void endElement(const XMLCh* const name);

  void warning(const SAXParseException& exception) 
  { processError(exception, L"Warning"); }
  void error(const SAXParseException& exception) 
  { processError(exception, L"Error"); }
  void fatalError(const SAXParseException& exception) 
  { processError(exception, L"Fatal Error"); }

  // Create a new grammar info list
  void CreateGrammarInfoList()
  {
    isDTMFGram = false;
    grammarInfo.word = L"";
    grammarInfo.tag = L"";
    grammarInfo.semantic = L"";  
    grammarInfoList = new GRAMMARINFOLIST();      
  }
  
  // Destroy grammar info list
  void DestroyGrammarInfoList()
  {
    if( grammarInfoList ) {
      delete grammarInfoList;
      grammarInfoList = NULL;
    }
  }
  
  // Release the ownership of grammar info list
  GRAMMARINFOLIST * AcquireGrammarInfoList()
  {
    GRAMMARINFOLIST * ret = grammarInfoList;
    grammarInfoList = NULL;
    return ret;
  }
  
  bool isDTMFGrammar() const { return isDTMFGram; }
private:
  void processError(const SAXParseException& exception, 
                    const VXIchar* errType);
  
  // Logging functions
  VXIlogResult LogError(VXIunsigned errorID, const VXIchar *format, ...) const;
  VXIlogResult LogDiag(VXIunsigned offset, const VXIchar *subtag, 
                       const VXIchar *format, ...) const;
  
private:
  GRAMMARINFOLIST * grammarInfoList;
  VXIrecGrammarInfo       grammarInfo;

  bool processTag;
  bool isDTMFGram;
  NODE_TYPE nodeType;
  VXIlogInterface* log;
};  


#ifdef __cplusplus
}
#endif

#endif /* include guard */
