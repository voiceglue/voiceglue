
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

// Xerces related
#ifndef HAVE_XERCES
#error Need Apache Xerces to build the VoiceXML interpreter
#endif
#include <sax2/ContentHandler.hpp>
#include <sax2/LexicalHandler.hpp>
using namespace xercesc;

#include "VXML.h"
#include "DocumentModel.hpp"
#include <string>
#include <list>

typedef std::basic_string<VXIchar> vxistring;

class VXMLDocumentRep;
class VXMLDocument;

class DocumentConverter : public ContentHandler, public LexicalHandler {
public:
  static bool Initialize();
  // One time initialization.
  //
  // Returns: True - initialization succeeded.
  //          False - initialization failed.

  static void Deinitialize();
  // One time cleanup of DocumentParser interface.

public:
  DocumentConverter();
  virtual ~DocumentConverter();

  virtual void startDocument();
  virtual void endDocument();
  virtual void setDocumentLocator(const Locator* const locator);

  virtual void startElement(const XMLCh* const uri,
                            const XMLCh* const localname,
                            const XMLCh* const qname,
                            const Attributes&  attrs);
  virtual void endElement(const XMLCh* const uri,
                          const XMLCh* const localname,
                          const XMLCh* const qname);

  virtual void characters(const XMLCh* const chars,
                          const unsigned int length);
  virtual void ignorableWhitespace(const XMLCh* const chars,
                                   const unsigned int length);

  virtual void processingInstruction(const XMLCh* const target,
                                     const XMLCh* const data);

  virtual void startPrefixMapping(const XMLCh* const prefix,
                                  const XMLCh* const uri)
  { }

  virtual void endPrefixMapping(const XMLCh* const prefix)
  { }

  virtual void skippedEntity(const XMLCh* const name)
  { }

public:
    virtual void comment(
        const   XMLCh* const    chars
        , const unsigned int    length
    );

    virtual void endCDATA ();
    virtual void endDTD ();
    virtual void endEntity (const XMLCh* const name);
    virtual void startCDATA ();
    virtual void startDTD
    (
        const   XMLCh* const    name
        , const   XMLCh* const    publicId
        , const   XMLCh* const    systemId
    );
    virtual void startEntity (const XMLCh* const name);

public:

  VXMLDocumentRep * GetDocument();
  // This function releases the document representation to the caller.
  
  float GetXMLVersion(void) { return version; }
  
  void ResetDocument();

  // Get/Set Base URL
  void SetBaseUri(const VXIchar* const base);  
  const vxistring & GetBaseUri(void) { return baseUri; }
  void RestoreBaseURLFromCache(const VXMLDocument & doc);
  
  // Restore default language id if read from cache
  void RestoreDefaultLangFromCache(const VXMLDocument & doc);
  void GetDefaultLang(vxistring & dlang)
  { dlang = defaultLang; }
  
  // Get/Set Grammar type
  void SetGrammarType(const VXIchar* const grammartype)
  { if( grammartype != NULL ) grammarType = grammartype; else grammarType=L""; } 
  const vxistring & GetGrammarType(void) { return grammarType; }

  void SetDocumentLevel(DocumentLevel dlevel)
  { documentLevel = dlevel; }

private:
  void ParseException(const VXIchar * message) const;

  void ProcessNodeAttribute(VXMLElementType elemType, int attrType,
                            const VXIchar* const value);

  void ProcessNodeFinal(VXMLElementType elemType);

  bool IsIgnorable(int elemType);

  void CopyElementStart(const XMLCh* const localname, const Attributes &attrs);
  void CopyElementClose(const XMLCh* const localname);
  void StartImplicitPrompt();
  void EndImplicitPrompt();

  const Locator * locator;
  VXMLDocumentRep * doc;
  float version;
  vxistring baseUri;
  vxistring grammarType;
  vxistring defaultLang;
  vxistring documentLang;
  
  // Document scope
  DocumentLevel documentLevel;
    
  // Ignoring for <metadata>
  int ignoreDepth;

  // Used to detect attribute vs. content conflicts.
  bool contentForbidden;    // Is content forbidden within this element?
  bool hasContent;          // Did content appear.

  // Menu <choice> numbering
  int choiceNumber;         // This is used to number choices in a menu.

  // For prompt handling, only one of implicitPrompt or explicitPrompt are
  // 'true'.  In the case of a <foreach> that triggers an implicitPrompt,
  // or is otherwise within an implicitPromt, the foreachImplicit will be
  // 'true'.
  bool implicitPrompt;      // Is this an implicit prompt (see hasValidPCDATA)?
  int  explicitPrompt;      // Are we inside a <prompt>?
  bool foreachImplicit;  

  bool inGrammar;           // Are we inside a <grammar>?
  int copyDepth;            // How deeply are we copying elements?
  bool pcdataImpliesPrompt; // False indicates PCDATA is valid and not SSML

  bool inCDATA;             // true if currently within a CDATA section

  std::list<vxistring> namedControlItems;
  std::list<vxistring> filledNameList;
};
