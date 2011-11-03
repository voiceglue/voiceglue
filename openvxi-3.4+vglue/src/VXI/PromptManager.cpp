
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

#include "PromptManager.hpp"
#include "SimpleLogger.hpp"            // for SimpleLogger
#include "VXIprompt.h"                 // for VXIpromptInterface
#include "VXML.h"                      // for node names
#include "DocumentModel.hpp"           // for VXMLNode, VXMLElement
#include "CommonExceptions.hpp"        // for InterpreterEvent, OutOfMemory
#include "PropertyList.hpp"            // for PropertyList
#include <sstream>
#include <iostream>

static VXIchar SPACE = ' ';

//#############################################################################

bool static ConvertValueToString(const VXIValue * value, vxistring & source)
{
  if (value == NULL) return false;

  switch (VXIValueGetType(value)) {
  case VALUE_BOOLEAN:
  {
    std::basic_stringstream<VXIchar> attrStream;
    if(VXIBooleanValue(reinterpret_cast<const VXIBoolean *>(value)) == TRUE)
      attrStream << L"true";
    else
      attrStream << L"false";
    source = attrStream.str();
    return true;
  }
  case VALUE_INTEGER:
  {
    std::basic_stringstream<VXIchar> attrStream;
    attrStream << VXIIntegerValue(reinterpret_cast<const VXIInteger *>(value));
    source = attrStream.str();
    return true;
  }
  case VALUE_FLOAT:
  {
    std::basic_stringstream<VXIchar> attrStream;
    attrStream << VXIFloatValue(reinterpret_cast<const VXIFloat *>(value));
    source = attrStream.str();
    return true;
  }
#if (VXI_CURRENT_VERSION >= 0x00030000)
  case VALUE_DOUBLE:
  {
    std::basic_stringstream<VXIchar> attrStream;
    attrStream << VXIDoubleValue(reinterpret_cast<const VXIDouble *>(value));
    source = attrStream.str();
    return true;
  }  
  case VALUE_ULONG:
  {
    std::basic_stringstream<VXIchar> attrStream;
    attrStream << VXIULongValue(reinterpret_cast<const VXIULong *>(value));
    source = attrStream.str();
    return true;
  }
#endif  
  case VALUE_STRING:
    source = VXIStringCStr(reinterpret_cast<const VXIString *>(value));
    return true;
  default:
    return false;
  }
}


inline void AddSSMLAttribute(const VXMLElement & elem, VXMLAttributeType attr,
                             const VXIchar * const defaultVal,
                             const VXIchar * const name, vxistring & sofar)
{
  // Get attribute or use the default value.
  vxistring attrString;
  if (!elem.GetAttribute(attr, attrString) || attrString.empty()) {
    if (defaultVal == NULL) return;
    attrString = defaultVal;
  }

  sofar += SPACE;
  sofar += name;
  sofar += L"=\"";

  // We need to escape three characters: (",<,&) -> (&quot;, &lt;, &amp;)
  vxistring::size_type pos = 0;
  while ((pos = attrString.find_first_of(L"\"<&", pos)) != vxistring::npos) {
    switch (attrString[pos]) {
    case '\"':   attrString.replace(pos, 1, L"&quot;");    ++pos;  break;
    case '<':    attrString.replace(pos, 1, L"&lt;");      ++pos;  break;
    case '&':    attrString.replace(pos, 1, L"&amp;");     ++pos;  break;
    }
  };

  sofar += attrString;
  sofar += L"\"";
}


inline void AppendTextSegment(vxistring & sofar, const vxistring & addition)
{
  if (addition.empty()) return;

  vxistring::size_type pos = sofar.length();

  sofar += addition;

  // We need to escape two characters: (<,&) -> (&lt;, &amp;)
  // NOTE: It starts from the old length and operates only on the addition.
  while ((pos = sofar.find_first_of(L"<&", pos)) != vxistring::npos) {
    switch (sofar[pos]) {
    case '<':    sofar.replace(pos, 1, L"&lt;");      ++pos;  break;
    case '&':    sofar.replace(pos, 1, L"&amp;");     ++pos;  break;
    }
  }
}

//#############################################################################

