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

#include "JSDOMException.hpp"
#include "JSDOM.hpp"

// JavaScript class definition
JSClass JSDOMException::_jsClass = {
  "DOMException", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JSDOMException::JSGetProperty, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, JSDOMException::JSDestructor,
  0, 0, 0, 0, 
  0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMException::JSInit(JSContext *cx, JSObject *obj) {
  if (obj==NULL)
    obj = JS_GetGlobalObject(cx);
  jsval oldobj;
  if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMException::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
    return JSVAL_TO_OBJECT(oldobj);

  return JS_InitClass(cx, obj, NULL, &JSDOMException::_jsClass,
                      JSDOMException::JSConstructor, 0,
                      JSDOMException::_JSPropertySpec, NULL,
                      JSDOMException::_JSPropertySpec_static, NULL);
}

// JavaScript Constructor
JSBool JSDOMException::JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  JSDOMException *p = NULL;
  if (argc == 0) {
    p = new JSDOMException(0);
  }
  else {
    int code = JSVAL_TO_INT(*argv);
	p = new JSDOMException(code);
  }

  if (!p || !JS_SetPrivate(cx, obj, p)) return JS_FALSE;
  p->_JSinternal.o = obj;
  p->_JSinternal.c = cx;
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}

// JavaScript Destructor
void JSDOMException::JSDestructor(JSContext *cx, JSObject *obj) {
  JSDOMException *p = (JSDOMException*)JS_GetPrivate(cx, obj);
  if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMException::getJSObject(JSContext *cx) {
  if (!cx) return NULL;
  if (!_JSinternal.o) {
    _JSinternal.o = newJSObject(cx);
    _JSinternal.c = cx;
    if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
  }
  return _JSinternal.o;
}

JSObject *JSDOMException::newJSObject(JSContext *cx) {
  return JS_NewObject(cx, &JSDOMException::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMException::_JSPropertySpec[] = {
    { "INDEX_SIZE_ERR", JSDOMException::JSVAR_INDEX_SIZE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "DOMSTRING_SIZE_ERR", JSDOMException::JSVAR_DOMSTRING_SIZE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NO_MODIFICATION_ALLOWED_ERR", JSDOMException::JSVAR_NO_MODIFICATION_ALLOWED_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NOT_FOUND_ERR", JSDOMException::JSVAR_NOT_FOUND_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NOT_SUPPORTED_ERR", JSDOMException::JSVAR_NOT_SUPPORTED_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "INVALID_STATE_ERR", JSDOMException::JSVAR_INVALID_STATE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "code", JSDOMException::JSVAR_code, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
};

JSPropertySpec JSDOMException::_JSPropertySpec_static[] = {
    { "INDEX_SIZE_ERR", JSDOMException::JSVAR_INDEX_SIZE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "DOMSTRING_SIZE_ERR", JSDOMException::JSVAR_DOMSTRING_SIZE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NO_MODIFICATION_ALLOWED_ERR", JSDOMException::JSVAR_NO_MODIFICATION_ALLOWED_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NOT_FOUND_ERR", JSDOMException::JSVAR_NOT_FOUND_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "NOT_SUPPORTED_ERR", JSDOMException::JSVAR_NOT_SUPPORTED_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { "INVALID_STATE_ERR", JSDOMException::JSVAR_INVALID_STATE_ERR, JSPROP_ENUMERATE | JSPROP_READONLY, JSDOMException::JSGetProperty, 0},
    { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMException::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
   if (JSVAL_IS_INT(id)) {
   
   JSDOMException *priv = (JSDOMException*)JS_GetPrivate(cx, obj);
      if (priv==NULL)
         *vp = JSVAL_NULL;

      switch(JSVAL_TO_INT(id)) {
      case JSVAR_INDEX_SIZE_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::INDEX_SIZE_ERR);
         break;

      case JSVAR_DOMSTRING_SIZE_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::DOMSTRING_SIZE_ERR);
         break;

      case JSVAR_NO_MODIFICATION_ALLOWED_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::NO_MODIFICATION_ALLOWED_ERR);
         break;

      case JSVAR_NOT_FOUND_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::NOT_FOUND_ERR);
         break;

      case JSVAR_NOT_SUPPORTED_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::NOT_SUPPORTED_ERR);
         break;

      case JSVAR_INVALID_STATE_ERR:
         *vp = __int_TO_JSVal(cx,JSDOMException::INVALID_STATE_ERR);
         break;

      case JSVAR_code:
         if (priv)
            *vp = INT_TO_JSVAL(priv->getCode());
         break;
      }
   }

   return JS_TRUE;
}

JSDOMException::JSDOMException(int code) : _code(code)
{
}

int JSDOMException::getCode()
{
  return _code;
}
