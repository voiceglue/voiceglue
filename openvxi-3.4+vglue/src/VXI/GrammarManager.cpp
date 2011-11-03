
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

#include "GrammarManager.hpp"

#include "SimpleLogger.hpp"
#include "CommonExceptions.hpp"
#include "PropertyList.hpp"
#include "VXML.h"
#include "DocumentModel.hpp"
#include <sstream>

//#############################################################################
// Grammar description class
//#############################################################################

const VXIchar * const GrammarManager::DTMFTerm      = L"@dtmfterm";
const VXIchar * const GrammarManager::FinalSilence  = L"@finalsilence";
const VXIchar * const GrammarManager::MaxTime       = L"@maxtime";
const VXIchar * const GrammarManager::RecordingType = L"@rectype";

enum GrammarScope {
  GRS_NONE,
  GRS_FIELD,
  GRS_DIALOG,
  GRS_DOC
};


class GrammarInfo {
public:
  GrammarInfo(VXIrecGrammar *g, const vxistring &id, const VXMLElement &elem,
              unsigned long gSeq, const VXIchar *srcexpr = NULL, const VXIchar *mime = NULL);
  ~GrammarInfo();

  VXIrecGrammar * GetRecGrammar() const    { return recgrammar; }
  void SetRecGrammar(VXIrecGrammar *rg) { recgrammar = rg; }

  void SetEnabled(bool b, GrammarScope aScope = GRS_NONE)
  {
    enabled = b;
    activatedScope = aScope;
  }

  GrammarScope GetScope(void) const        { return activatedScope; }
  unsigned long GetSequence(void) const    { return grammarSeq; }

  bool IsEnabled() const                   { return enabled; }
  bool IsDynamic() const                   { return !srcexpr.empty(); }
  bool IsScope(GrammarScope s) const       { return s == scope; }

  bool IsField(const vxistring & f) const  { return f == field; }
  bool IsDialog(const vxistring & d) const { return d == dialog; }
  bool IsDoc(const vxistring & d) const    { return d == docID; }
  void GetElement(VXMLElement & e) const   { e = element; }

  const VXIchar *GetSrcExpr() const { return srcexpr.c_str(); }
  const VXIchar *GetMimeType() const { return mimetype.c_str(); }

private:
  void _Initialize(const VXMLElement & elem);
  // This sets the field & dialog names.

  const VXMLElement element;
  VXIrecGrammar *   recgrammar;   // grammar handle returned from rec interface
  GrammarScope      scope;
  vxistring         field;
  vxistring         dialog;
  vxistring         docID;
  vxistring         srcexpr;
  vxistring         mimetype;
  bool              enabled;

  // store current activated info to determine precedence
  GrammarScope      activatedScope;
  unsigned long     grammarSeq;
};


GrammarInfo::GrammarInfo(VXIrecGrammar * g,
                         const vxistring & id,
                         const VXMLElement & elem,
                         unsigned long gSeq,
						 const VXIchar *expr, const VXIchar *mime)
    : element(elem), recgrammar(g), scope(GRS_NONE), docID(id), enabled(false),
      srcexpr(expr?expr:L""), mimetype(mime?mime:L""), activatedScope(GRS_NONE), 
      grammarSeq(gSeq)
{
  // (1) Determine the grammar scope.

  // (1.1) Obtain default from position in the tree.
  VXMLElement par = elem;
  while (par != 0) {
    VXMLElementType name = par.GetName();
    if (name == NODE_INITIAL || name == NODE_FIELD ||
        name == NODE_RECORD  || name == NODE_TRANSFER)
      { scope = GRS_FIELD;  break; }
    else if (name == NODE_FORM || name == NODE_MENU)
      { scope = GRS_DIALOG; break; }
    else if (name == NODE_VXML || name == DEFAULTS_LANGUAGE ||
             name == DEFAULTS_ROOT)
      { scope = GRS_DOC;    break; }
    par = par.GetParent();
  }

  // (1.2) if scope explicitly specified in grammar or parent, override!
  vxistring str;
  elem.GetAttribute(ATTRIBUTE_SCOPE, str);
  if (str.empty() && par != 0) {
    par.GetAttribute(ATTRIBUTE_SCOPE, str);
  }
  if (!str.empty()) {
    if (str == L"dialog")        scope = GRS_DIALOG;
    else if (str == L"document") scope = GRS_DOC;
  }

  // (2) Do remaining initialization.
  _Initialize(elem);
}


GrammarInfo::~GrammarInfo()
{
  // NOTE: 'recgrammar' must be freed externally.
}


void GrammarInfo::_Initialize(const VXMLElement & elem)
{
  if (elem == 0) return;

  VXMLElementType name = elem.GetName();

  if (name == NODE_FIELD || name == NODE_INITIAL ||
      name == NODE_RECORD || name == NODE_TRANSFER)
    elem.GetAttribute(ATTRIBUTE__ITEMNAME, field);
  else if (name == NODE_FORM || name == NODE_MENU) {
    elem.GetAttribute(ATTRIBUTE__ITEMNAME, dialog);
    return;
  }

  _Initialize(elem.GetParent());
}

//#############################################################################

class GrammarInfoUniv {
public:
  GrammarInfoUniv(VXIrecGrammar * g, const VXMLElement & e, const vxistring & l,
                  const vxistring & n, unsigned long seq)
    : element(e), recgrammar(g), languageID(l), name(n), enabled(false),
      grammarSeq(seq) { }

  ~GrammarInfoUniv() { } // NOTE: 'recgrammar' must be freed externally.

public:
  void SetEnabled(bool b)                  { enabled = b; }
  bool IsEnabled() const                   { return enabled; }

  VXIrecGrammar * GetRecGrammar() const    { return recgrammar; }
  unsigned long GetSequence() const        { return grammarSeq; }
  const vxistring & GetLanguage() const    { return languageID; }
  const vxistring & GetName() const        { return name; }

  void GetElement(VXMLElement & e) const   { e = element; }

private:
  const VXMLElement element;
  VXIrecGrammar *   recgrammar;   // grammar handle returned from rec interface
  vxistring         languageID;
  vxistring         name;
  bool              enabled;
  unsigned long     grammarSeq;
};

//#############################################################################
// Grammar description class
//#############################################################################

GrammarManager::GrammarManager(VXIrecInterface * r, const SimpleLogger & l)
  : log(l), vxirec(r), grammarSequence(0)
{
}


GrammarManager::~GrammarManager()
{
  ReleaseGrammars();
}

