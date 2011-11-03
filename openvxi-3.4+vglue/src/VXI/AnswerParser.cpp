
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
#include "AnswerParser.hpp"
#include "CommonExceptions.hpp"
#include "SimpleLogger.hpp"
#include "VXIrec.h"                     // for a single type name (eliminate?)
#include "XMLChConverter.hpp"           // for xmlcharstring
#include <sstream>
#include <map>
#include <list>
#include <vector>

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

#include <framework/MemBufInputSource.hpp>
#include <sax/ErrorHandler.hpp>         // by XMLErrorReporter
#include <dom/DOM.hpp>
#include <sax/SAXParseException.hpp>
#include <util/XMLChar.hpp>
using namespace xercesc;

#ifndef WIN32
inline bool Compare(const XMLCh * a, const XMLCh * b)
{
  return XMLString::compareString(a, b) == 0;
}
#endif

// ---------------------------------------------------------------------------
class AnswerHolder {
public:
  AnswerHolder(const XMLCh* ut, const XMLCh* im, 
               const XMLCh* gid, const XMLCh* cf, float fCf,             
               GrammarManager & gm)
    : utterance(ut), inputmode(im), grammarid(gid), confidence(cf),       
      fConfidence(fCf), grammarInfo(0), gramMgr(gm), answerElement() { }
  
  void SetInstance(DOMElement *is) { instanceDomList.push_back(is); }
  void SetInstance(const xmlcharstring & is){ instanceStrList.push_back(is); }  

  bool operator<(const AnswerHolder & x) 
  {
    // Compare conf. score first
    if( fConfidence < x.fConfidence ) 
      return true;
    else if( fConfidence > x.fConfidence ) 
      return false;
    
    // Otherwise compare precedence if conf. score is the same
    switch(gramMgr.CompareGrammar(grammarInfo, x.grammarInfo)) {
      case -1: return false;
      default: return true;
    }
  }

  bool operator>(const AnswerHolder & x) 
  {
    return !operator<(x);
  }

  AnswerHolder & operator=(const AnswerHolder &x)
  {
    if (this != &x) {
      utterance = x.utterance;
      inputmode = x.inputmode;
      grammarid = x.grammarid;
      grammarInfo = x.grammarInfo;
      confidence = x.confidence;
      fConfidence = x.fConfidence;
      instanceDomList = x.instanceDomList;
      instanceStrList = x.instanceStrList;
      answerElement = x.answerElement;
    }
    return *this;
  }
  
  xmlcharstring utterance;
  const XMLCh* inputmode;
  const XMLCh* grammarid;
  const XMLCh* confidence;
  float fConfidence;
  unsigned long grammarInfo;
  GrammarManager & gramMgr;
  VXMLElement answerElement;
  
  typedef std::list<DOMElement*> DOMLIST;
  DOMLIST instanceDomList;
  typedef std::list<xmlcharstring> STRLIST;
  STRLIST instanceStrList;    
};  

typedef std::vector<AnswerHolder > ANSWERHOLDERVECTOR;
typedef std::map<VXIflt32, ANSWERHOLDERVECTOR, std::greater<VXIflt32> > ANSWERHOLDERMAP;

class AnswerParserErrorReporter : public ErrorHandler {
public:
  AnswerParserErrorReporter()  { }
  ~AnswerParserErrorReporter() { }

  void warning(const SAXParseException& toCatch)     { /* Ignore */ }
  void fatalError(const SAXParseException& toCatch)  { error(toCatch); }
  void resetErrors() { }

  void error(const SAXParseException & toCatch)
  { throw SAXParseException(toCatch); }

private:
  AnswerParserErrorReporter(const AnswerParserErrorReporter &);
  void operator=(const AnswerParserErrorReporter &);
};

// ---------------------------------------------------------------------------