////////
// About prompts:
//
// Prompts may be defined in two places within VXML documents.
// 1) form items - menu, field, initial, record, transfer, subdialog, object
// 2) executable content - block, catch, filled
//
// The <enumerate> element only is valid with parent menu or fields having one
// or more option elements.  Otherwise an error.semantic results.
//
// From the VXML working drafts, barge-in may be controlled on a prompt by
// prompt basis.  There are several unresolved issues with this request.
// Instead this implements a different tactic.

void PromptManager::ThrowEventIfError(VXIpromptResult rc)
{
  switch( rc )
  {
    case VXIprompt_RESULT_SUCCESS:
      break;
    case VXIprompt_RESULT_TTS_BAD_LANGUAGE:
      throw VXIException::InterpreterEvent(EV_UNSUPPORT_LANGUAGE);
    case VXIprompt_RESULT_HW_BAD_TYPE:
      throw VXIException::InterpreterEvent(EV_UNSUPPORT_FORMAT);
    case VXIprompt_RESULT_NO_RESOURCE:
      throw VXIException::InterpreterEvent(EV_ERROR_NORESOURCE);
    case VXIprompt_RESULT_NO_AUTHORIZATION:
      throw VXIException::InterpreterEvent(EV_ERROR_NOAUTHORIZ);
    default:  // ignore all other errors, just pass through
      break;
  }   
}

void PromptManager::WaitAndCheckError()
{
  VXIpromptResult rc = VXIprompt_RESULT_SUCCESS;
  prompt->Wait(prompt, &rc);
  ThrowEventIfError(rc);
}

bool PromptManager::ConnectResources(SimpleLogger * l, VXIpromptInterface * p, ExecutableContentHandler *ech)
{
  if (l == NULL || p == NULL || ech == NULL) return false;
  log = l;
  prompt = p;
  contentHandler = ech;
  enabledSegmentInQueue = false;
  playedBargeinDisabledPrompt = false;
  timeout = -1;
  return true;
}


void PromptManager::PlayFiller(const vxistring & src, 
                               const VXIMapHolder & properties)
{
  log->LogDiagnostic(2, L"PromptManager::PlayFiller()");

  if (src.empty()) return;

  const VXIMap * propMap = properties.GetValue();
  // Get fetchaudiominimum value
  VXIint minPlayMsec = 0;
  const VXIValue * val = (VXIMapGetProperty(propMap, L"fetchaudiominimum"));
  if( val &&  VXIValueGetType(val) == VALUE_STRING ) {
    const VXIchar* fmin = 
          VXIStringCStr(reinterpret_cast<const VXIString*>(val));
    if( fmin )
      if(!PropertyList::ConvertTimeToMilliseconds(*log, fmin, minPlayMsec))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
  } 
  prompt->PlayFiller(prompt, NULL, src.c_str(), NULL, propMap, minPlayMsec);
}


void PromptManager::Play(bool *playedBDPrompt)
{
  log->LogDiagnostic(2, L"PromptManager::Play()");

  *playedBDPrompt = playedBargeinDisabledPrompt;

  WaitAndCheckError();
  prompt->Play(prompt);
  enabledSegmentInQueue = false;
  playedBargeinDisabledPrompt = false;
  timeout = -1;
}


void PromptManager::PlayAll()
{
  log->LogDiagnostic(2, L"PromptManager::PlayAll()");

  prompt->Play(prompt);
  WaitAndCheckError();
  enabledSegmentInQueue = false;
  playedBargeinDisabledPrompt = false;
  timeout = -1;
}


int PromptManager::GetMillisecondTimeout() const
{
  return timeout;
}