void GrammarManager::ThrowSpecificEventError(VXIrecResult err, OpType opType)
{
  switch (err) {
    case VXIrec_RESULT_SUCCESS: break;
    case VXIrec_RESULT_NO_RESOURCE:
      throw VXIException::InterpreterEvent(EV_ERROR_NORESOURCE);
    case VXIrec_RESULT_NO_AUTHORIZATION:
      throw VXIException::InterpreterEvent(EV_ERROR_NOAUTHORIZ);
    case VXIrec_RESULT_MAX_SPEECH_TIMEOUT:
      throw VXIException::InterpreterEvent(EV_MAXSPEECH);
    case VXIrec_RESULT_UNSUPPORTED_FORMAT:
      throw VXIException::InterpreterEvent(EV_UNSUPPORT_FORMAT);
    case VXIrec_RESULT_UNSUPPORTED_LANGUAGE:
      throw VXIException::InterpreterEvent(EV_UNSUPPORT_LANGUAGE);
    case VXIrec_RESULT_UNSUPPORTED_BUILTIN:
      throw VXIException::InterpreterEvent(EV_UNSUPPORT_BUILTIN);
    case VXIrec_RESULT_FETCH_TIMEOUT:
    case VXIrec_RESULT_FETCH_ERROR:
      throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH);
    case VXIrec_RESULT_OUT_OF_MEMORY:
      throw VXIException::OutOfMemory();
    // these events are supported for hotword transfer functionality
    case VXIrec_RESULT_CONNECTION_NO_AUTHORIZATION:
      throw VXIException::InterpreterEvent(EV_TELEPHONE_NOAUTHORIZ);
    case VXIrec_RESULT_CONNECTION_BAD_DESTINATION:
      throw VXIException::InterpreterEvent(EV_TELEPHONE_BAD_DEST);
    case VXIrec_RESULT_CONNECTION_NO_ROUTE:
      throw VXIException::InterpreterEvent(EV_TELEPHONE_NOROUTE);
    case VXIrec_RESULT_CONNECTION_NO_RESOURCE:
      throw VXIException::InterpreterEvent(EV_TELEPHONE_NORESOURCE);
    case VXIrec_RESULT_UNSUPPORTED_URI:
      throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_URI);
    default:
      if( opType == GRAMMAR )
        throw VXIException::InterpreterEvent(EV_ERROR_BAD_GRAMMAR);
  }
}

void GrammarManager::LoadGrammars(const VXMLElement& doc,
                                  vxistring & documentID,
                                  PropertyList & properties,
                                  bool isDefaults)
{
  if (doc == 0) return;

  // (1) Retrieve the ID for this document.  This is important for grammar
  // activation.
  if (doc.GetName() == NODE_VXML)
    doc.GetAttribute(ATTRIBUTE__ITEMNAME, documentID);

  // (2) Recursively find and build all grammars on this page.
  if (isDefaults)
    BuildUniversals(doc, properties);
  else {
    VXIMapHolder temp(NULL);
    BuildGrammars(doc, documentID, properties, temp);
  }
}

VXIrecGrammar * GrammarManager::BuildInlineGrammar(const VXMLElement & element,
                                                const VXIMapHolder & localProps)
{
  vxistring text, header, trailer;

  // (1) Get CDATA in this element
  GetEnclosedText(log, element, text);

  // (2) Get attributes
  vxistring mimeType, mode, root, tagFormat, xmlBase, xmlLang;
  element.GetAttribute(ATTRIBUTE_TYPE, mimeType);
  element.GetAttribute(ATTRIBUTE_MODE, mode);
  element.GetAttribute(ATTRIBUTE_ROOT, root);
  element.GetAttribute(ATTRIBUTE_TAGFORMAT, tagFormat);
  element.GetAttribute(ATTRIBUTE_BASE, xmlBase);
  element.GetAttribute(ATTRIBUTE_XMLLANG, xmlLang);

  // (3) Determine the mimetype.

  // if mimetype is empty but root is defined
  // assume that this is srgs+xml mimetype
  if( mimeType.empty() && !root.empty())
    mimeType = L"application/srgs+xml";

  if( mimeType.empty() && mode == L"dtmf" )
    mimeType = REC_MIME_CHOICE_DTMF;

  // (4) Is this an SRGS grammar?
  if (mimeType.find(L"application/srgs+xml") == 0 &&
      (mimeType.length() == 20 || mimeType[20] == L';'))
  {
    // All grammars have a language assigned during parsing.
    header = L"<?xml version='1.0'?>"
             L"<grammar xmlns='http://www.w3.org/2001/06/grammar'"
             L" version='1.0' mode='";
    header += mode;
    header += L"' root='";
    header += root;
    if (!tagFormat.empty()) {
      header += L"' tag-format='";
      header += tagFormat;
    }
    if (!xmlLang.empty()) {
      header += L"' xml:lang='";
      header += xmlLang;
    }
    if (!xmlBase.empty()) {
      header += L"' xml:base='";
      // remove query-string (info. after ?) from the base if exists
      vxistring::size_type qPos = xmlBase.find_first_of(L"?");
      header += (qPos == vxistring::npos ? xmlBase : xmlBase.substr(0, qPos));
    }
    header += L"'>";

    trailer = L"</grammar>";
  }

  // (5) Create the grammar.
  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::LoadGrammars - type="
           << mimeType << L", grammar=" << header << text << trailer << L">";
    log.EndDiagnostic();
  }

  VXIrecGrammar * vg = NULL;

  if (header.empty() && trailer.empty())
    vg = GrammarManager::CreateGrammarFromString(vxirec, log, text,
                                                 mimeType.c_str(),
                                                 localProps);
  else
    vg = GrammarManager::CreateGrammarFromString(vxirec, log,
                                                 header + text + trailer,
                                                 mimeType.c_str(),
                                                 localProps);

  if (vg == NULL)
    throw VXIException::InterpreterEvent(EV_ERROR_BAD_INLINE);

  return vg;
}