AnswerParser::AnswerParser()
{
    //  Log to voiceglue
    /*
    if (voiceglue_loglevel() >= LOG_INFO)
    {
	std::ostringstream logstring;
	logstring << "AnswerParser: new XercesDOMParser()";
	voiceglue_log ((char) LOG_INFO, logstring);
    };
    */

  nlsmlParser = new XercesDOMParser();
  if (nlsmlParser == NULL)
    throw VXIException::OutOfMemory();

  nlsmlParser->setValidationScheme( XercesDOMParser::Val_Never);
  nlsmlParser->setDoNamespaces(false);
  nlsmlParser->setDoSchema(false);
  nlsmlParser->setValidationSchemaFullChecking(false);
  nlsmlParser->setCreateEntityReferenceNodes(false);

  // nlsmlParser->setToCreateXMLDeclTypeNode(true);

  ErrorHandler *errReporter = new AnswerParserErrorReporter();
  if (errReporter == NULL) {

    //  Log to voiceglue
      /*
    if (voiceglue_loglevel() >= LOG_INFO)
    {
	std::ostringstream logstring;
	logstring << "AnswerParser: delete XercesDOMParser";
	voiceglue_log ((char) LOG_INFO, logstring);
    };
      */

    delete nlsmlParser;
    throw VXIException::OutOfMemory();
  }

  nlsmlParser->setErrorHandler(errReporter);
}


AnswerParser::~AnswerParser()
{
  if (nlsmlParser != NULL) {
    const ErrorHandler * reporter = nlsmlParser->getErrorHandler();
    nlsmlParser->setErrorHandler(NULL);
    delete reporter;

    //  Log to voiceglue
    /*
    if (voiceglue_loglevel() >= LOG_INFO)
    {
	std::ostringstream logstring;
	logstring << "AnswerParser: delete XercesDOMParser";
	voiceglue_log ((char) LOG_INFO, logstring);
    };
    */

    delete nlsmlParser;
    nlsmlParser = NULL;
  }
}

inline bool AnswerParser::IsAllWhiteSpace(const XMLCh* str)
{
  if( !str ) return true;
  while( *str != 0 ) {
    if( *str != '\r' && *str != '\n' && *str != '\t' && *str != ' ')
      return false;
    ++str;  
  }
  return true;
}

AnswerParser::ParseResult AnswerParser::Parse(SimpleLogger & log, AnswerTranslator & translator,
                        GrammarManager & gm, int maxnbest,
                        VXIContent * document, VXMLElement & bestAnswer)
{
  if (document == NULL) return AnswerParser::InvalidParameter;

  // (1) Get document contents.
  const VXIchar * docType;
  const VXIbyte * docContent;
  VXIulong docContentSize = 0;

  if (VXIContentValue(document, &docType, &docContent, &docContentSize) !=
      VXIvalue_RESULT_SUCCESS) return AnswerParser::InvalidParameter;
  if (docContent == NULL || docContentSize == 0) return AnswerParser::InvalidParameter;

  if (log.IsLogging(3)) {
    VXIString *key = NULL, *value = NULL;
    if (log.LogContent(docType, docContent, docContentSize,
                       &key, &value))
    {
      vxistring temp = VXIStringCStr(key);
      temp += L": ";
      temp += VXIStringCStr(value);
      log.StartDiagnostic(0) << L"AnswerParser::Parse - Raw NLSML result saved in: "
                             << temp.c_str();
      log.EndDiagnostic();
    }
    if( key ) VXIStringDestroy(&key);
    if( value ) VXIStringDestroy(&value);
  }

  // (2) Parse NLSML format.
  vxistring type(docType);

  if (type != VXIREC_MIMETYPE_XMLRESULT) {
	  log.StartDiagnostic(0) << L"AnswerParser::Parse - unsupported content: "
                             << type.c_str();
      log.EndDiagnostic();
	  return AnswerParser::UnsupportedContent;
  }
  else {
    try {
      VXIcharToXMLCh membufURL(L"nlsml recognition result");
      MemBufInputSource buf(docContent,docContentSize,membufURL.c_str(),false);
      nlsmlParser->parse(buf);
    }
    catch (const XMLException & exception) {
      if (log.IsLogging(0)) {
        XMLChToVXIchar message(exception.getMessage());
        log.StartDiagnostic(0) << L"AnswerParser::Parse - XML parsing "
          L"error from DOM: " << message;
        log.EndDiagnostic();
      }
	  return AnswerParser::ParseError;
    }
    catch (const SAXParseException & exception) {
      if (log.IsLogging(0)) {
        XMLChToVXIchar message(exception.getMessage());
        log.StartDiagnostic(0) << L"AnswerParser::Parse - Parse error at line "
                               << exception.getLineNumber()
                               << L", column " << exception.getColumnNumber()
                               << L" - " << message;
        log.EndDiagnostic();
      }
      return AnswerParser::ParseError;
    }

    switch (ProcessNLSML(log, translator, gm, maxnbest, bestAnswer)) {
    case -1:
      return AnswerParser::ParseError;
    case 0:
      return AnswerParser::Success;
    case 1:
      return AnswerParser::NoInput;
    case 2:
      return AnswerParser::NoMatch;
    default:
      break;
    }
  }

  return AnswerParser::Success;
}

