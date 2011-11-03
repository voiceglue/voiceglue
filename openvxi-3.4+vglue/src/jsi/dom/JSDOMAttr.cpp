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

#include "JSDOMAttr.hpp"
#include "JSDOMElement.hpp"
#include <xercesc/dom/DOMException.hpp>

// JavaScript class definition
JSClass JSDOMAttr::_jsClass = {
    "Attr", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    JSDOMAttr::JSGetProperty, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub, JSDOMAttr::JSDestructor,
    0, 0, 0, 0, 
    0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMAttr::JSInit(JSContext *cx, JSObject *obj) {
    if (obj==NULL)
        obj = JS_GetGlobalObject(cx);
    jsval oldobj;
    if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMAttr::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
        return JSVAL_TO_OBJECT(oldobj);

    JSObject *newobj = JS_InitClass(cx, obj, NULL, &JSDOMAttr::_jsClass,
                                         NULL, 0,
                                         JSDOMAttr::_JSPropertySpec, NULL,
                                         NULL, NULL);
    if (newobj) {
        InitNodeObject(cx, newobj);
    }

    return newobj;
}

// JavaScript Destructor
void JSDOMAttr::JSDestructor(JSContext *cx, JSObject *obj) {
    JSDOMAttr *p = (JSDOMAttr*)JS_GetPrivate(cx, obj);
    if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMAttr::getJSObject(JSContext *cx) {
    if (!cx) return NULL;
    if (!_JSinternal.o) {
        _JSinternal.o = newJSObject(cx);
        _JSinternal.c = cx;
        if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
    }
    return _JSinternal.o;
}

JSObject *JSDOMAttr::newJSObject(JSContext *cx) {
    return JS_NewObject(cx, &JSDOMAttr::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMAttr::_JSPropertySpec[] = {
    { "name", JSDOMAttr::JSVAR_name, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "specified", JSDOMAttr::JSVAR_specified, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "value", JSDOMAttr::JSVAR_value, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "ownerElement", JSDOMAttr::JSVAR_ownerElement, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMAttr::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
   if (JSVAL_IS_INT(id)) {

      if (JSVAL_TO_INT(id) < JSDOMNode::JSVAR_LASTENUM )
         return JSDOMNode::JSGetProperty(cx, obj, id, vp);

      try
      {
         JSDOMAttr *priv;
         switch(JSVAL_TO_INT(id)) {
         case JSVAR_name:
            priv = (JSDOMAttr*)JS_GetPrivate(cx, obj);
            if (priv==NULL)
               *vp = JSVAL_NULL;
            else
               *vp = __STRING_TO_JSVAL(priv->getName());
            break;

         case JSVAR_specified:
            priv = (JSDOMAttr*)JS_GetPrivate(cx, obj);
            if (priv==NULL)
               *vp = JSVAL_NULL;
            else
               *vp = __bool_TO_JSVal(cx,priv->getSpecified());
            break;

         case JSVAR_value:
            priv = (JSDOMAttr*)JS_GetPrivate(cx, obj);
            if (priv==NULL)
               *vp = JSVAL_NULL;
            else
               *vp = __STRING_TO_JSVAL(priv->getValue());
            break;

         case JSVAR_ownerElement:
            priv = (JSDOMAttr*)JS_GetPrivate(cx, obj);
            if (priv==NULL)
               *vp = JSVAL_NULL;
            else {
               JSDOMElement *e = priv->getOwnerElement();
               *vp = (e ? __objectp_TO_JSVal(cx,e) : JSVAL_NULL);
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


JSDOMAttr::JSDOMAttr( DOMAttr *attr, JSDOMDocument *doc, JSDOMElement *owner ) 
    : JSDOMNode(attr,doc),_attr(attr), _owner(owner)
{
}

JSString* JSDOMAttr::getName()
{
   if (_attr == NULL) return NULL;
   XMLChToVXIchar name(_attr->getName());
   GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, name.c_str());
   return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

bool JSDOMAttr::getSpecified()
{
   return (_attr ? _attr->getSpecified() : false);
}

JSString* JSDOMAttr::getValue()
{
   if (_attr == NULL) return NULL;
   XMLChToVXIchar value(_attr->getValue());
   GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, value.c_str());
   return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSDOMElement* JSDOMAttr::getOwnerElement()
{
   return _owner;
}