// This function is used to recursively walk through the tree, loading and
// speech or dtmf grammars which are found.
//
// NOTE: There is a rather messy optimization with properties, levelProperties,
//       and localProps.  This made the code much harder to read but had
//       significant performance benefits.  Be very careful!
//
void GrammarManager::BuildGrammars(const VXMLElement& doc,
                                  const vxistring & documentID,
                                  PropertyList & properties,
                                  VXIMapHolder & levelProperties,
                                  int menuAcceptLevel)
{
  // (1) Look for grammars in current nodes.

  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & element = reinterpret_cast<const VXMLElement &>(child);
    VXMLElementType elementName = element.GetName();

    // (2) Handle <grammar>

    if (elementName == NODE_GRAMMAR) {
      vxistring src;

      // skip dynamic grammars
      element.GetAttribute(ATTRIBUTE_SRCEXPR, src);
      if (!src.empty()) {
        vxistring mime;
        element.GetAttribute(ATTRIBUTE_TYPE, mime);
        if (mime.empty()) mime = VXI_MIME_SRGS;
        AddGrammar(NULL, documentID, element, src.c_str(), mime.c_str());
        continue;
      }

      element.GetAttribute(ATTRIBUTE_SRC, src);

      // (2.1) Get the recognizer properties associated with this element.
      if (levelProperties.GetValue() == NULL)
        levelProperties.Acquire(GetRecProperties(properties));

      VXIMapHolder localProps(NULL);
      localProps = levelProperties;

      SetGrammarLoadProperties(element, properties, localProps);

      // (2.2) Does the grammar come from an external URI?
      if (!src.empty()) {
        if (log.IsLogging(2)) {
          log.StartDiagnostic(2) << L"GrammarManager::LoadGrammars - <grammar "
            L"src=\"" << src << L"\">";
          log.EndDiagnostic();
        }

        // (2.2.1) Generate error if fragment-only URI in external grammar
        if (!src.empty() && src[0] == '#') {
            log.LogError(215);
            throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                L"a fragment-only URI is not permited in external grammar");
        }

        VXIMapHolder fetchobj;
        if (fetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();
        properties.GetFetchobjCacheAttrs(element, PropertyList::Grammar,
                                         fetchobj);

        // (2.2.2) Load the grammar from the URI.
        vxistring mimeType;
        element.GetAttribute(ATTRIBUTE_TYPE, mimeType);
        if (mimeType.empty()) mimeType = VXI_MIME_SRGS;

        VXIrecGrammar * vg
          = GrammarManager::CreateGrammarFromURI(vxirec, log, src,
                                                 mimeType.c_str(),
                                                 fetchobj.GetValue(),
                                                 localProps);
        AddGrammar(vg, documentID, element);
      }

      // (2.3) Otherwise this is an inlined grammar.
      else {
        log.LogDiagnostic(2, L"GrammarManager::LoadGrammars - <grammar>");

        AddGrammar(BuildInlineGrammar(element, localProps),
                   documentID, element);
      }
    }

    // (3) Handle <link>.  Properties cannot be defined in <link>.

    else if (elementName == NODE_LINK) {
      log.LogDiagnostic(2, L"GrammarManager::LoadGrammars - <link>");

      // (3.1) Create DTMF grammar is specified.
      vxistring dtmf;
      element.GetAttribute(ATTRIBUTE_DTMF, dtmf);
      if (!dtmf.empty()) {
        // Flatten grammar properties if necessary.
        if (levelProperties.GetValue() == NULL)
          levelProperties.Acquire(GetRecProperties(properties));

        // NOTE: We don't need to worry about xml:lang, xml:base, or weight;
        //       This is a generated (no xml:base) dtmf (xml:lang) grammar.

        VXIrecGrammar * vg = NULL;
        vg = GrammarManager::CreateGrammarFromString(vxirec, log, dtmf,
                                                     REC_MIME_CHOICE_DTMF,
                                                     levelProperties);
        if (vg == NULL)
          throw VXIException::InterpreterEvent(EV_ERROR_BAD_CHOICE);

        AddGrammar(vg, documentID, element);
      }

      // (3.2) Create child grammars.
      BuildGrammars(element, documentID, properties, levelProperties);
    }

    // (4) Handle <field>.

    else if (elementName == NODE_FIELD) {
      log.LogDiagnostic(2, L"GrammarManager::LoadGrammars - <field>");

      // (4.1) Get the properties from the field.
      bool doPop = properties.PushProperties(element);
      VXIMapHolder localProps(NULL);
      if (doPop || levelProperties.GetValue() == NULL) 
        localProps.Acquire(GetRecProperties(properties));
      else
        localProps = levelProperties;

      // Set weight, language, and fetchhint.
      SetGrammarLoadProperties(element, properties, localProps);

      // (4.2) Build option grammar (if necessary).
      BuildOptionGrammars(documentID, element, localProps);

      // (4.3) Add the built-in grammars (if they exist).
      VXIrecGrammar * vg = NULL;
      vxistring type;
      element.GetAttribute(ATTRIBUTE_TYPE, type);
      if (!type.empty()) {
        vxistring newuri(L"builtin:grammar/");
        newuri += type;

        vg = GrammarManager::CreateGrammarFromURI(vxirec, log, newuri,
                                                  NULL, NULL, localProps);
        AddGrammar(vg, documentID, element);

        newuri = L"builtin:dtmf/";
        newuri += type;

        vg = GrammarManager::CreateGrammarFromURI(vxirec, log, newuri,
                                                  NULL, NULL, localProps);
        AddGrammar(vg, documentID, element);
      }

      // (4.4) Recursively add grammars (this handles <grammar>)
      BuildGrammars(element, documentID, properties, localProps);
      if (doPop) properties.PopProperties();
    }

    // (5) Handle <menu>.

    else if (elementName == NODE_MENU) {
      log.LogDiagnostic(2, L"GrammarManager::LoadGrammars - <menu>");

      // (5.1) Get the properties from the menu.
      bool doPop = properties.PushProperties(element);
      VXIMapHolder localProps(NULL);
      if (doPop || levelProperties.GetValue() == NULL) 
        localProps.Acquire(GetRecProperties(properties));
      else
        localProps = levelProperties;

      // Set weight, language, and fetchhint.
      SetGrammarLoadProperties(element, properties, localProps);

      // (5.2) Get grammar accept attribute & handle <choice>s.
      vxistring accept;
      if (element.GetAttribute(ATTRIBUTE_ACCEPT, accept) == true &&
          accept == L"approximate")
      {
        BuildGrammars(element, documentID, properties, localProps, 1);
      }
      else
        BuildGrammars(element, documentID, properties, localProps, 0);

      // (5.3) Undo pop if necessary.
      if (doPop) properties.PopProperties();
    }

    // (6) Handle <choice>

    else if (elementName == NODE_CHOICE) {
      log.LogDiagnostic(2, L"GrammarManager::LoadGrammars - <choice>");

      // (6.1) If there is a <grammar> tag, it overrides any implicit grammar.

      // (6.1.1) Check for <grammar> element.
      bool foundGrammar = false;

      for (VXMLNodeIterator it(element); it; ++it) {
        VXMLNode child = *it;
        if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
        const VXMLElement & temp = reinterpret_cast<const VXMLElement&>(child);
        if (temp.GetName() != NODE_GRAMMAR) continue;
        foundGrammar = true;
        break;
      }

      // (6.1.2) If found, apply recursion.
      if (foundGrammar) {
        // <choice> nodes can't contain properties.  Don't need to call Push.
        BuildGrammars(element, documentID, properties, levelProperties);
      }

      // (6.2) DTMF & CDATA grammars.

      vxistring dtmf;
      element.GetAttribute(ATTRIBUTE_DTMF, dtmf);

      vxistring text;
      if (!foundGrammar)
        GetEnclosedText(log, element, text);

      if (!dtmf.empty() || !text.empty()) {
        // (6.2.1) Set up properties.

        // This should not ever be necessary, but just in case...
        if (levelProperties.GetValue() == NULL) {
          levelProperties.Acquire(GetRecProperties(properties));
          log.LogError(999, SimpleLogger::MESSAGE,
                       L"GrammarManager::BuildGrammars levelProperties "
                       L"not already built for a <choice>");
        }

        // (6.2.2) Build DTMF grammars
        if (!dtmf.empty()) {
          VXIrecGrammar * vg = GrammarManager::CreateGrammarFromString(
                                                     vxirec, log, dtmf,
                                                     REC_MIME_CHOICE_DTMF,
                                                     levelProperties);
          if (vg == NULL)
            throw VXIException::InterpreterEvent(EV_ERROR_BAD_CHOICE);

          AddGrammar(vg, documentID, element);
        }

        // (6.2.3) Build CDATA grammars (if not overriden by explicit <grammar>

        if (!text.empty()) {
          // Set accept property.  This changes the pseudo-read-only
          // levelProperties, so we'll undo the change after creating the
          // grammar.
          vxistring accept;
          if (element.GetAttribute(ATTRIBUTE_ACCEPT, accept)) {
            if (accept == L"approximate")
              AddParamValue(levelProperties, REC_GRAMMAR_ACCEPTANCE, 1);
            else
              AddParamValue(levelProperties, REC_GRAMMAR_ACCEPTANCE, 0);
          }
          else
            AddParamValue(levelProperties, REC_GRAMMAR_ACCEPTANCE,
                          menuAcceptLevel ? 1 : 0);

          VXIrecGrammar * vg = GrammarManager::CreateGrammarFromString(
                                                       vxirec, log, text,
                                                       REC_MIME_CHOICE,
                                                       levelProperties);
          if (vg == NULL)
            throw VXIException::InterpreterEvent(EV_ERROR_BAD_CHOICE);

          AddGrammar(vg, documentID, element);

          // As promised, undo the REC_GRAMMAR_ACCEPTANCE property.
          VXIMapDeleteProperty(levelProperties.GetValue(),
                               REC_GRAMMAR_ACCEPTANCE);
        }
      }
    }

    // (7) Otherwise, nothing was found at this level.  Use recursion to check
    //     the next level down.

    else {
      bool doPop = properties.PushProperties(element);

      if (doPop) {
        VXIMapHolder temp(NULL);
        BuildGrammars(element, documentID, properties, temp, menuAcceptLevel);
      }
      else
        BuildGrammars(element, documentID, properties, levelProperties,
                      menuAcceptLevel);

      if (doPop) properties.PopProperties();
    }
  }
}