// ---------------------------------------------------------------------------
// NLSML Parser
// ---------------------------------------------------------------------------

static const XMLCh NODE_RESULT[]
    = { 'r','e','s','u','l','t','\0' };
static const XMLCh NODE_INTERPRETATION[]
    = { 'i','n','t','e','r','p','r','e','t','a','t','i','o','n','\0' };
static const XMLCh NODE_INPUT[]
    = { 'i','n','p','u','t','\0' };
static const XMLCh NODE_INSTANCE[]
    = { 'i','n','s','t','a','n','c','e','\0' };
static const XMLCh NODE_NOINPUT[]
    = { 'n','o','i','n','p','u','t','\0' };
static const XMLCh NODE_NOMATCH[]
    = { 'n','o','m','a','t','c','h','\0' };

static const XMLCh ATTR_GRAMMAR[]
    = { 'g','r','a','m','m','a','r','\0' };
static const XMLCh ATTR_CONFIDENCE[]
    = { 'c','o','n','f','i','d','e','n','c','e','\0' };
static const XMLCh ATTR_MODE[]
    = { 'm','o','d','e','\0' };

static const XMLCh DEFAULT_CONFIDENCE[]
    = { '1','0','0','\0' };
static const XMLCh DEFAULT_MODE[]
    = { 's','p','e','e','c','h','\0' };


// This function is used by the other NLSMLSetVars functions to set everything
// but the 'interpretation'.
//
void NLSMLSetSomeVars(vxistring variable, AnswerTranslator & translator,
                      const XMLCh * confidence, const XMLCh *utterance,
                      const XMLCh * mode)
{
  // (1) Set the 'confidence'.
  vxistring expr = variable + L".confidence = ";
  expr += XMLChToVXIchar(confidence).c_str();
  expr += L" / 100;";
  translator.EvaluateExpression(expr);

  // (2) Set the 'utterance'
  translator.SetString(variable + L".utterance",
                       XMLChToVXIchar(utterance).c_str());

  // (3) Set the 'inputmode'
  if (Compare(mode, DEFAULT_MODE))
    translator.SetString(variable + L".inputmode", L"voice");
  else
    translator.SetString(variable + L".inputmode",
                         XMLChToVXIchar(mode).c_str());
}


// This sets all of the application.lastresult$ array when the interpretation
// is a simple result.
//
bool NLSMLSetVars(AnswerTranslator & translator, int nbest,
                  const XMLCh * confidence, const XMLCh * utterance,
                  const XMLCh * mode, const XMLCh * interp)
{
  std::basic_ostringstream<VXIchar> out;
  out << L"application.lastresult$[" << nbest << L"]";
  translator.EvaluateExpression(out.str() + L" = new Object();");

  // Set everything but interpretation.
  NLSMLSetSomeVars(out.str(), translator, confidence, utterance, mode);

  // Set 'interpretation'.
  translator.SetString(out.str() + L".interpretation",
                       XMLChToVXIchar(interp).c_str());

  return true;
}

