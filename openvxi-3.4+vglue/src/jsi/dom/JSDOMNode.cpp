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

#include "JSDOMNode.hpp"
#include "JSDOMNodeList.hpp"
#include "JSDOMNamedNodeMap.hpp"
#include "JSDOMDocument.hpp"
#include "JSDOMElement.hpp"
#include "JSDOMAttr.hpp"
#include "JSDOMCharacterData.hpp"
#include "JSDOMText.hpp"
#include "JSDOMComment.hpp"
#include "JSDOMCDATA.hpp"
#include "JSDOMEntityReference.hpp"
#include "JSDOMProcessingInstruction.hpp"

#include <xercesc/dom/DOMException.hpp>


// JavaScript class definition
JSClass JSDOMNode::_jsClass = {
  "Node", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JSDOMNode::JSGetProperty, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, JSDOMNode::JSDestructor,
  0, 0, 0, 0, 
  0, 0, 0, 0
};

bool JSDOMNode::InitNodeObject(JSContext *cx, JSObject *obj)
{
  JS_DefineProperties( cx, obj, JSDOMNode::_JSPropertySpec );
  JS_DefineFunctions( cx, obj, JSDOMNode::_JSFunctionSpec );
  return true;
}

// JavaScript Initialization Method
JSObject *JSDOMNode::JSInit(JSContext *cx, JSObject *obj) {
  if (obj==NULL)
    obj = JS_GetGlobalObject(cx);
  jsval oldobj;
  if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMNode::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
    return JSVAL_TO_OBJECT(oldobj);

  return JS_InitClass(cx, obj, NULL, &JSDOMNode::_jsClass,
                      JSDOMNode::JSConstructor, 0,
                      JSDOMNode::_JSPropertySpec, JSDOMNode::_JSFunctionSpec,
                      JSDOMNode::_JSPropertySpec_static, NULL);
}

// JavaScript Constructor
JSBool JSDOMNode::JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  JSDOMNode *p = NULL;
  if (argc == 0) {
    p = new JSDOMNode;
  }

  if (!p || !JS_SetPrivate(cx, obj, p)) return JS_FALSE;
  p->_JSinternal.o = obj;
  p->_JSinternal.c = cx;
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}