// The defaults have a very simple structure.  The element in this case is the
// desired language.  This has an 'id' attribute and contains the <link> and 
// <property> elements.
//
void GrammarManager::BuildUniversals(const VXMLElement& doc,
                                     PropertyList & properties)
{
  // (1) Collect all the properties defined at this level.  This is the only
  //     point at which properties may be declared.

  properties.PushProperties(doc);
  VXIMapHolder localProps(GetRecProperties(properties));

  // (2) Get the name of the language.

  vxistring languageID;
  if (!doc.GetAttribute(ATTRIBUTE_ID, languageID)) {
    log.LogError(999, SimpleLogger::MESSAGE, L"defaults document corrupted - "
                 L"no id attribute on <language> element");
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH);
  }

  // (3) Find each link.

  for (VXMLNodeIterator temp3(doc); temp3; ++temp3) {
    VXMLNode child = *temp3;
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & link = reinterpret_cast<const VXMLElement&>(child);
    if (link.GetName() != NODE_LINK) continue;

    // (3.1) Get the name of the link.
    
    vxistring linkName;
    if (!link.GetAttribute(ATTRIBUTE_EVENT, linkName)) {
      log.LogError(999, SimpleLogger::MESSAGE, L"defaults document corrupted "
                   L"- no event attribute on <link> element");
      throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH);
    }

    // (3.2) Process dtmf grammars for this link.

    vxistring dtmf;
    link.GetAttribute(ATTRIBUTE_DTMF, dtmf);
    if (!dtmf.empty()) {
      // NOTE: We don't need to worry about xml:lang, xml:base, or weight;
      //       This is a generated (no xml:base) dtmf (xml:lang) grammar.
      VXIrecGrammar * vg = NULL;
      vg = GrammarManager::CreateGrammarFromString(vxirec, log, dtmf,
                                                   REC_MIME_CHOICE_DTMF,
                                                   localProps);
      if (vg == NULL)
        throw VXIException::InterpreterEvent(EV_ERROR_BAD_CHOICE);

      AddUniversal(vg, link, languageID, linkName);
    }

    // (4) And each grammar.
    for (VXMLNodeIterator temp4(link); temp4; ++temp4) {
      VXMLNode child = *temp4;
      if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & gram = reinterpret_cast<const VXMLElement &>(child);
      if (gram.GetName() != NODE_GRAMMAR) continue;

      // (4.1) Get caching properties on the <grammar> element.
      SetGrammarLoadProperties(gram, properties, localProps);

      // (4.2) Process remote grammars
      vxistring src;
      gram.GetAttribute(ATTRIBUTE_SRC, src);

      if (!src.empty()) {
        // (4.2.1) Generate error if fragment-only URI in external grammar
        if (!src.empty() && src[0] == '#') {
          log.LogError(215);
          throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                L"a fragment-only URI is not permited in external grammar");
        }

        VXIMapHolder fetchobj;
        if (fetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();
        properties.GetFetchobjCacheAttrs(gram, PropertyList::Grammar, fetchobj);

        // (4.2.2) Load the grammar from the URI.
        vxistring mimeType;
        gram.GetAttribute(ATTRIBUTE_TYPE, mimeType);

        VXIrecGrammar * vg
          = GrammarManager::CreateGrammarFromURI(vxirec, log, src,
                                                 mimeType.c_str(),
                                                 fetchobj.GetValue(),
                                                 localProps);

        AddUniversal(vg, gram, languageID, linkName);
      }
      else {
        AddUniversal(BuildInlineGrammar(gram, localProps),
                     gram, languageID, linkName);
      }
    } // end <grammar>
  } // end <link>

  properties.PopProperties();
}


class VXIVectorHolder {
public:
  VXIVectorHolder() : _v(NULL) { _v = VXIVectorCreate(); }
  ~VXIVectorHolder()           { if (_v != NULL) VXIVectorDestroy(&_v); }
  VXIVector * GetValue() const { return _v; }

private:
  VXIVectorHolder(const VXIVectorHolder &); /* intentionally not defined. */
  VXIVector * _v;
};


void GrammarManager::BuildOptionGrammars(const vxistring & documentID,
                                         const VXMLElement & element,
                                         const VXIMapHolder & props)

{
  log.LogDiagnostic(2, L"GrammarManager::BuildOptionGrammars()");

  // (1) Create new vectors to hold the grammar.

  VXIVectorHolder speechUtts;
  VXIVectorHolder speechVals;
  VXIVectorHolder dtmfUtts;
  VXIVectorHolder dtmfVals;
  VXIVectorHolder gramAcceptanceAttrs;

  if (speechUtts.GetValue() == NULL || speechVals.GetValue() == NULL ||
      dtmfUtts.GetValue()   == NULL || dtmfVals.GetValue()   == NULL ||
      gramAcceptanceAttrs.GetValue() == NULL)
    throw VXIException::OutOfMemory();

  // (2) Get each option.

  for (VXMLNodeIterator it(element); it; ++it) {
    VXMLNode child = *it;

    // (2.1) Ignore anything which isn't an option.

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    VXMLElement & domElem = reinterpret_cast<VXMLElement &>(child);
    if (domElem.GetName() != NODE_OPTION) continue;

    // (2.2) Get attributes and CDATA.

    vxistring value;
    domElem.GetAttribute(ATTRIBUTE_VALUE, value);

    vxistring dtmf;
    domElem.GetAttribute(ATTRIBUTE_DTMF, dtmf);

    vxistring accept;
    domElem.GetAttribute(ATTRIBUTE_ACCEPT, accept);

    vxistring text;
    GrammarManager::GetEnclosedText(log, domElem, text);

    if (value.empty()) value = text;
    if (value.empty()) value = dtmf;

    // (2.3) Add to vectors as appropriate.

    if (!text.empty()) {
      VXIVectorAddElement(speechUtts.GetValue(),
         reinterpret_cast<VXIValue *>(VXIStringCreate(text.c_str())));
      VXIVectorAddElement(speechVals.GetValue(),
         reinterpret_cast<VXIValue *>(VXIStringCreate(value.c_str())));
    }

    if (!dtmf.empty()) {
      VXIVectorAddElement(dtmfUtts.GetValue(),
         reinterpret_cast<VXIValue *>(VXIStringCreate(dtmf.c_str())));
      VXIVectorAddElement(dtmfVals.GetValue(),
         reinterpret_cast<VXIValue *>(VXIStringCreate(value.c_str())));
    }

    if( !text.empty() || !dtmf.empty() )
    {
      if (!accept.empty() && accept == L"approximate") {
        VXIVectorAddElement(gramAcceptanceAttrs.GetValue(),
          reinterpret_cast<VXIValue *>(VXIIntegerCreate(1)));
        } else {
          VXIVectorAddElement(gramAcceptanceAttrs.GetValue(),
            reinterpret_cast<VXIValue *>(VXIIntegerCreate(0)));
        }
    }

  }

  // (3) Add grammars.

  VXIrecGrammar * grammar;

  if (VXIVectorLength(speechUtts.GetValue()) > 0) {
    VXIrecResult err = vxirec->LoadGrammarOption(vxirec, props.GetValue(),
                                                 speechUtts.GetValue(),
                                                 speechVals.GetValue(),
                                                 gramAcceptanceAttrs.GetValue(),
                                                 FALSE, &grammar);
    if( err != VXIrec_RESULT_SUCCESS )
      ThrowSpecificEventError(err, GRAMMAR);

    AddGrammar(grammar, documentID, element);
  }

  if (VXIVectorLength(dtmfUtts.GetValue()) > 0) {
    VXIrecResult err = vxirec->LoadGrammarOption(vxirec, props.GetValue(),
                                                 dtmfUtts.GetValue(),
                                                 dtmfVals.GetValue(),
                                                 gramAcceptanceAttrs.GetValue(),
                                                 TRUE, &grammar);
    if( err != VXIrec_RESULT_SUCCESS )
      ThrowSpecificEventError(err, GRAMMAR);

    AddGrammar(grammar, documentID, element);
  }

  log.LogDiagnostic(2, L"GrammarManager::BuildOptionGrammar - end");
}


