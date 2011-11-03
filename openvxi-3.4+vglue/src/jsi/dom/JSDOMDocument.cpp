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

#include "JSDOMDocument.hpp"
#include "JSDOMNodeList.hpp"
#include "JSDOMElement.hpp"
#include <xercesc/dom/DOMException.hpp>

// JavaScript class definition
JSClass JSDOMDocument::_jsClass = {
   "Document", JSCLASS_HAS_PRIVATE,
   JS_PropertyStub, JS_PropertyStub,
   JSDOMDocument::JSGetProperty, JS_PropertyStub,
   JS_EnumerateStub, JS_ResolveStub,
   JS_ConvertStub, JSDOMDocument::JSDestructor,
   0, 0, 0, 0, 
   0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMDocument::JSInit(JSContext *cx, JSObject *obj)
{
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG, "JSDOMDocument::JSInit() started");

   if (obj==NULL)
      obj = JS_GetGlobalObject(cx);

   jsval oldobj;
   if (JS_TRUE == JS_LookupProperty
       (cx, obj, JSDOMDocument::_jsClass.name, &oldobj) &&
       JSVAL_IS_OBJECT(oldobj))
   {
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG,
		       "Returning previous JSDOMDocument object");
      return JSVAL_TO_OBJECT(oldobj);
   };

   JSObject *newobj =
       JS_InitClass
       (cx, obj, NULL, &JSDOMDocument::_jsClass,
	NULL, 0,
	JSDOMDocument::_JSPropertySpec,
	JSDOMDocument::_JSFunctionSpec,
	NULL, NULL);

   if (newobj)
   {
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG,
		       "JS_InitClass(JSDOMDocument) OK, do: InitNodeObject()");
      InitNodeObject(cx, newobj);
   }
   else
   {
       if (voiceglue_loglevel() >= LOG_ERR)
	   voiceglue_log ((char) LOG_ERR,
			  "JS_InitClass(JSDOMDocument) failed");
   };

   return newobj;
}

// JavaScript Destructor
void JSDOMDocument::JSDestructor(JSContext *cx, JSObject *obj) {
   JSDOMDocument *p = (JSDOMDocument*)JS_GetPrivate(cx, obj);
   if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMDocument::getJSObject(JSContext *cx) {
   if (!cx) return NULL;
   if (!_JSinternal.o) {
      _JSinternal.o = newJSObject(cx);
      _JSinternal.c = cx;
      if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
   }
   return _JSinternal.o;
}

JSObject *JSDOMDocument::newJSObject(JSContext *cx) {
   return JS_NewObject(cx, &JSDOMDocument::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMDocument::_JSPropertySpec[] = {
   { "documentElement", JSDOMDocument::JSVAR_documentElement, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
   { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMDocument::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
  if (JSVAL_IS_INT(id)) {
    if (JSVAL_TO_INT(id) < JSDOMNode::JSVAR_LASTENUM )
      return JSDOMNode::JSGetProperty(cx, obj, id, vp );

   try {
      JSDOMDocument *priv;
      switch(JSVAL_TO_INT(id)) {
      case JSVAR_documentElement:
        priv = (JSDOMDocument*)JS_GetPrivate(cx, obj);
        if (priv==NULL)
          *vp = JSVAL_NULL;
        else {
          JSDOMElement *elem = priv->getDocumentElement();
          if (elem == NULL)
            *vp = JSVAL_NULL;
          else
            *vp = __objectp_TO_JSVal(cx,elem);
        }
        break;
      }
    }
    catch(const DOMException &dome) {
      return js_throw_dom_error(cx, obj, dome.code);
    }
    catch( ... ) {
      return js_throw_error(cx, obj, "unknown exception" );
    }
  }
  return JS_TRUE;
}

// JavaScript Function Table
JSFunctionSpec JSDOMDocument::_JSFunctionSpec[] = {
   { "getElementById", JSFUNC_getElementById, 1, 0, 0 },
   { "getElementsByTagName", JSFUNC_getElementsByTagName, 1, 0, 0 },
   { "getElementsByTagNameNS", JSFUNC_getElementsByTagNameNS, 2, 0, 0 },
   { 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMDocument::JSFUNC_getElementById(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMDocument *p = (JSDOMDocument*)JS_GetPrivate(cx, obj);
      if (argc < 1) return JS_FALSE;
      if (argc == 1) {
         if (JSVAL_IS_STRING(argv[0])) {
            JSDOMElement *e = p->getElementById(JSVAL_TO_STRING(argv[0]));
            *rval = (e ? __objectp_TO_JSVal(cx,e) : JSVAL_NULL);
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

JSBool JSDOMDocument::JSFUNC_getElementsByTagName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMDocument *p = (JSDOMDocument*)JS_GetPrivate(cx, obj);
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

JSBool JSDOMDocument::JSFUNC_getElementsByTagNameNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
   try {
      JSDOMDocument *p = (JSDOMDocument*)JS_GetPrivate(cx, obj);
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


JSDOMDocument::JSDOMDocument(DOMDocument *doc) : JSDOMNode(doc, this), _doc(doc)
{
}

JSDOMElement* JSDOMDocument::getDocumentElement()
{
   if (!_doc)
      return NULL;
   DOMElement *elem = _doc->getDocumentElement();
   return elem ? new JSDOMElement(elem, this) : NULL;
}

JSDOMNodeList* JSDOMDocument::getElementsByTagName(JSString* tagname)
{
   if (!_doc) 
      return new JSDOMNodeList(NULL, this);

   GET_VXICHAR_FROM_JSCHAR(tmpval, JS_GetStringChars(tagname));
   VXIcharToXMLCh tag(tmpval);

   DOMNodeList *list = _doc->getElementsByTagName(tag.c_str());
   return new JSDOMNodeList(list, this);
}

JSDOMNodeList* JSDOMDocument::getElementsByTagNameNS(JSString *namespaceURI, JSString *localName)
{
   if (!_doc) 
      return new JSDOMNodeList(NULL, this);

   GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
   VXIcharToXMLCh xmlns(nsval);

   GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
   VXIcharToXMLCh xmlname(nameval);

   DOMNodeList *list = _doc->getElementsByTagNameNS(xmlns.c_str(), xmlname.c_str());
   return new JSDOMNodeList(list, this);
}

JSDOMElement* JSDOMDocument::getElementById(JSString *elementId)
{
   if (!_doc) 
      return NULL;

   GET_VXICHAR_FROM_JSCHAR(idval, JS_GetStringChars(elementId));
   VXIcharToXMLCh xmlid(idval);

   DOMElement *elem = _doc->getElementById(xmlid.c_str());
   return elem ? new JSDOMElement(elem) : NULL;
}
