
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

#ifndef HAVE_XERCES
#error Need Apache Xerces to build the VoiceXML interpreter
#endif
#include <sax2/SAX2XMLReader.hpp>
#include <framework/XMLDocumentHandler.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMDocument.hpp>
using namespace xercesc;

#include "VXIvalue.h"
#include <list>

class DocumentConverter;
class VXMLDocument;
class SimpleLogger;
extern "C" struct VXIcacheInterface;
extern "C" struct VXIinetInterface;

class DocumentParser : private XMLDocumentHandler {
public:
  // One time initialization of DocumentParser interface.
  //
  // Returns: True - initialization succeeded.
  //          False - initialization failed.
  static bool Initialize(unsigned int cacheSize);

  // One time cleanup of DocumentParser interface.
  static void Deinitialize();

  DocumentParser();   // May throw VXIException::OutOfMemory
  virtual ~DocumentParser();

public:
  // This retrieves a given URI and converts its contents into a VXMLElement
  // tree.  The returned 'document' will contain only the contents of the
  // <vxml> section.  Thus, only documents containing <vxml> are supported.
  //
  // Returns: -1 Out of memory?
  //           0 Success
  //           1 Invalid parameter
  //           2 Unable to open URI
  //           3 Unable to read from URI
  //           4 Unable to parse contents of URI
  int FetchDocument(const VXIchar * uri, const VXIMapHolder & properties,
                    VXIinetInterface * inet, VXIcacheInterface * cache,
                    SimpleLogger & log, VXMLDocument & document,
                    VXIMapHolder & docProperties, 
                    bool isDefaults = false, bool isRootApp = false,
                    VXIbyte **content = NULL, VXIulong *size = 0);

  // Returns: -1 Out of memory?
  //           0 Success
  //           1 Invalid parameter
  //           2 Unable to open URI
  //           3 Unable to read from URI
  //           4 Unable to parse contents of URI
  int FetchXML(const VXIchar * uri,
	       const VXIMapHolder & fetchobj,
	       VXIMapHolder & fetchStatus, 
	       VXIinetInterface * inet,
	       SimpleLogger & log,
	       DOMDocument **doc,
	       bool parse_doc);

  // Returns: -1 Out of memory?
  //           0 Success
  //           1 Invalid parameter, such as unsupported encoding
  //           2 Unable to open URI
  //           3 Unable to read from URI
  static int FetchContent(const VXIchar * uri, const VXIMapHolder & properties,
                          VXIMapHolder & fetchInfo,
                          VXIinetInterface * inet, SimpleLogger & log,
                          const vxistring & encoding, vxistring & content);

private:
  // FetchBuffer will create a memory buffer containing the contents of the
  // URI.  ReleaseBuffer must be called by the consumer to cleanup this memory.
  //
  // Returns: -1 Out of memory?
  //           0 Success
  //           1 Invalid parameter
  //           2 Unable to open URI
  //           3 Unable to read from URI
  //           4 Unsupported encoding
  static int FetchBuffer(const VXIchar * uri, const VXIMapHolder & properties,
                         VXIMapHolder & fetchStatus,
                         VXIinetInterface * inet, SimpleLogger & log,
                         const VXIbyte * & buffer, VXIulong & bufferSize,
                         vxistring & docURI, int parseVXMLDocument,
			 VXMLDocument * * document);

  static void ReleaseBuffer(const VXIbyte * & buffer);

  virtual void docCharacters
  (
      const   XMLCh* const    chars
      , const unsigned int    length
      , const bool            cdataSection
  ){}

  virtual void docComment( const XMLCh* const comment ){}
  virtual void docPI
  (
      const   XMLCh* const    target
      , const XMLCh* const    data
  ){}
  virtual void endDocument(){}
  virtual void endElement
  (
      const   XMLElementDecl& elemDecl
      , const unsigned int    uriId
      , const bool            isRoot
      , const XMLCh* const    prefixName = 0
  ){}
  virtual void endEntityReference
  (
      const   XMLEntityDecl&  entDecl
  ){}
  virtual void ignorableWhitespace
  (
      const   XMLCh* const    chars
      , const unsigned int    length
      , const bool            cdataSection
  ){}
  virtual void resetDocument(){ encoding.clear(); }
  virtual void startDocument(){}
  virtual void startElement
  (
      const   XMLElementDecl&         elemDecl
      , const unsigned int            uriId
      , const XMLCh* const            prefixName
      , const RefVectorOf<XMLAttr>&   attrList
      , const unsigned int            attrCount
      , const bool                    isEmpty
      , const bool                    isRoot
  ){}
  virtual void startEntityReference(const XMLEntityDecl& entDecl){}
  virtual void XMLDecl
  (
      const   XMLCh* const    versionStr
      , const XMLCh* const    encodingStr
      , const XMLCh* const    standaloneStr
      , const XMLCh* const    autoEncodingStr
  );

private:
  xercesc::SAX2XMLReader * parser;
  std::list<xercesc::DOMBuilder *> domParsers;
  DocumentConverter * converter;
  bool loadedVXML20;
  vxistring encoding;
};