bool GrammarManager::GetEnclosedText(const SimpleLogger & log,
                                     const VXMLElement & doc, vxistring & str)
{
  log.LogDiagnostic(2, L"GrammarManager::GetEnclosedText()");

  // Clear the result first...
  str.erase();

  for (VXMLNodeIterator it(doc); it; ++it) {
    if ((*it).GetType() == VXMLNode::Type_VXMLContent) {
      if (!str.empty()) str += ' ';
      VXMLNode temp = *it;
      str = reinterpret_cast<VXMLContent &>(temp).GetValue();
    }
  }

  log.LogDiagnostic(2, L"GrammarManager::GetEnclosedText - end");
  return !str.empty();
}


VXIrecGrammar*
GrammarManager::CreateGrammarFromURI(VXIrecInterface * vxirec,
                                     const SimpleLogger & log,
                                     const vxistring & source,
                                     const VXIchar * type,
                                     const VXIMap * fetchProps,
                                     const VXIMapHolder & recProps)
{
  if (log.IsLogging(3)) {
    log.StartDiagnostic(3) << L"GrammarManager::LoadGrammar (type="
                           << ((type == NULL) ? L"NULL" : type)
                           << L"): " << source;
    log.EndDiagnostic();
  }

  VXIrecGrammar * vg;
  VXIrecResult err = vxirec->LoadGrammarURI(vxirec, recProps.GetValue(),
                                            type,
                                            source.c_str(),
                                            fetchProps, &vg);
    if( err != VXIrec_RESULT_SUCCESS )
      ThrowSpecificEventError(err, GRAMMAR);

  log.LogDiagnostic(2, L"GrammarManager::CreateGrammarFromURI - success");

  return vg;
}


VXIrecGrammar*
GrammarManager::CreateGrammarFromString(VXIrecInterface * vxirec,
                                        const SimpleLogger & log,
                                        const vxistring & source,
                                        const VXIchar * type,
                                        const VXIMapHolder & recProps)
{
  if (log.IsLogging(3)) {
    log.StartDiagnostic(3) << L"GrammarManager::LoadGrammar (type=" << type
                           << L"): " << source;
    log.EndDiagnostic();
  }

  VXIrecGrammar * grammar;
  VXIrecResult err = vxirec->LoadGrammarString(vxirec, recProps.GetValue(),
                                               type, source.c_str(), &grammar);

  if( err != VXIrec_RESULT_SUCCESS )
      ThrowSpecificEventError(err, GRAMMAR);

  log.LogDiagnostic(2, L"GrammarManager::CreateGrammarFromString - success");

  return grammar;
}


void GrammarManager::AddGrammar(VXIrecGrammar * gr,
                                const vxistring & documentID,
                                const VXMLElement & elem,
								const VXIchar *expr, const VXIchar *type)
{
  GrammarInfo* gp = new GrammarInfo(gr, documentID, elem, GetGrammarSequence(), expr, type);
  if (gp == NULL) throw VXIException::OutOfMemory();

  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::AddGrammar(" << gr << L")"
                           << L" seq: " << gp->GetSequence();
    log.EndDiagnostic();
  }

  grammars.push_back(gp);
}


void GrammarManager::AddUniversal(VXIrecGrammar * gr,
                                  const VXMLElement & elem,
                                  const vxistring & langID,
                                  const vxistring & name)
{
  GrammarInfoUniv * gp = new GrammarInfoUniv(gr, elem, langID, name,
                                             GetGrammarSequence());
  if (gp == NULL) throw VXIException::OutOfMemory();

  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::AddGrammar(" << gr << L")"
                           << L" seq: " << gp->GetSequence();
    log.EndDiagnostic();
  }

  universals.push_back(gp);
}


void GrammarManager::DisableAllGrammars()
{
  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::DisableAllGrammars: Enter";
    log.EndDiagnostic();
  }

  for (GRAMMARS::iterator i = grammars.begin(); i != grammars.end(); ++i) {
    if ((*i)->IsEnabled()) {
      vxirec->DeactivateGrammar(vxirec, (*i)->GetRecGrammar());
      (*i)->SetEnabled(false);
      if ((*i)->IsDynamic()){
        if (log.IsLogging(2)) {
          log.StartDiagnostic(2) << L"GrammarManager::DisableAllGrammars: Free dynamic grammar";
          log.EndDiagnostic();
        }

        VXIrecGrammar * grammar = (*i)->GetRecGrammar();
        vxirec->FreeGrammar(vxirec, &grammar);
        (*i)->SetRecGrammar(NULL);
      }
    }
  }

  for (UNIVERSALS::iterator j = universals.begin(); j != universals.end(); ++j){
    if ((*j)->IsEnabled()) {
      vxirec->DeactivateGrammar(vxirec, (*j)->GetRecGrammar());
      (*j)->SetEnabled(false);
    }
  }

  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::DisableAllGrammars: Leave";
    log.EndDiagnostic();
  }
}

void GrammarManager::ReleaseGrammars()
{
  while (!grammars.empty()) {
    GrammarInfo * gp = grammars.back();
    grammars.pop_back();
    VXIrecGrammar * grammar = gp->GetRecGrammar();
    if (grammar) {
      if (log.IsLogging(2)) {
        log.StartDiagnostic(2) << L"GrammarManager::ReleaseGrammars(" << grammar
                               << L")";
        log.EndDiagnostic();
      }

      if (gp->IsEnabled())
        vxirec->DeactivateGrammar(vxirec, grammar);
      vxirec->FreeGrammar(vxirec, &grammar);
      delete gp;
    }
  }

  while (!universals.empty()) {
    GrammarInfoUniv * gp = universals.back();
    universals.pop_back();
    VXIrecGrammar * grammar = gp->GetRecGrammar();

    if (log.IsLogging(2)) {
      log.StartDiagnostic(2) << L"GrammarManager::ReleaseGrammars(" << grammar
                             << L")";
      log.EndDiagnostic();
    }

    vxirec->FreeGrammar(vxirec, &grammar);
    delete gp;
  }
}


