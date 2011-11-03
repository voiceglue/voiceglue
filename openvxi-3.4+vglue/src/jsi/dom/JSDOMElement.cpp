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

#include "JSDOMElement.hpp"
#include <xercesc/dom/DOMException.hpp>

// JavaScript class definition
JSClass JSDOMElement::_jsClass = {
	"Element", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSDOMElement::JSGetProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSDOMElement::JSDestructor,
	0, 0, 0, 0, 
	0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMElement::JSInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);
	jsval oldobj;
	if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMElement::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
		return JSVAL_TO_OBJECT(oldobj);
	JSObject *newobj =
	    JS_InitClass
	    (cx, obj, NULL, &JSDOMElement::_jsClass,
	     NULL, 0,
	     JSDOMElement::_JSPropertySpec, JSDOMElement::_JSFunctionSpec,
	     NULL, NULL);

	if (newobj)
	{
	    InitNodeObject(cx, newobj);
	}
	else
	{
	};

	return newobj;
}

// JavaScript Destructor
void JSDOMElement::JSDestructor(JSContext *cx, JSObject *obj) {
	JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
	if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMElement::getJSObject(JSContext *cx) {
	if (!cx) return NULL;
	if (!_JSinternal.o) {
		_JSinternal.o = newJSObject(cx);
		_JSinternal.c = cx;
		if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
	}
	return _JSinternal.o;
}

JSObject *JSDOMElement::newJSObject(JSContext *cx) {
	return JS_NewObject(cx, &JSDOMElement::_jsClass, /*JSDOMNode::newJSObject(cx)*/ NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMElement::_JSPropertySpec[] = {
	{ "tagName", JSDOMElement::JSVAR_tagName, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMElement::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
   try {
      if (JSVAL_IS_INT(id)) {

         if (JSVAL_TO_INT(id) < JSDOMNode::JSVAR_LASTENUM )
            return JSDOMNode::JSGetProperty(cx, obj, id, vp);

         JSDOMElement *priv = (JSDOMElement*)JS_GetPrivate(cx, obj);

         switch(JSVAL_TO_INT(id)) {
         case JSVAR_tagName:
            if (priv==NULL)
               *vp = JSVAL_NULL;
            else
               *vp = __STRING_TO_JSVAL(priv->getTagName());
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
JSFunctionSpec JSDOMElement::_JSFunctionSpec[] = {
	{ "getAttribute", JSFUNC_getAttribute, 1, 0, 0 },
	{ "getAttributeNS", JSFUNC_getAttributeNS, 2, 0, 0 },
	{ "getAttributeNode", JSFUNC_getAttributeNode, 1, 0, 0 },
	{ "getAttributeNodeNS", JSFUNC_getAttributeNodeNS, 2, 0, 0 },
	{ "getElementsByTagName", JSFUNC_getElementsByTagName, 1, 0, 0 },
	{ "getElementsByTagNameNS", JSFUNC_getElementsByTagNameNS, 2, 0, 0 },
	{ "hasAttribute", JSFUNC_hasAttribute, 1, 0, 0 },
	{ "hasAttributeNS", JSFUNC_hasAttributeNS, 2, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMElement::JSFUNC_getAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 1) return JS_FALSE;
      if (argc == 1) {
         if (JSVAL_IS_STRING(argv[0])) {
            *rval = __STRING_TO_JSVAL(p->getAttribute(JSVAL_TO_STRING(argv[0])));
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_getAttributeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 2) return JS_FALSE;
      if (argc == 2) {
         if (JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) {
            *rval = __STRING_TO_JSVAL(p->getAttributeNS(
            JSVAL_TO_STRING(argv[0]),
            JSVAL_TO_STRING(argv[1])
            ));
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_getAttributeNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 1) return JS_FALSE;
      if (argc == 1) {
         if (JSVAL_IS_STRING(argv[0])) {
            JSDOMAttr *attr = p->getAttributeNode(JSVAL_TO_STRING(argv[0]));
            *rval = (attr ? __objectp_TO_JSVal(cx,attr) : JSVAL_NULL);
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_getAttributeNodeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 2) return JS_FALSE;
      if (argc == 2) {
         if (JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) {
            JSDOMAttr *attr = p->getAttributeNodeNS(
               JSVAL_TO_STRING(argv[0]),
               JSVAL_TO_STRING(argv[1]));
            *rval = (attr ? __objectp_TO_JSVal(cx,attr) : JSVAL_NULL);
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_getElementsByTagName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 1) return JS_FALSE;
      if (argc == 1) {
         if (JSVAL_IS_STRING(argv[0])) {
            JSDOMNodeList *nl = p->getElementsByTagName(JSVAL_TO_STRING(argv[0]));
            *rval = (nl ? __objectp_TO_JSVal(cx,nl) : JSVAL_NULL);
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_getElementsByTagNameNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
   JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 2) return JS_FALSE;
      if (argc == 2) {
         if (JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) {

            JSDOMNodeList *nl = p->getElementsByTagNameNS(
               JSVAL_TO_STRING(argv[0]),
               JSVAL_TO_STRING(argv[1]));

            *rval = (nl ? __objectp_TO_JSVal(cx,nl) : JSVAL_NULL);
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_hasAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 1) return JS_FALSE;
      if (argc == 1) {
         if (JSVAL_IS_STRING(argv[0])) {
            *rval = __bool_TO_JSVal(cx,p->hasAttribute(JSVAL_TO_STRING(argv[0])));
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}

JSBool JSDOMElement::JSFUNC_hasAttributeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMElement *p = (JSDOMElement*)JS_GetPrivate(cx, obj);
      if (argc < 2) return JS_FALSE;
      if (argc == 2) {
         if (JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) {
            *rval = __bool_TO_JSVal(cx,p->hasAttributeNS(
               JSVAL_TO_STRING(argv[0]),
               JSVAL_TO_STRING(argv[1]) ));
            return JS_TRUE;
         }
      }
   }
   catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
   }
   catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
   }
   return JS_FALSE;
}




JSDOMElement::JSDOMElement( DOMElement *elem, JSDOMDocument *doc ) : JSDOMNode( elem, doc )
{
	_elem = elem;
} 

JSString* JSDOMElement::getTagName()
{
	if (!_elem) return NULL;
	XMLChToVXIchar name(_elem->getTagName());
	GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, name.c_str());
	return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSString* JSDOMElement::getAttribute(JSString* name)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(name));
	VXIcharToXMLCh xmlname(nameval);

	XMLChToVXIchar result(_elem->getAttribute(xmlname.c_str()));
	GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, result.c_str());
	return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSDOMAttr* JSDOMElement::getAttributeNode(JSString* name)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(name));
	VXIcharToXMLCh xmlname(nameval);

	DOMAttr *attr = _elem->getAttributeNode(xmlname.c_str());
	return attr ? new JSDOMAttr(attr, getOwnerDocument(), this) : NULL;
}

JSDOMNodeList* JSDOMElement::getElementsByTagName(JSString* name)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(name));
	VXIcharToXMLCh xmlname(nameval);

	DOMNodeList *nl = _elem->getElementsByTagName(xmlname.c_str());
	return new JSDOMNodeList(nl, getOwnerDocument());
}

JSString* JSDOMElement::getAttributeNS(JSString* namespaceURI, JSString* localName)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
	VXIcharToXMLCh xmlns(nsval);

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
	VXIcharToXMLCh xmlname(nameval);

	XMLChToVXIchar result(_elem->getAttributeNS(xmlns.c_str(), xmlname.c_str()));
	GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, result.c_str());
	return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSDOMAttr* JSDOMElement::getAttributeNodeNS(JSString* namespaceURI, JSString* localName)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
	VXIcharToXMLCh xmlns(nsval);

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
	VXIcharToXMLCh xmlname(nameval);

	DOMAttr *attr = _elem->getAttributeNodeNS(xmlns.c_str(), xmlname.c_str());
	return attr ? new JSDOMAttr(attr, getOwnerDocument(), this) : NULL;
}

JSDOMNodeList* JSDOMElement::getElementsByTagNameNS(JSString* namespaceURI, JSString* localName)
{
	if (!_elem) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
	VXIcharToXMLCh xmlns(nsval);

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
	VXIcharToXMLCh xmlname(nameval);

	DOMNodeList *nl = _elem->getElementsByTagNameNS(xmlns.c_str(), xmlname.c_str());
	return new JSDOMNodeList(nl, getOwnerDocument());
}

bool JSDOMElement::hasAttribute(JSString* name)
{
	if (!_elem) return false;

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(name));
	VXIcharToXMLCh xmlname(nameval);

	return _elem->hasAttribute(xmlname.c_str());
}

bool JSDOMElement::hasAttributeNS(JSString* namespaceURI, JSString* localName)
{
	if (!_elem) return false;;

	GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
	VXIcharToXMLCh xmlns(nsval);

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
	VXIcharToXMLCh xmlname(nameval);

	return _elem->hasAttributeNS(xmlns.c_str(), xmlname.c_str());
}