// JavaScript Destructor
void JSDOMNode::JSDestructor(JSContext *cx, JSObject *obj) {
  JSDOMNode *p = (JSDOMNode*)JS_GetPrivate(cx, obj);
  if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMNode::getJSObject(JSContext *cx) {
  if (!cx) return NULL;
  if (!_JSinternal.o) {
    _JSinternal.o = newJSObject(cx);
    _JSinternal.c = cx;
    if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
  }
  return _JSinternal.o;
}

JSObject *JSDOMNode::newJSObject(JSContext *cx) {
  return JS_NewObject(cx, &JSDOMNode::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMNode::_JSPropertySpec[] = {
    { "ELEMENT_NODE", JSDOMNode::JSVAR_ELEMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ATTRIBUTE_NODE", JSDOMNode::JSVAR_ATTRIBUTE_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "TEXT_NODE", JSDOMNode::JSVAR_TEXT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "CDATA_SECTION_NODE", JSDOMNode::JSVAR_CDATA_SECTION_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ENTITY_REFERENCE_NODE", JSDOMNode::JSVAR_ENTITY_REFERENCE_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ENTITY_NODE", JSDOMNode::JSVAR_ENTITY_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "PROCESSING_INSTRUCTION_NODE", JSDOMNode::JSVAR_PROCESSING_INSTRUCTION_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "COMMENT_NODE", JSDOMNode::JSVAR_COMMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "DOCUMENT_NODE", JSDOMNode::JSVAR_DOCUMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "nodeName", JSDOMNode::JSVAR_nodeName, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "nodeValue", JSDOMNode::JSVAR_nodeValue, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "nodeType", JSDOMNode::JSVAR_nodeType, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "parentNode", JSDOMNode::JSVAR_parentNode, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "childNodes", JSDOMNode::JSVAR_childNodes, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "firstChild", JSDOMNode::JSVAR_firstChild, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "lastChild", JSDOMNode::JSVAR_lastChild, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "previousSibling", JSDOMNode::JSVAR_previousSibling, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "nextSibling", JSDOMNode::JSVAR_nextSibling, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "attributes", JSDOMNode::JSVAR_attributes, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "ownerDocument", JSDOMNode::JSVAR_ownerDocument, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "namespaceURI", JSDOMNode::JSVAR_namespaceURI, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "prefix", JSDOMNode::JSVAR_prefix, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "localName", JSDOMNode::JSVAR_localName, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { 0, 0, 0, 0, 0 }
};

JSPropertySpec JSDOMNode::_JSPropertySpec_static[] = {
    { "ELEMENT_NODE", JSDOMNode::JSVAR_ELEMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ATTRIBUTE_NODE", JSDOMNode::JSVAR_ATTRIBUTE_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "TEXT_NODE", JSDOMNode::JSVAR_TEXT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "CDATA_SECTION_NODE", JSDOMNode::JSVAR_CDATA_SECTION_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ENTITY_REFERENCE_NODE", JSDOMNode::JSVAR_ENTITY_REFERENCE_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "ENTITY_NODE", JSDOMNode::JSVAR_ENTITY_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "PROCESSING_INSTRUCTION_NODE", JSDOMNode::JSVAR_PROCESSING_INSTRUCTION_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "COMMENT_NODE", JSDOMNode::JSVAR_COMMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { "DOCUMENT_NODE", JSDOMNode::JSVAR_DOCUMENT_NODE, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMNode::JSGetProperty, 0},
    { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMNode::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
   try {
      if (JSVAL_IS_INT(id)) {
   
         JSDOMNode *priv = (JSDOMNode*)JS_GetPrivate(cx, obj);
         if (priv==NULL)
           *vp = JSVAL_NULL;

         switch(JSVAL_TO_INT(id)) {
         case JSVAR_ELEMENT_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::ELEMENT_NODE);
            break;

         case JSVAR_ATTRIBUTE_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::ATTRIBUTE_NODE);
            break;

         case JSVAR_TEXT_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::TEXT_NODE);
            break;

         case JSVAR_CDATA_SECTION_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::CDATA_SECTION_NODE);
            break;

         case JSVAR_ENTITY_REFERENCE_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::ENTITY_REFERENCE_NODE);
            break;

         case JSVAR_ENTITY_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::ENTITY_NODE);
            break;

         case JSVAR_PROCESSING_INSTRUCTION_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::PROCESSING_INSTRUCTION_NODE);
            break;

         case JSVAR_COMMENT_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::COMMENT_NODE);
            break;

         case JSVAR_DOCUMENT_NODE:
            *vp = __int_TO_JSVal(cx,JSDOMNode::DOCUMENT_NODE);
            break;

         case JSVAR_nodeName:
            if (priv)
               *vp = __STRING_TO_JSVAL(priv->getNodeName());
            break;

         case JSVAR_nodeValue:
            if (priv)
               *vp = __STRING_TO_JSVAL(priv->getNodeValue());
            break;

         case JSVAR_nodeType:
            if (priv)
               *vp = __int_TO_JSVal(cx,priv->getNodeType());
            break;

         case JSVAR_parentNode:
            if (priv) {
               JSDOMNode *n = priv->getParentNode();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_childNodes:
            if (priv) {
               JSDOMNodeList *nl = priv->getChildNodes();
               *vp = (nl ? __objectp_TO_JSVal(cx,nl) : JSVAL_NULL);
            }
            break;

         case JSVAR_firstChild:
            if (priv){
               JSDOMNode *n = priv->getFirstChild();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_lastChild:
            if (priv){
               JSDOMNode *n = priv->getLastChild();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_previousSibling:
            if (priv){
               JSDOMNode *n = priv->getPreviousSibling();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_nextSibling:
            if (priv){
               JSDOMNode *n = priv->getNextSibling();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_attributes:
            if (priv){
               JSDOMNamedNodeMap *nm = priv->getAttributes();
               *vp = (nm ? __objectp_TO_JSVal(cx,nm) : JSVAL_NULL);
            }
            break;

         case JSVAR_ownerDocument:
            if (priv){
               JSDOMNode *n = priv->getOwnerDocument();
               *vp = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
            }
            break;

         case JSVAR_namespaceURI:
            if (priv)
               *vp = __STRING_TO_JSVAL(priv->getNamespaceURI());
            break;

         case JSVAR_prefix:
            if (priv)
               *vp = __STRING_TO_JSVAL(priv->getPrefix());
            break;

         case JSVAR_localName:
            if (priv)
               *vp = __STRING_TO_JSVAL(priv->getLocalName());
            break;
         }
     }
  }
  catch(const DOMException &dome) {
    return js_throw_dom_error(cx, obj, dome.code);
  }
  catch( ... ) {
    return js_throw_error(cx, obj, "unknown exception" );
  }

  return JS_TRUE;
}

// JavaScript Function Table
JSFunctionSpec JSDOMNode::_JSFunctionSpec[] = {
    { "hasAttributes", JSFUNC_hasAttributes, 0, 0, 0 },
    { "hasChildNodes", JSFUNC_hasChildNodes, 0, 0, 0 },
    { 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMNode::JSFUNC_hasAttributes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  JSDOMNode *p = (JSDOMNode*)JS_GetPrivate(cx, obj);
  if (argc == 0) {
    *rval = __bool_TO_JSVal(cx,p->hasAttributes());
    return JS_TRUE;
  }
  return JS_FALSE;
}

JSBool JSDOMNode::JSFUNC_hasChildNodes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  JSDOMNode *p = (JSDOMNode*)JS_GetPrivate(cx, obj);
  if (argc == 0) {
    *rval = __bool_TO_JSVal(cx,p->hasChildNodes());
    return JS_TRUE;
  }
  return JS_FALSE;
}



JSDOMNode *JSDOMNode::createNode(DOMNode *node, JSDOMDocument *doc, JSDOMNode *ownerNode)
{
  if (!node) return NULL;

  JSDOMNode *jsnode = NULL;

  int type = node->getNodeType();
  switch( type )
  {
  case JSDOMNode::ELEMENT_NODE:
    jsnode = new JSDOMElement( reinterpret_cast<DOMElement *>(node), doc);
    break;
  case JSDOMNode::ATTRIBUTE_NODE:
    jsnode = new JSDOMAttr( reinterpret_cast<DOMAttr *>(node), doc, (JSDOMElement*) ownerNode );
    break;
  case JSDOMNode::TEXT_NODE:
    jsnode = new JSDOMText( reinterpret_cast<DOMText *>(node), doc );
    break;
  case JSDOMNode::CDATA_SECTION_NODE:
    jsnode = new JSDOMCDATA( reinterpret_cast<DOMCDATASection *>(node), doc );
    break;
  case JSDOMNode::ENTITY_REFERENCE_NODE:
    jsnode = new JSDOMEntityReference( reinterpret_cast<DOMEntityReference *>(node), doc );
    break;
  case JSDOMNode::PROCESSING_INSTRUCTION_NODE:
    jsnode = new JSDOMProcessingInstruction( reinterpret_cast<DOMProcessingInstruction *>(node), doc );
    break;
  case JSDOMNode::COMMENT_NODE:
    jsnode = new JSDOMComment( reinterpret_cast<DOMComment *>(node), doc );
    break;
  case JSDOMNode::DOCUMENT_NODE:
    jsnode = new JSDOMDocument( reinterpret_cast<DOMDocument *>(node));
    break;

  // !!! not it the VXML spec
  //case JSDOMNode::ENTITY_NODE:  
  //case DOCUMENT_TYPE_NODE:
  //case DOCUMENT_FRAGMENT_NODE:
  //case NOTATION_NODE:
  default:
    jsnode = new JSDOMNode( node, doc ); 
    break;
  };

  return jsnode;
}



JSDOMNode::JSDOMNode( DOMNode *node, JSDOMDocument *owner ) : _node(node), _owner(owner)
{
}

JSString* JSDOMNode::getNodeName()
{
  if (_node == NULL) return NULL;

  XMLChToVXIchar name(_node->getNodeName());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, name.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSString* JSDOMNode::getNodeValue()
{
  if (_node == NULL) return NULL;

  XMLChToVXIchar value(_node->getNodeValue());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, value.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

int JSDOMNode::getNodeType()
{
  return ( _node ? _node->getNodeType() : 0 );
}

JSDOMNode* JSDOMNode::getParentNode()
{
  return JSDOMNode::createNode(_node->getParentNode(),_owner);
}

JSDOMNodeList* JSDOMNode::getChildNodes()
{
  return new JSDOMNodeList((_node ? _node->getChildNodes() : NULL), _owner);
}

JSDOMNode* JSDOMNode::getFirstChild()
{
  return JSDOMNode::createNode(_node->getFirstChild(),_owner);
}

JSDOMNode* JSDOMNode::getLastChild()
{
  return JSDOMNode::createNode(_node->getLastChild(),_owner);
}

JSDOMNode* JSDOMNode::getPreviousSibling()
{
  return JSDOMNode::createNode(_node->getPreviousSibling(),_owner);
}

JSDOMNode* JSDOMNode::getNextSibling()
{
  return JSDOMNode::createNode(_node->getNextSibling(),_owner);
}

JSDOMNamedNodeMap* JSDOMNode::getAttributes()
{
  if (!_node) return NULL;
  DOMNamedNodeMap *attrs = _node->getAttributes();
  if (!attrs) return NULL;
  return new JSDOMNamedNodeMap(attrs, this);
}

JSDOMDocument* JSDOMNode::getOwnerDocument()
{
  /*DOMDocument *owner = _node->getOwnerDocument();
  if (!owner) return NULL;
  return new JSDOMDocument(_node->getOwnerDocument());*/
  return _owner;
}

JSString* JSDOMNode::getNamespaceURI()
{
  if (_node == NULL) return NULL;
  const XMLCh *p = _node->getNamespaceURI();
  if (!p) return NULL;
  XMLChToVXIchar ns(_node->getNamespaceURI());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, ns.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSString* JSDOMNode::getPrefix()
{
  if (_node == NULL) return NULL;
  const XMLCh *p = _node->getPrefix();
  if (!p) return NULL;
  XMLChToVXIchar prefix(p);
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, prefix.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSString* JSDOMNode::getLocalName()
{
  if (_node == NULL) return NULL;
  XMLChToVXIchar name(_node->getLocalName());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, name.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

bool JSDOMNode::hasChildNodes()
{
  return ( _node ? _node->hasChildNodes() : false );
}
 
bool JSDOMNode::hasAttributes()
{
  return ( _node ? _node->hasAttributes() : false );
}