bool GrammarManager::EnableGrammars(const vxistring & documentID,
                                    const vxistring & dialogName,
                                    const vxistring & fieldName,
                                    const VXIMapHolder & properties,
                                    bool isModal,
                                    Scripter &script,
                                    PropertyList & propList)
{
  bool enabled = false;
  if (log.IsLogging(2)) {
    log.StartDiagnostic(2) << L"GrammarManager::EnableGrammar: Enter";
    log.EndDiagnostic();
  }

  // (1) Do ordinary grammars...
  // They are activated in reverse order.  This should activate them
  // in the order of precedence the VXMl spec dictates (3.1.4)
  for (GRAMMARS::iterator i = grammars.end(); i != grammars.begin();)
  {
    i--;

    bool docsMatch    = (*i)->IsDoc(documentID);
    bool dialogsMatch = docsMatch    && (*i)->IsDialog(dialogName);
    bool fieldsMatch  = dialogsMatch && (*i)->IsField(fieldName);
    bool fieldGram = (!fieldName.empty() && fieldsMatch);
    bool dialogGram = (!isModal && (*i)->IsScope(GRS_DIALOG) && dialogsMatch);
    bool docGram = (!isModal && (*i)->IsScope(GRS_DOC));
    GrammarScope cScope = GRS_NONE;

    if ( fieldGram ||
        // Enable those field grammars matching our (field, form) pair
         dialogGram ||
        // Enable form grammars & dialog scope fields, if not modal
         docGram
        // Enable document level grammars, if not modal
       )
    {
      // Dynamic grammars have to be loaded prior to activation.
      bool dynamic = (*i)->IsDynamic();
      if (dynamic) {
        vxistring src;
        try {
          script.EvalScriptToString((*i)->GetSrcExpr(), src);
        }
        catch(...) {
          // per spec, throw error.semantic on eval error
          throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
        }

        if (log.IsLogging(2)) {
          log.StartDiagnostic(2) << L"GrammarManager::EnableGrammar, Load Dynamic: "
                                 << (*i)->GetSrcExpr();
          log.EndDiagnostic();
        }

        VXIMapHolder localProps(GetRecProperties(propList));
        VXMLElement elem;
        (*i)->GetElement(elem);
        SetGrammarLoadProperties(elem, propList, localProps);

        VXIMapHolder fetchobj;
        if (fetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();
        propList.GetFetchobjCacheAttrs(elem,PropertyList::Grammar,fetchobj);

        VXIrecGrammar * vg
          = GrammarManager::CreateGrammarFromURI(vxirec, log, src,
                                                 (*i)->GetMimeType(),
                                                 fetchobj.GetValue(),
                                                 localProps);
        (*i)->SetRecGrammar(vg);
      }

      if (log.IsLogging(2)) {
        log.StartDiagnostic(2) << L"GrammarManager::EnableGrammar("
                               << (*i)->GetRecGrammar() << L")";
        log.EndDiagnostic();
      }

      // activate the grammar
      VXIrecResult err = vxirec->ActivateGrammar(vxirec, properties.GetValue(),
                                                 (*i)->GetRecGrammar());

      if( err != VXIrec_RESULT_SUCCESS )
       ThrowSpecificEventError(err, GRAMMAR);

      // determine grammar scope
      if (fieldGram)
        cScope = GRS_FIELD;
      else if (dialogGram)
        cScope = GRS_DIALOG;
      else if (docGram)
        cScope = GRS_DOC;

     (*i)->SetEnabled(true, cScope);
      enabled = true;
    }
  }

  // (2) Do universals
#pragma message("Universals should be sensitive to the active language.")

  // (2.1) Get the property.
  const VXIValue * val = VXIMapGetProperty(properties.GetValue(),
                                           PROP_UNIVERSALS);
  if (val == NULL || VXIValueGetType(val) != VALUE_STRING)
    return enabled;
  const VXIchar * temp = VXIStringCStr(reinterpret_cast<const VXIString*>(val));

  // (2.2) Convert the property into a set of tokens separated by the delimiter
  //       characters.
  const VXIchar * DELIM = L"|";

  vxistring request = DELIM;
  for (; *temp != '\0'; ++temp) {
    VXIchar t = *temp;
    if (t == ' ' || t == '\t' || t == '\r' || t == '\n')
      request += DELIM;
    else 
      request += t;
  }
  request += DELIM;

  // (2.3) If the universals string is 'none', we are done.
  if (request == L"|none|")
    return enabled;

  // (2.4) Check for all anywhere in the string.
  bool doAll = (request.find(L"|all|") != vxistring::npos);

  // (2.5) Now walk the list.
  for (UNIVERSALS::iterator j = universals.begin(); j != universals.end(); ++j)
  {
    bool doThis = doAll;
    if (!doThis) {
      vxistring delimName = DELIM;
      delimName += (*j)->GetName();
      delimName += DELIM;
      doThis = (request.find(delimName) != vxistring::npos);
    }

    if (doThis) {
      VXIrecResult err = vxirec->ActivateGrammar(vxirec, properties.GetValue(),
                                                 (*j)->GetRecGrammar());

      if (err != VXIrec_RESULT_SUCCESS)
        ThrowSpecificEventError(err, GRAMMAR);

      (*j)->SetEnabled(true);
      enabled = true;
    }
  }

  return enabled;
}


// This function is responsible for calling the VXIrec level Recognize function
// and then mapping the grammar back to the corresponding VXML node.
//
GrammarManager::RecResult GrammarManager::Recognize(const VXIMapHolder        & properties,
                              VXIrecRecognitionResult * & result)
{
  // (1) Attempt a recognition.
  VXIrecResult err = vxirec->Recognize(vxirec, properties.GetValue(), &result);

  if (err != VXIrec_RESULT_SUCCESS && result != NULL) {
    result->Destroy(&result);
	result = NULL;
  }

  // (2) Process return value.
  switch (err) {
  case VXIrec_RESULT_SUCCESS:
    break;
  default:
    // try to throw any specific event for the error before bail out
    ThrowSpecificEventError(err, RECOGNIZE);
    // throw an internal error
    log.StartDiagnostic(0) << L"GrammarManager::InternalRecognize - "
      L"VXIrecInterface::Recognize returned " << int (err);
    log.EndDiagnostic();
    log.LogError(420, SimpleLogger::MESSAGE,
                 L"function did not return the expected VXIrecSUCCESS result");
    return GrammarManager::InternalError;
  }

  // (3) Is the result structure valid?
  if (result == NULL) {
    log.LogError(420, SimpleLogger::MESSAGE,
                 L"function returned VXIrecSUCCESS but did not allocate "
                 L"an answer structure");
    return GrammarManager::InternalError;
  }

  if (result->xmlresult == NULL) {
    log.LogError(420, SimpleLogger::MESSAGE,
                 L"function returned VXIrecSUCCESS but did not allocate "
                 L"an XML result");
    result->Destroy(&result);    
	result = NULL;
    return GrammarManager::InternalError;
  }

  return GrammarManager::Success;
}


bool GrammarManager::FindMatchedElement(const vxistring & id,
                                        VXMLElement & match) const
{
  unsigned long trash;
  return FindMatchedElement(id, match, trash);
}


// Find the VXML element associated with the matched grammar.
//
bool GrammarManager::FindMatchedElement(const vxistring & id,
                                        VXMLElement & match,
                                        unsigned long & grammarInfo) const
{
  const VXIrecGrammar * bestGrammar = NULL;

  if (id.empty()) return false;

  VXIrecResult err = vxirec->GetMatchedGrammar(vxirec, id.c_str(),
                                               &bestGrammar);
  if (err != VXIrec_RESULT_SUCCESS) {
    log.LogError(420, SimpleLogger::MESSAGE,
                 L"function failed to return a grammar match");
    return false;
  }

  for (GRAMMARS::const_iterator i = grammars.begin(); i!=grammars.end(); ++i) {
    if ((*i)->GetRecGrammar() != bestGrammar) continue;

    if (!(*i)->IsEnabled()) {
      log.StartDiagnostic(0) << L"GrammarManager::FindMatchedElement - "
          L"Grammar <" << (*i)->GetRecGrammar() << L"> is not activated!";
      log.EndDiagnostic();
      log.LogError(420, SimpleLogger::MESSAGE,
                   L"function returned an inactive grammar");
      return false;
    }

    (*i)->GetElement(match);
    grammarInfo = (*i)->GetSequence();
    return true;
  }

  for (UNIVERSALS::const_iterator j = universals.begin();
       j != universals.end(); ++j)
  {
    if ((*j)->GetRecGrammar() != bestGrammar) continue;

    if (!(*j)->IsEnabled()) {
      log.LogError(420, SimpleLogger::MESSAGE,
                   L"function returned an inactive grammar");
      return false;
    }

    (*j)->GetElement(match);
    grammarInfo = (*j)->GetSequence();
    return true;
  }

  log.LogError(420, SimpleLogger::MESSAGE,
               L"function returned a non-existent grammar");
  return false;
}

// compare the precedence of info1 against info2, return 1 if info1 has
// higher precedence than info2, -1 otherwise, 0 if both has the same precedence
int GrammarManager::CompareGrammar(const unsigned long grammar1,
                                   const unsigned long grammar2)
{
  // Is either grammar an universal?
  bool firstIsUniversal = false;
  bool secondIsUniversal = false;
	
  for (UNIVERSALS::const_iterator j = universals.begin();
       j != universals.end(); ++j)
  {
    unsigned long temp = (*j)->GetSequence();
    if (temp == grammar1) firstIsUniversal = true;
    if (temp == grammar2) secondIsUniversal = true;
  }

  if (firstIsUniversal && secondIsUniversal) {
    if (grammar1 < grammar2) return 1;
    if (grammar1 > grammar2) return -1;
    return 0;
  }

  if (firstIsUniversal) return 1;
  if (secondIsUniversal) return -1;

  // The remaining grammars are not universals.

  const GrammarInfo * info1 = NULL;
  const GrammarInfo * info2 = NULL;

  for (GRAMMARS::const_iterator i = grammars.begin(); i!=grammars.end(); ++i) {
    unsigned long temp = (*i)->GetSequence();
    if (temp == grammar1) info1 = *i;
    if (temp == grammar2) info2 = *i;
  }

  if (info1 && info2) {
    GrammarScope s1 = info1->GetScope();
    GrammarScope s2 = info2->GetScope();
    if( info1 == info2 ) return 0;
    if( s1 == GRS_NONE || s2 == GRS_NONE ) return 0; // unable to determine
    if( s1 < s2 ) return 1;
    if( s1 > s2 ) return -1;
    if( s1 == s2 ) {
      // if both have the same precedence, use document order
      // to determine
      if( info1->GetSequence() < info2->GetSequence() )
        return 1;
      else if(info1->GetSequence() > info2->GetSequence())
        return -1;
      else return 0;
    }
  }

  return 0; // unable to determine
}

GrammarManager::RecResult GrammarManager::Record(const VXIMapHolder & properties,
                           bool flushBeforeRecord,
                           VXIrecRecordResult * & resultStruct)
{
  const VXIMap *finalProp = NULL;
  VXIMapHolder addlProp(NULL);
  if (flushBeforeRecord) {
    addlProp = properties;
    AddParamValue(addlProp, REC_DTMF_FLUSH_QUEUE, true);
    finalProp = addlProp.GetValue();
  } else {
    finalProp = properties.GetValue();
  }

  VXIrecResult err = vxirec->Record(vxirec, finalProp, &resultStruct);

  if (err != VXIrec_RESULT_SUCCESS) {
    // try to throw a specific event for the error before bailing out
    ThrowSpecificEventError(err, RECORD);
    
    // throw an internal error if specific event cannot be thrown
    log.StartDiagnostic(0) << L"GrammarManager::Record - "
      L"VXIrecInterface::Record returned " << int (err);
    log.EndDiagnostic();
    log.LogError(421, SimpleLogger::MESSAGE,
                 L"function did not return the expected VXIrecSUCCESS result");
    if (resultStruct) {
      resultStruct->Destroy(&resultStruct);
      resultStruct = NULL;
    }
    return GrammarManager::InternalError;
  }

  if (resultStruct == NULL) {
    log.LogError(421, SimpleLogger::MESSAGE,
                 L"function returned VXIrecSUCCESS but did not allocate "
                 L"an answer structure");
    return GrammarManager::InternalError;
  }

  if (resultStruct->waveform == NULL && resultStruct->xmlresult == NULL) {
    log.LogError(421, SimpleLogger::MESSAGE,
                 L"function did not produce a recording or XML result");
    return GrammarManager::InternalError;
  }

  if (resultStruct->waveform != NULL && resultStruct->duration == 0) {
    log.LogError(421, SimpleLogger::MESSAGE,
                 L"function returned invalid recording duration");
    return GrammarManager::InternalError;
  }

  return GrammarManager::Success;
}

void GrammarManager::SetInputMode(const VXIchar *mode, VXIMapHolder &props) const
{
  int modeVal = REC_INPUT_MODE_DTMF_SPEECH;
  if( mode ) {
    vxistring value(mode);
    if (value == L"voice dtmf" || value == L"dtmf voice")
      modeVal = REC_INPUT_MODE_DTMF_SPEECH;
    else if (value == L"voice")
      modeVal = REC_INPUT_MODE_SPEECH;
    else if (value == L"dtmf")
      modeVal = REC_INPUT_MODE_DTMF;
    else {
      log.StartDiagnostic(0) << L"GrammarManager::SetInputMode - "
        L"Property '" << REC_INPUT_MODES << L"' contains unrecognized value \"" << value <<
        L"\", deliberately set to \"voice dtmf\"";
      log.EndDiagnostic();
    }
  }
  AddParamValue(props, REC_INPUT_MODES, modeVal);     
}                              

VXIMap * GrammarManager::GetRecProperties(const PropertyList & props,
                                          int timeout) const
{
  VXIMapHolder m;
  if (m.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (1) Retrieve flattened property list.
  props.GetProperties(m);

  // (2) Convert & manipulate the properties.
  const VXIchar * j;
  VXIint time;
  VXIflt32 fraction;

  // (2.1) Language
  j = props.GetProperty(PropertyList::Language);
  if (j != NULL)
    AddParamValue(m, REC_LANGUAGE, j);

  // (2.2) Completion timeout
  j = props.GetProperty(PROP_COMPLETETIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_TIMEOUT_COMPLETE, time);
  }
  
  // (2.3) Incompletion timeout
  j = props.GetProperty(PROP_INCOMPLETETIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_TIMEOUT_INCOMPLETE, time);
  }
  
  // (2.4) Inter-Digit timeout
  j = props.GetProperty(PROP_INTERDIGITTIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_DTMF_TIMEOUT_INTERDIGIT, time);
  }
  
  // (2.5) Inter-Digit timeout
  j = props.GetProperty(PROP_TERMTIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_DTMF_TIMEOUT_TERMINATOR, time);
  }
  
  // (2.6) Confidence level
  j = props.GetProperty(PROP_CONFIDENCE);
  if (j != NULL && props.ConvertValueToFraction(log, j, fraction))
    AddParamValue(m, REC_CONFIDENCE_LEVEL, fraction);

  // (2.7) Barge-in sensitivity level
  j = props.GetProperty(PROP_SENSITIVITY);
  if (j != NULL && props.ConvertValueToFraction(log, j, fraction))
    AddParamValue(m, REC_SENSITIVITY, fraction);

  // (2.8) Performance tradeoff - speed vs. accuracy
  j = props.GetProperty(PROP_SPEEDVSACC);
  if (j != NULL && props.ConvertValueToFraction(log, j, fraction))
    AddParamValue(m, REC_SPEED_VS_ACCURACY, fraction);

  // (2.9) DTMF terminator character
  j = props.GetProperty(PROP_TERMCHAR);
  if (j != NULL) {
    if (wcslen(j) < 2)
      AddParamValue(m, REC_DTMF_TERMINATOR_CHAR, j);
    else {
      log.StartDiagnostic(0) << L"GrammarManager::GetRecProperties - "
        L"Unable to set " << REC_DTMF_TERMINATOR_CHAR << L" from value \"" <<
        j << L"\".  Defaulting to '#'.";
      log.EndDiagnostic();
      // Should we use the default, or rather not set anything ?
      AddParamValue(m, REC_DTMF_TERMINATOR_CHAR, L"#");
    }
  }

  // (2.10) Input modes
  j = props.GetProperty(PROP_INPUTMODES);
  SetInputMode(j, m);

  // (2.11) Timeout settings
  if (timeout == -1) {
    j = props.GetProperty(PROP_TIMEOUT);
    if (j != NULL)
      if( !PropertyList::ConvertTimeToMilliseconds(log, j, timeout) )
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
  }

  if (timeout != -1) {
    AddParamValue(m, REC_DTMF_TIMEOUT, timeout);
    AddParamValue(m, REC_TIMEOUT, timeout);
  }

  // (2.12) Bargein type
  int bargintype = REC_BARGEIN_SPEECH;
  j = props.GetProperty(PROP_BARGEINTYPE);
  if (j != NULL) {
    vxistring value(j);
    if (value == L"hotword")
      bargintype = REC_BARGEIN_HOTWORD;
    else if (value == L"speech")
      bargintype = REC_BARGEIN_SPEECH;
    else {
      log.StartDiagnostic(0) << L"GrammarManager::GetRecProperties - "
        L"Property '" << REC_BARGEIN_TYPE << L"' contains unrecognized value \"" << value <<
        L"\", deliberately set to \"speech\"";
      log.EndDiagnostic();
    }
  }
  AddParamValue(m, REC_BARGEIN_TYPE, bargintype);

  // (2.13) maxnbest property
  j = props.GetProperty(PROP_MAXNBEST);
  if (j != NULL) {
    VXIint nbest = 1;
    std::basic_stringstream<VXIchar> nbestStream(j);
    nbestStream >> nbest;
    AddParamValue(m, REC_RESULT_NBEST_SIZE, nbest);
  }

  // (2.14) maxspeechtimeout property
  j = props.GetProperty(PROP_MAXSPEECHTIME);
  if (j != NULL) {
    if( !PropertyList::ConvertTimeToMilliseconds(log, j, timeout) )
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_TIMEOUT_SPEECH, timeout);
  }

  // (2.15) recordutterance properties
  j = props.GetProperty(PROP_RECORDUTTERANCE);
  if (j != NULL)
    AddParamValue(m, REC_RECORDUTTERANCE, j);

  j = props.GetProperty(PROP_RECORDUTTERANCETYPE);
  if (j != NULL)
    AddParamValue(m, REC_RECORDUTTERANCETYPE, j);

  return m.Release();
}