void PromptManager::Queue(const VXMLElement & elem, const VXMLElement & ref,
                          const PropertyList & propertyList,
                          PromptTranslator & translator)
{
  log->LogDiagnostic(2, L"PromptManager::Queue()");

  // (1) Find <field> or <menu> associated with this prompt, if any.  This is
  // used by the <enumerate> element.
  VXMLElement item = ref;
  while (item != 0) {
    VXMLElementType type = item.GetName();
    if (type == NODE_FIELD || type == NODE_MENU) break;
    item = item.GetParent();
  }

  // (1.1) Search for <catch>, if found in root application
  // need to replace SSML xml:base with the current one
  bool replaceXmlBase = false;
  VXMLElement nParent = elem.GetParent();
  while( nParent != 0 ) {
    if( nParent.GetName() == NODE_CATCH && 
        nParent.GetDocumentLevel() == APPLICATION ) 
    {
      replaceXmlBase = true;
      break;
    }  
    nParent = nParent.GetParent();
  }
   
  // (2) Get properties.
  VXIMapHolder properties;
  if (properties.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (2.1) Flatten the property list.
  propertyList.GetProperties(properties);

  // (2.2) Add the URI information.
  propertyList.GetFetchobjBase(properties);

  // (3) Handle <prompt> elements

  // (3.1) Update timeout if specified.
  vxistring timeoutString;
  if (elem.GetAttribute(ATTRIBUTE_TIMEOUT, timeoutString) == true)
    if(!PropertyList::ConvertTimeToMilliseconds(*log, timeoutString, timeout))
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);

  // (3.2) Get bargein setting.

  PromptManager::BargeIn bargein = PromptManager::ENABLED;
  const VXIchar * j = propertyList.GetProperty(PROP_BARGEIN);
  if (j != NULL && vxistring(L"false") == j)
    bargein = PromptManager::DISABLED;

  vxistring bargeinAttr;
  if (elem.GetAttribute(ATTRIBUTE_BARGEIN, bargeinAttr) == true) {
    bargein = PromptManager::ENABLED;
    if (bargeinAttr == L"false") bargein = PromptManager::DISABLED;
    // override default bargein attribute
    VXIMapSetProperty(properties.GetValue(), PROP_BARGEIN, 
                     (VXIValue*)VXIStringCreate(bargeinAttr.c_str())); 
  }
  
  vxistring bargeinType;
  if (elem.GetAttribute(ATTRIBUTE_BARGEINTYPE, bargeinType) == true) {
    // override default bargeintype attribute
    VXIMapSetProperty(properties.GetValue(), PROP_BARGEINTYPE, 
                     (VXIValue*)VXIStringCreate(bargeinType.c_str())); 
  }
   
  // (4) Build prompt, recursively handling the contents.
  vxistring content, ssmlHeader;
  bool hasPromptData = false;
  for (VXMLNodeIterator it(elem); it; ++it)
    ProcessSegments(*it, item, propertyList, translator,
                    bargein, properties, content, ssmlHeader, hasPromptData, replaceXmlBase);
  if (content.empty()) return;

  // (5) Add a new segment.
  AddSegment(SEGMENT_SSML, content, properties, bargein);
}

bool PromptManager::AddSegment(PromptManager::SegmentType type,
                               const vxistring & data,
                               const VXIMapHolder & properties,
                               PromptManager::BargeIn bargein,
                               bool throwExceptions)
{
  const VXIMap * propMap = properties.GetValue();

  VXIpromptResult result;

  // Handle the easy case.
  switch (type) {
  case PromptManager::SEGMENT_AUDIO:
    result = prompt->Queue(prompt, NULL, data.c_str(), NULL, propMap);
    break;
  case PromptManager::SEGMENT_SSML:
    result = prompt->Queue(prompt, VXI_MIME_SSML, NULL, data.c_str(), propMap);
    break;
  }

  if (throwExceptions) {
    ThrowEventIfError(result);
  }

  switch (bargein) {
  case PromptManager::DISABLED:
    if (!enabledSegmentInQueue) {
      prompt->Play(prompt);
      playedBargeinDisabledPrompt = true;
    }
    break;
  case PromptManager::ENABLED:
    enabledSegmentInQueue = true;
    break;
  case PromptManager::UNSPECIFIED:
    break;
  }

  return result == VXIprompt_RESULT_SUCCESS;
}