// This function recursively processes the data within an <instance>.
//
bool NLSMLSetInterp(AnswerTranslator & translator, SimpleLogger & log,
                    vxistring & path, const DOMNode * interp)
{
  bool foundText = false;

  // Look for text first.
  DOMNode * temp = NULL;
  vxistring textValue(L"");
  for (temp = interp->getFirstChild(); temp != NULL;
       temp = temp->getNextSibling())
  {
    if (temp->getNodeType() != DOMNode::TEXT_NODE &&
        temp->getNodeType() != DOMNode::CDATA_SECTION_NODE ) continue;
    foundText = true;
    textValue += XMLChToVXIchar(temp->getNodeValue()).c_str();
  }
  
  if( foundText )
    translator.SetString(path, textValue.c_str());
  
  bool foundElement = false;

  // Now try for elements.
  for (temp = interp->getFirstChild(); temp != NULL;
       temp = temp->getNextSibling())
  {
    if (temp->getNodeType() != DOMNode::ELEMENT_NODE) continue;
    if (foundText) {
      log.StartDiagnostic(0)
        << L"AnswerParser::ProcessNLSML - malformed <instance> at "
        << path << L"; mixed text and elements";
      log.EndDiagnostic();
      return false;
    }
    if (!foundElement) {
      translator.EvaluateExpression(path + L" = new Object();");
      foundElement = true;
    }

    // Although ECMAScript arrays are treated like properties, calling
    // x.1 = 'val' is invalid.  Instead, x[1] = 'val' must be used.
    XMLChToVXIchar name(temp->getNodeName());
    VXIchar first = *(name.c_str());
    bool isArray = (first=='0' || first=='1' || first=='2' || first=='3' || 
                    first=='4' || first=='5' || first=='6' || first=='7' ||
                    first=='8' || first=='9');

    vxistring newPath = path;
    if (isArray)
      newPath += L"[";
    else
      newPath += L".";
    newPath += name.c_str();
    if (isArray)
      newPath += L"]";

    NLSMLSetInterp(translator, log, newPath, temp);
  }

  if (!foundElement && !foundText) {
    log.StartDiagnostic(0)
      << L"AnswerParser::ProcessNLSML - malformed <instance> at "
      << path << L"; no data found";
    log.EndDiagnostic();
    return false;
  }

  return true;
}


// This sets all of the application.lastresult$ array when <instance> contains
// a complex result.
//
bool NLSMLSetVars(AnswerTranslator & translator, SimpleLogger & log,
                  int nbest, const XMLCh * confidence,
                  const XMLCh * utterance, const XMLCh * mode,
                  const DOMNode * interp)
{
  std::basic_ostringstream<VXIchar> out;
  out << L"application.lastresult$[" << nbest << L"]";
  translator.EvaluateExpression(out.str() + L" = new Object();");

  // Set everything but interpretation.
  NLSMLSetSomeVars(out.str(), translator, confidence, utterance, mode);

  // Set 'interpretation'.
  vxistring path = out.str() + L".interpretation";
  return NLSMLSetInterp(translator, log, path, interp);
}

// This function set up lastresult$ n best
//
static bool SetUpNBestList(int *nbest, bool *isAmbigous, SimpleLogger & log, 
                          AnswerTranslator & translator, AnswerHolder & ans) 
{
  // if there is an instance of DOMElement type
  // setting these instance
  int index = 0;
  if( ans.instanceStrList.empty() ) {
    AnswerHolder::DOMLIST::iterator pos;
    for(pos = ans.instanceDomList.begin(); 
        pos != ans.instanceDomList.end(); ++pos, ++index) {

      if (index > 0)
        *isAmbigous = true;

      if (log.IsLogging(2)) {
         log.StartDiagnostic(2) << L"SetUpNBestList(" << *nbest
         << L", " << XMLChToVXIchar(ans.grammarid).c_str() << L")";
         log.EndDiagnostic();
      }     

      if (!NLSMLSetVars(translator, log, *nbest, ans.confidence, 
                        ans.utterance.c_str(), ans.inputmode, (*pos)))
      {
        log.LogDiagnostic(0, L"AnswerParser: <instance> element "
                      L"processing failed");
        return false;
      }
      ++(*nbest);
    }  
  } 
  else {
    // Use the input text(utterance) as instance
    AnswerHolder::STRLIST::iterator pos;
    for(pos = ans.instanceStrList.begin(); 
        pos != ans.instanceStrList.end(); ++pos, ++index) {

      if (index > 0)
        *isAmbigous = true;

      if (log.IsLogging(2)) {
         log.StartDiagnostic(2) << L"SetUpNBestList(" << *nbest
         << L", " << XMLChToVXIchar(ans.grammarid).c_str() << L")";
         log.EndDiagnostic();
      }

      NLSMLSetVars(translator, *nbest, ans.confidence, 
                   ans.utterance.c_str(), ans.inputmode, (*pos).c_str());
      ++(*nbest);
    }
  } 
  return true;
}