VXIMap * GrammarManager::GetRecordProperties(const PropertyList & props,
                                             int timeout) const
{
  VXIMapHolder m;
  if (m.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (1) Retrieve flattened property list.
  props.GetProperties(m);

  // (2) Convert & manipulate the properties.
  const VXIchar * j;
  VXIint time;

  // (2.1) Completion timeout
  j = props.GetProperty(GrammarManager::MaxTime);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_MAX_RECORDING_TIME, time);
  }
  
  // (2.2) Final silence
  j = props.GetProperty(GrammarManager::FinalSilence);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_TIMEOUT_COMPLETE, time);
  }
  
  // (2.3) Type
  j = props.GetProperty(GrammarManager::RecordingType);
  if (j != NULL)
    AddParamValue(m, REC_RECORD_MIME_TYPE, j);

  // (2.4) DTMF terminates record?
  j = props.GetProperty(GrammarManager::DTMFTerm);
  if (j != NULL) {
    int dtmfterm;
    if (vxistring(j) == L"false")
      dtmfterm = 0;
    else
      dtmfterm = 1;

    AddParamValue(m, REC_TERMINATED_ON_DTMF, dtmfterm);
  }

  // (2.5) Timeout settings
  if (timeout == -1) {
    j = props.GetProperty(PROP_TIMEOUT);
    if (j != NULL)
      if(!PropertyList::ConvertTimeToMilliseconds(log, j, timeout))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
  }

  if (timeout != -1) {
    AddParamValue(m, REC_DTMF_TIMEOUT, timeout);
    AddParamValue(m, REC_TIMEOUT, timeout);
  }

  // (2.6) Inter-Digit timeout
  j = props.GetProperty(PROP_INTERDIGITTIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_DTMF_TIMEOUT_INTERDIGIT, time);
  }
  
  // (2.7) Terminator char timeout
  j = props.GetProperty(PROP_TERMTIME);
  if (j != NULL ) {
    if(!props.ConvertTimeToMilliseconds(log, j, time))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
    AddParamValue(m, REC_DTMF_TIMEOUT_TERMINATOR, time);
  } 

  // (2.8) DTMF terminator character
  j = props.GetProperty(PROP_TERMCHAR);
  if ((j != NULL) && (j[0] != L'\0')) {
    if (wcslen(j) == 1)
      AddParamValue(m, REC_DTMF_TERMINATOR_CHAR, j);
    else {
      log.StartDiagnostic(0) << L"GrammarManager::GetRecordProperties - "
        L"Unable to set " << REC_DTMF_TERMINATOR_CHAR << L" from value \"" <<
        j << L"\".  Defaulting to '#'.";
      log.EndDiagnostic();
      // Should we use the default, or rather not set anything ?
      AddParamValue(m, REC_DTMF_TERMINATOR_CHAR, L"#");
    }
  }

  // (2.9) Input modes
  j = props.GetProperty(PROP_INPUTMODES);
  SetInputMode(j, m);

  // (3) Done

  return m.Release();
}