void PromptManager::ProcessSegments(const VXMLNode & node,
                                    const VXMLElement & item,
                                    const PropertyList & propertyList,
                                    PromptTranslator & translator,
                                    PromptManager::BargeIn bargein,
                                    VXIMapHolder & props,
                                    vxistring & sofar,
                                    vxistring & ssmlHeader,
                                    bool & hasPromptData,
                                    bool replaceXmlBase)
{
  // (1) Handle bare content.
  switch (node.GetType()) {
  case VXMLNode::Type_VXMLContent:
  {
    const VXMLContent & content = reinterpret_cast<const VXMLContent &>(node);
    const vxistring & data = content.GetValue();
    if( sofar.empty() && ssmlHeader.empty() )
      ssmlHeader = data.substr(0, content.GetSSMLHeaderLen());      
    // If the content contains only </speak> and there is no audio 
    // the prompt is empty, no need to queue empty prompt!!!
    if( data == L"</speak>" && sofar.length() <= ssmlHeader.length() )
      sofar = L"";
    else {
      bool done = false;          
      const VXIchar* baseuri = propertyList.GetProperty(PropertyList::BaseURI);
      if( replaceXmlBase && baseuri && sofar.empty() && !ssmlHeader.empty()) {
        // In case of the prompt being played from the <catch> element which 
        // originated from the root application.  According to VoiceXML 2.0
        // specs. All relative uri must be resolved to the active document due
        // to the as-if-by-copy semantic
        vxistring::size_type idx = ssmlHeader.find(L"xml:base");
        vxistring::size_type idxDelim1, idxDelim2;
        if( *baseuri != L'\0' && idx != vxistring::npos ) {
          // Found the xml:base attribute
          // Now search for delimiter "
          idxDelim1 = ssmlHeader.find(L"\"", idx);
          if( idxDelim1 != vxistring::npos )
          {
            // Found delimiter "
            idxDelim2 = ssmlHeader.find(L"\"", idxDelim1 + 1);              
          }
          else {
            // Try to find delimiter '
            idxDelim1 = ssmlHeader.find(L"'", idx);
            idxDelim2 = ssmlHeader.find(L"'", idxDelim1 + 1);                        
          }
          // Replace base uri
          if( idxDelim1 != vxistring::npos && 
              idxDelim2 != vxistring::npos ) {
            idxDelim1++;
            vxistring buri = baseuri;
            idx = buri.find_first_of(L"?");
            if( idx != vxistring::npos ) buri = buri.substr(0,idx);
            ssmlHeader.replace(idxDelim1, idxDelim2-idxDelim1, buri);
            sofar += ssmlHeader;
            sofar += data.substr(content.GetSSMLHeaderLen());
            done = true;
          }
        }
      }
      // pass-through
      if( !done ) {
        sofar += data;
      }
    }
    hasPromptData = (sofar.length() > ssmlHeader.length());
    return;
  }
  case VXMLNode::Type_VXMLElement:
    break;
  default: // This should never happen.
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"unexpected type in PromptManager::ProcessSegments");
    throw VXIException::Fatal();
  }

  // (2) Retrieve fetch properties (if we haven't already)
  const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(node);
  vxistring attr;

  switch (elem.GetName()) {

  // (3) audio
  case NODE_AUDIO:
  {
    // (3.1) Process the 'expr' if provided.  NOTE: Exactly one of 'src' and 
    //       'expr' MUST be specified.

    vxistring expr;
    
    if (elem.GetAttribute(ATTRIBUTE_EXPR, expr)) {
      // (3.1.1) Handle the expr case.
      VXIValue * value = translator.EvaluateExpression(expr);
      if (value == NULL) return;  // Ignore this element
  
      // (3.1.2) Handle audio content from <record> elements.
      if (VXIValueGetType(value) == VALUE_CONTENT) {
        // Queue the current prompt segment
        if( hasPromptData )
        {
          sofar += L"</speak>";
          AddSegment(SEGMENT_SSML, sofar, props, bargein);
        }
        // reset 
        sofar = ssmlHeader;
        AddContent(props, sofar, value);
        // Referencing recorded data by queuing mark name
        sofar += L"</speak>";
        AddSegment(SEGMENT_SSML, sofar, props, bargein);
        // reset
        sofar = ssmlHeader;        
        return;
      }

      // (3.1.3) Otherwise, try to convert the type into a string.
      bool conversionFailed = !ConvertValueToString(value, expr);
      VXIValueDestroy(&value);
  
      if (conversionFailed) {
        log->LogDiagnostic(0, L"PromptManager::ProcessSegments - "
                           L"audio src evaluation failed");
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                                             L"audio src evaluation failed");
      }

      if (expr.empty()) return;  // Ignore this element
    }

    // (3.2) Add an <audio> element to the current request.
    sofar += L"<audio";

    AddSSMLAttribute(elem, ATTRIBUTE_SRC, expr.c_str(), L"src", sofar);

    AddSSMLAttribute(elem, ATTRIBUTE_FETCHTIMEOUT,
                     propertyList.GetProperty(L"fetchtimeout"),
                     L"fetchtimeout", sofar);

    AddSSMLAttribute(elem, ATTRIBUTE_FETCHHINT,
                     propertyList.GetProperty(L"audiofetchhint"),
                     L"fetchhint", sofar);

    AddSSMLAttribute(elem, ATTRIBUTE_MAXAGE,
                     propertyList.GetProperty(L"audiomaxage"),
                     L"maxage", sofar);

    AddSSMLAttribute(elem, ATTRIBUTE_MAXSTALE,
                     propertyList.GetProperty(L"audiomaxstale"),
                     L"maxstale", sofar);

    // Add in the alternate text (if any).
    if (elem.hasChildren()) {
      sofar += L">";
      for (VXMLNodeIterator it(elem); it; ++it)
        ProcessSegments(*it, item, propertyList, translator,
                        bargein, props, sofar, ssmlHeader, hasPromptData, replaceXmlBase);
      sofar += L"</audio>";
    }
    else
      sofar += L"/>";

    return;
  }

  // (4) <value>
  case NODE_VALUE:
  {
    // (4.1) Evaluate the expression.  Can we handle this type?
    vxistring expr;
    if (!elem.GetAttribute(ATTRIBUTE_EXPR, expr) || expr.empty()) return;
    VXIValue * value = translator.EvaluateExpression(expr);
    if (value == NULL) return;

    // (4.2) Convert to a string a play as TTS.
    bool conversionFailed = !ConvertValueToString(value, expr);
    VXIValueDestroy(&value);

    if (conversionFailed) {
      log->LogDiagnostic(0, L"PromptManager::ProcessSegments - "
                         L"value expr was not a simple type");
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                                           L"value expr gave invalid type");
    }

    if (expr.empty()) return;
    AppendTextSegment(sofar, expr);
    return;
  }
  break;

  // (5) <enumerate>
  case NODE_ENUMERATE:
  {
    // (5.1) Is enumerate valid in this location?
    if (item == 0)
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC, L"invalid use "
                                           L"of enumerate element");

    bool foundMatch = false;

    for (VXMLNodeIterator it(item); it; ++it) {
      // (5.2) Ignore anything which isn't an <option> or <choice> grammar.
      if ((*it).GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLNode & tmp = *it;
      const VXMLElement & itemElem = reinterpret_cast<const VXMLElement&>(tmp);
      switch (itemElem.GetName()) {
      case NODE_CHOICE:
      case NODE_OPTION:
        foundMatch = true;
        break;
      default:
        continue;
      }

      // (5.3) Convert item into text.
      vxistring promptText;
      for (VXMLNodeIterator choiceNode(itemElem); choiceNode; ++choiceNode) {
        if ((*choiceNode).GetType() == VXMLNode::Type_VXMLContent) {

          if (!promptText.empty()) promptText += SPACE;
          const VXMLNode temp = *choiceNode;
  
          promptText += reinterpret_cast<const VXMLContent &>(temp).GetValue();
          /*
          // Get text from this node.
          vxistring text = reinterpret_cast<const VXMLContent &>
                             (*choiceNode).GetValue();

          if (!promptText.empty()) promptText += SPACE;

          // Strip the leading whitespace.
          while (text.length() > 0) {
            VXIchar c = text[0];
            if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
              text.erase(0, 1);
            else {
              promptText += text;
              break;
            }
          }

          // Strip off any trailing whitespace
          while (promptText.length() > 0) {
            unsigned int len = promptText.length();
            VXIchar c = promptText[len - 1];
            if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
              promptText.erase(len - 1, 1);
            else break;
          }
          */
        }
      }

      // (5.4) Handle the simple case where enumerate does not specify a
      // format string.
      if (!elem.hasChildren()) {
        AppendTextSegment(sofar, promptText);
        sofar += L"<break time='small'/>";
      }
      else {
        // (5.5) Get the associated dtmf value.
        vxistring dtmfText;
        itemElem.GetAttribute(ATTRIBUTE_DTMF, dtmfText);

        translator.SetVariable(L"_prompt", promptText);
        translator.SetVariable(L"_dtmf",   dtmfText);

        for (VXMLNodeIterator subNode(elem); subNode; ++subNode)
          ProcessSegments(*subNode, item, propertyList, translator,
                          bargein, props, sofar, ssmlHeader, hasPromptData, replaceXmlBase);
      }
    }

    // (5.6) Final check - was there an <option> or <choice>?
    if (!foundMatch)
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC, L"invalid use "
                                           L"of enumerate element");
    break;
  }

  // (6) <foreach>
  case NODE_FOREACH:
  {
    log->LogDiagnostic(2, L"PromptManager::ProcessSegments(), Handle foreach");

    // (6.1) Get array attribute, and verify that it's an Array
    vxistring array;
    elem.GetAttribute(ATTRIBUTE_ARRAY, array);

    vxistring test = array + L" instanceof Array";
    if (!translator.TestCondition(test)) {
      vxistring msg = array + L" is not an array";
      throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC, msg);
    }

    // (6.2) Get item
    vxistring itemName;
    elem.GetAttribute(ATTRIBUTE_ITEM, itemName);
    translator.EvalScript( L"var " + itemName + L" = null;" );

    // (6.3) Iterate array
    VXIValue *vlen = translator.EvaluateExpression( array + L".length" );
    VXIunsigned len = VXIIntegerValue(reinterpret_cast<VXIInteger *>(vlen));
    for (int i = 0; i < len; i++) {
      // (6.3.1) Set the item with the current array element
      std::basic_stringstream<VXIchar> script;
      script << itemName << L" = " << array << L"[" << i << L"];";
	  translator.EvalScript(script.str());

      // (6.3.2) Process child elements of the <foreach>
      for (VXMLNodeIterator it(elem); it; ++it)
        ProcessSegments(*it, item, propertyList, translator,
                        bargein, props, sofar, ssmlHeader, hasPromptData, replaceXmlBase);
    }
    break;
  }

  // (7) <mark>
  case NODE_MARK:
  {
    // (7.1) expand "nameexpr" if needed
    vxistring name;
    elem.GetAttribute(ATTRIBUTE_NAME, name);
    if (name.empty()) {
      elem.GetAttribute(ATTRIBUTE_NAMEEXPR, name);
      if(name.empty()) return;

      VXIValue * value = translator.EvaluateExpression(name);
      if (value == NULL) return;

      if(!ConvertValueToString(value, name)) name.clear();
      VXIValueDestroy(&value);
    }

    if(!name.empty())
      sofar += L"<mark name='" + name + L"'/>";
    break;
  }

  // Executable content
  default:
    contentHandler->executable_element(elem);
  }
}