// This function converts an NLSML result into ECMAScript.
//
int AnswerParser::ProcessNLSML(SimpleLogger & log,
                               AnswerTranslator & translator,
                               GrammarManager & gm, int maxnbest,
                               VXMLElement & bestAnswer)
{
  DOMNode * doc = nlsmlParser->getDocument();
  if (doc == NULL || !doc->hasChildNodes()) {
    log.LogDiagnostic(0, L"AnswerParser::ProcessNLSML - "
                      L"Document parse failed");
    return -1;
  }

  // (1) <result> element
  DOMElement * result = NULL;
  DOMNode * temp = NULL;
  for (temp = doc->getFirstChild(); temp != NULL;
       temp = temp->getNextSibling())
  {
    switch (temp->getNodeType()) {
    case DOMNode::ELEMENT_NODE:
      if (result == NULL)
        result = reinterpret_cast<DOMElement *>(temp);
      else {
        log.LogDiagnostic(0,L"AnswerParser: document must have a single root");
        return -1;
      }
      break;
    case DOMNode::TEXT_NODE:
      log.LogDiagnostic(0, L"AnswerParser: PCDATA not allowed after <xml>");
      return -1;
    default:
      continue;
    }
  }

  if (result == NULL || !Compare(result->getNodeName(), NODE_RESULT)) {
    log.LogDiagnostic(0, L"AnswerParser: document must have a single "
                      L"root named 'result'");
    return -1;
  }

  const XMLCh * resultGrammar = result->getAttribute(ATTR_GRAMMAR);

  // (2) <interpretation> elements

  int nbest = -1;
  ANSWERHOLDERMAP answerMap;

  for (DOMNode * temp2 = result->getFirstChild(); temp2 != NULL;
       temp2 = temp2->getNextSibling())
  {
    switch (temp2->getNodeType()) {
    case DOMNode::ELEMENT_NODE:
      break;   
    case DOMNode::TEXT_NODE: 
      // check for white space
      if (IsAllWhiteSpace(temp2->getNodeValue())) continue;               
      log.LogDiagnostic(0, L"AnswerParser: PCDATA not allowed after <result>");
      return -1;
    default:
      continue;
    }

    DOMElement * interpretation = reinterpret_cast<DOMElement *>(temp2);

    // (2.1) Is this an interpretation element?
    if (!Compare(interpretation->getNodeName(), NODE_INTERPRETATION)) {
      log.LogDiagnostic(0, L"AnswerParser: only <interpretation> elements may "
                        L"follow <result>");
      return -1;
    }

    // (2.2) Get the grammar attribute.
    const XMLCh * grammar = interpretation->getAttribute(ATTR_GRAMMAR);
    if (grammar == NULL || grammar[0] == 0) grammar = resultGrammar;

    // (2.3) Get the confidence attribute.
    const XMLCh * interpretationConfidence
      = interpretation->getAttribute(ATTR_CONFIDENCE);

    // (3) <input> element.

    // (3.1) Find the <input> element.  There can be only one.
    DOMElement * input = NULL;

    for (temp = interpretation->getFirstChild(); temp != NULL;
         temp = temp->getNextSibling())
    {
      switch (temp->getNodeType()) {
      case DOMNode::TEXT_NODE:
        // check for white space
        if( IsAllWhiteSpace(temp->getNodeValue())) continue;               
        log.LogDiagnostic(0, L"AnswerParser: PCDATA not allowed after "
                          L"<interpretation>");
        return -1;
      case DOMNode::ELEMENT_NODE:
        if (Compare(temp->getNodeName(), NODE_INSTANCE)) continue;
        if (!Compare(temp->getNodeName(), NODE_INPUT)) {
          log.LogDiagnostic(0, L"AnswerParser: Only <input> and <instance> "
                            L"elements are allowed in an <interpretation>");
          return -1;
        }
        if (input != NULL) {
          log.LogDiagnostic(0, L"AnswerParser: Only one <input> element is "
                            L"allowed in an <interpretation>");
          return -1;
        }
        input = reinterpret_cast<DOMElement *>(temp);
        break;
      default:
        continue;
      }
    }

    // (3.1.5) Get the input mode.
    const XMLCh * mode = input->getAttribute(ATTR_MODE);
    if (mode == NULL || mode[0] == 0) mode = DEFAULT_MODE;


    // (3.2) Look at the data inside of <input>.  Is this an event?

    xmlcharstring inputText;
    for (temp = input->getFirstChild(); temp != NULL;
         temp = temp->getNextSibling())
    {
      short type = temp->getNodeType();
      switch (type) {
      case DOMNode::CDATA_SECTION_NODE:
      case DOMNode::TEXT_NODE:
        inputText += temp->getNodeValue();
        break;
      case DOMNode::ELEMENT_NODE:
        // NOTE: The nbest count here is one less than the array value.  The
        //       value is not incremented until the <instance> element is
        //       processed.  Thus, the checks are against -1 NOT 0.
        //
        if (Compare(temp->getNodeName(), NODE_NOINPUT)) {
          if (nbest == -1) return 1;
          return 0; // Ignore this and subsequent interpretations.
        }
        if (Compare(temp->getNodeName(), NODE_NOMATCH)) {

          //special case of setting lastresult$ input mode
          translator.EvaluateExpression(L"application.lastresult$ = "
                                        L"new Array();");
          translator.EvaluateExpression(L"application.lastresult$[0] = new Object();");

          if (Compare(mode, DEFAULT_MODE))
            translator.SetString(L"application.lastresult$[0].inputmode", L"voice");
          else
            translator.SetString(L"application.lastresult$[0].inputmode", XMLChToVXIchar(mode).c_str());

          if (nbest == -1) return 2;
          return 0; // Ignore this and subsequent interpretations.
        }

        log.LogDiagnostic(0, L"AnswerParser: Only <noinput> and <nomatch> "
                          L"elements or PCDATA is allowed in an <input>");
        return -1;
      default:
        continue;
      }
    }
     
    if (inputText.empty()) {
      log.LogDiagnostic(0, L"AnswerParser: <input> element may not be empty");
      return -1;
    }

    if (grammar == NULL) {
      log.LogDiagnostic(0, L"AnswerParser: the grammar attribute must be "
                        L"defined in either <result> or <interpretation>");
      return -1;
    }

    // (3.3) Get the confidence.
    const XMLCh * confidence = input->getAttribute(ATTR_CONFIDENCE);
    if (confidence == NULL || confidence[0] == 0)
      confidence = interpretationConfidence;
    if (confidence == NULL || confidence[0] == 0)
      confidence = DEFAULT_CONFIDENCE;
    
    // (3.3.1) convert confidence score to float
    VXIflt32 vConfidence = 0.5;
    std::basic_stringstream<VXIchar> confStream(XMLChToVXIchar(confidence).c_str());
    confStream >> vConfidence;    
    
    // (3.5) Determine precedence of this grammar
    
    // (3.5.1) retrieve the grammar for this interpretation

    // create the answer info.
    AnswerHolder newanswer(inputText.c_str(), mode, grammar, confidence, vConfidence, gm);
    
    // find this answer grammar
    VXMLElement answerElement;
    if (!gm.FindMatchedElement(XMLChToVXIchar(newanswer.grammarid).c_str(), 
                               answerElement, newanswer.grammarInfo) ||
        answerElement == 0)
    {
      // No match found!
      log.LogError(433);
      return 2;
    }
    
    // set the answer element here
    newanswer.answerElement = answerElement;
    
    // find the current confidence in answer map
    ANSWERHOLDERMAP::iterator mit = answerMap.find(vConfidence);
    AnswerHolder *newAnswerPtr = NULL;
    // insert answer with confidence sorted in order
    if( mit == answerMap.end()) {
      // this score is first one in this list, create it
      ANSWERHOLDERVECTOR newlist;
      newlist.push_back(newanswer);
      answerMap[vConfidence] = newlist;
      newAnswerPtr = &(answerMap[vConfidence].back());
    } 
    else {      
      (*mit).second.push_back(newanswer);
      newAnswerPtr = &((*mit).second.back());     
    }         

    // (4) Look at each <instance>.
    //
    // At this point, we have DOMStrings with the 'grammar' name, 'confidence'
    // score, input 'mode', and 'inputText'.  We have already verified that
    // only <input> and <instance> nodes exist at this level.

    bool hasInstance = false;

    for (DOMNode * temp3 = interpretation->getFirstChild(); temp3 != NULL;
         temp3 = temp3->getNextSibling())
    {
      if (temp3->getNodeType() != DOMNode::ELEMENT_NODE) continue;
      if (!Compare(temp3->getNodeName(), NODE_INSTANCE)) continue;
      hasInstance = true;
      ++nbest;

      // (4.2) Process the instance

      // (4.2.1) Does the instance have data?

      DOMElement * instance = reinterpret_cast<DOMElement *>(temp3);
      bool hasInstanceData = false;

      for (DOMNode * temp4 = instance->getFirstChild();
           temp4 != NULL && !hasInstanceData;
           temp4 = temp4->getNextSibling())
      {
        short type = temp4->getNodeType();
        switch (type) {
        case DOMNode::CDATA_SECTION_NODE:
        case DOMNode::TEXT_NODE:
        case DOMNode::ELEMENT_NODE:
          newAnswerPtr->SetInstance(instance);
          hasInstanceData = true;
          break;
        default:
          continue;
        }
      }
      
      // (4.2.2) If the contents were empty - use input text as interpretation.
      if (!hasInstanceData) {
        newAnswerPtr->SetInstance(inputText);       
      }
    }
    
    // (4.2.3) If <instance> did not appear - use input text as interpretation.
    if (!hasInstance) {
      newAnswerPtr->SetInstance(inputText);             
    }
  }

  // Setting up the lastresult$ structure
  ANSWERHOLDERMAP::iterator it = answerMap.begin();
  if( it == answerMap.end() ) {
    // something is wrong, return error
    log.LogDiagnostic(0, L"AnswerParser: the result structure is corrupted");                         
    return -1;
  }
  else {    
    // go through the vector and sort its precedence using selection sort
    for( it = answerMap.begin(); it != answerMap.end(); ++it ) {
      ANSWERHOLDERVECTOR & a = (*it).second;
      int vsize = a.size();
      if( vsize <= 1 ) continue;
      for(int i = 0; i < vsize; i++ ) {
        int min = i;
        for( int j = i+1; j < vsize; j++)
          if (a[j] < a[min]) min = j;       
        AnswerHolder t = a[min]; a[min] = a[i]; a[i] = t;
      }
    }
    
    // go through the vector and setup lastresult structure
    int nbestIndex = 0;
    bool done = false, isAmbigous = false;
    for( it = answerMap.begin(); !done && it != answerMap.end(); ++it ) {
      ANSWERHOLDERVECTOR & a = (*it).second;
      int vsize = a.size();
      for( int i = 0; !done && i < vsize; i++) {
        if( nbestIndex == 0 ) {
          // create the result structure at first element
          translator.EvaluateExpression(L"application.lastresult$ = "
                                        L"new Array();");
          bestAnswer = a[i].answerElement;
        }                 

        if( !SetUpNBestList(&nbestIndex, &isAmbigous, log, translator, a[i]))
          return -1;

        // check for maxnbest, if there is ambiguity, ignore maxnbest
        // and setup appropriate nbest list
        if( isAmbigous && nbestIndex > maxnbest-1 ) done = true;
        else if( nbestIndex > maxnbest -1 ) done = true;
      }       
    }     
  }
  return 0;
}