void GrammarManager::SetGrammarLoadProperties(const VXMLElement & element, 
                                              const PropertyList& props,
                                              VXIMapHolder & properties) const
{
  vxistring attr;
  const VXIchar * j;

  // (1) Set weight
  VXIflt32 val;
  if (element.GetAttribute(ATTRIBUTE_WEIGHT, attr) == true &&
      PropertyList::ConvertValueToFraction(log, attr, val))
  {
    AddParamValue(properties, REC_GRAMMAR_WEIGHT, val);
  }
  else
    AddParamValue(properties, REC_GRAMMAR_WEIGHT, 1.0f);

  // (1.1) Set Grammar Mode; as a convenience
  attr = L"";
  if( element.GetAttribute(ATTRIBUTE_MODE, attr ) == true )
  {
      AddParamValue(properties, REC_GRAMMAR_MODE, attr );
  }

  // (2) Set language; parsing should ensure that this value is always defined
  attr = L"";
  if (!element.GetAttribute(ATTRIBUTE_XMLLANG, attr)) {
    log.LogError(999, SimpleLogger::MESSAGE,
                 L"GrammarManager::SetGrammarLoadProperties did not "
                 L"find a language");
  }
  AddParamValue(properties, REC_LANGUAGE, attr);

  // (3) Set prefetch status
  attr = L"";
  if (element.GetAttribute(ATTRIBUTE_FETCHHINT, attr) != true) {
    const VXIValue * v = VXIMapGetProperty(properties.GetValue(),
                                           L"grammarfetchhint");
    if (VXIValueGetType(v) == VALUE_STRING)
      attr = VXIStringCStr(reinterpret_cast<const VXIString *>(v));
  }

  if (attr == L"prefetch")
    AddParamValue(properties, REC_PREFETCH_REQUEST, 1);
  else
    AddParamValue(properties, REC_PREFETCH_REQUEST, 0); 
    
  // (4) Input Mode
  j = props.GetProperty(PROP_INPUTMODES);
  SetInputMode(j, properties);
}