void PromptManager::AddContent(VXIMapHolder & props,
                               vxistring & sofar,
                               VXIValue * value)
{
  VXIMapHolder allRefs(NULL);

  // (1) Get existing map (if it exists) or create a new one.
  const VXIValue * temp = VXIMapGetProperty(props.GetValue(),
                                            PROMPT_AUDIO_REFS);
  if (temp != NULL && VXIValueGetType(temp) == VALUE_MAP)
    allRefs.Acquire(VXIMapClone(reinterpret_cast<const VXIMap *>(temp)));
  else
    allRefs.Acquire(VXIMapCreate());
  if (allRefs.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (2) Generate a hidden name.
  vxistring hiddenName;
  VXMLDocumentModel::CreateHiddenVariable(hiddenName);
  hiddenName.erase(0, 1);
  hiddenName = vxistring(PROMPT_AUDIO_REFS_SCHEME) + hiddenName;

  // (3) Place hidden name and content into the map.
  VXIMapSetProperty(allRefs.GetValue(), hiddenName.c_str(), value);

  // (4) Replace the references map with the updated one.
  VXIMapSetProperty(props.GetValue(), PROMPT_AUDIO_REFS,
                    reinterpret_cast<VXIValue *>(allRefs.Release()));
  
  // (5) Handle the SSML case
  sofar += L"<mark name=\"";
  sofar += hiddenName;
  sofar += L"\"/>";
}

bool PromptManager::ProcessPrefetchPrompts(const VXMLNode & node, const VXMLElement& item,
                      const PropertyList & propertyList, 
                      PromptTranslator & translator, 
                      vxistring & sofar)
{
  bool rc = false;
   
  switch (node.GetType()) {
    case VXMLNode::Type_VXMLContent:
    {
      const VXMLContent & content = reinterpret_cast<const VXMLContent &>(node);
      sofar += content.GetValue();
      return true;
    }
    case VXMLNode::Type_VXMLElement:
      break;
    default:
      return false;
  }
        
  // Look at other type
  const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(node);

  switch (elem.GetName()) 
  {
    // audio
    case NODE_AUDIO:
    {
      // ignore this entire prompt if expr is found
      vxistring expr;
      if (elem.GetAttribute(ATTRIBUTE_EXPR, expr) == true)
        return false;
      else {
        rc = true;
        sofar += L"<audio";
        AddSSMLAttribute(elem, ATTRIBUTE_SRC, expr.c_str(), L"src", sofar);
        
        AddSSMLAttribute(elem, ATTRIBUTE_FETCHTIMEOUT,
                     propertyList.GetProperty(L"fetchtimeout"),
                     L"fetchtimeout", sofar);

        AddSSMLAttribute(elem, ATTRIBUTE_FETCHHINT,
                     propertyList.GetProperty(L"audiofetchhint"),
                     L"fetchhint", sofar);

        AddSSMLAttribute(elem, ATTRIBUTE_MAXAGE,
                     propertyList.GetProperty(L"audiomaxage"),
                     L"maxage", sofar);

        AddSSMLAttribute(elem, ATTRIBUTE_MAXSTALE,
                     propertyList.GetProperty(L"audiomaxstale"),
                     L"maxstale", sofar);
                         
        // Add in the alternate text (if any).
        if (elem.hasChildren()) {
          sofar += L">";
          for (VXMLNodeIterator it(elem); rc && it; ++it)
            rc = ProcessPrefetchPrompts(*it, item, propertyList, translator, sofar);         
          sofar += L"</audio>";
        }
        else
          sofar += L"/>";      
        
        if( rc ) sofar += L"</speak>";
      }
    
    } break;    
    
    // value tag and other tags found inside a prompt, do not fetch this entire prompt
    // case NODE_VALUE: case NODE_ENUMERATE:
    default:
      return false;
  }
  return rc;
}

void PromptManager::PreloadPrompts(const VXMLElement& doc,
                                   PropertyList & properties, 
                                   PromptTranslator & translator)
{
  VXIMapHolder temp(NULL);
  DoPreloadPrompts(doc, properties, temp, translator);
}

void PromptManager::DoPreloadPrompts(const VXMLElement& doc,
                                     PropertyList & properties,
                                     VXIMapHolder & levelProperties,
                                     PromptTranslator & translator)
{
  // (1) Look for prompts in current nodes.

  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & element = reinterpret_cast<const VXMLElement &>(child);
    VXMLElementType elementName = element.GetName();

    // (2) Handle anything which is not a prompt.

    if (elementName != NODE_PROMPT) { 
      bool doPop = properties.PushProperties(element);

      if (doPop) {
        VXIMapHolder temp(NULL);
        DoPreloadPrompts(element, properties, temp, translator);
      }
      else
        DoPreloadPrompts(element, properties, levelProperties, translator);

      if (doPop) properties.PopProperties();

      continue;
    }

    // (3) Handle <prompt>.  

    // (3.1) Ignore anything other than simple content.

    if (!element.hasChildren()) continue;

    // (3.2) Find the fetchhint setting.
    if (levelProperties.GetValue() == NULL) {
      levelProperties.Acquire(VXIMapCreate());
      properties.GetProperties(levelProperties);
    }

    vxistring attr = L"";
    if (element.GetAttribute(ATTRIBUTE_FETCHHINT, attr) != true) {
      const VXIValue * v = VXIMapGetProperty(levelProperties.GetValue(),
                                             L"audiofetchhint");
      if (VXIValueGetType(v) == VALUE_STRING)
        attr = VXIStringCStr(reinterpret_cast<const VXIString *>(v));
    }

    // (3.3) Get the level properties if necessary & add the prefetch setting.
    if (attr == L"prefetch")
      AddParamValue(levelProperties, PROMPT_PREFETCH_REQUEST, 1);
    else
      AddParamValue(levelProperties, PROMPT_PREFETCH_REQUEST, 0);

    // (3.4) Create full ssml document
    vxistring ssmldoc;    
    bool shouldPrefetch = true;
    
    for (VXMLNodeIterator it2(element); shouldPrefetch && it2; ++it2)
       shouldPrefetch = ProcessPrefetchPrompts(*it2, element, properties, translator, ssmldoc);     

     // (3.5) Call prefetch for prompt.
    if( shouldPrefetch ) {
      prompt->Prefetch(prompt, VXI_MIME_SSML, NULL,
                     ssmldoc.c_str(),
                     levelProperties.GetValue());
    }
    
    // (3.6) We might undo the addition of PROMPT_PREFETCH_REQUEST, but this
    //       parameter is always set and will be overwritten on the next pass.
  }
}

