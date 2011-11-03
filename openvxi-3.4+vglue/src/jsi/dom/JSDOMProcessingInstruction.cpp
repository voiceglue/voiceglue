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

#include "JSDOMProcessingInstruction.hpp"
#include <xercesc/dom/DOMException.hpp>

// JavaScript class definition
JSClass JSDOMProcessingInstruction::_jsClass = {
  "ProcessingInstruction", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JSDOMProcessingInstruction::JSGetProperty, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, JSDOMProcessingInstruction::JSDestructor,
  0, 0, 0, 0, 
  0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMProcessingInstruction::JSInit(JSContext *cx, JSObject *obj) {
  if (obj==NULL)
    obj = JS_GetGlobalObject(cx);

  jsval oldobj;
  if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMProcessingInstruction::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
    return JSVAL_TO_OBJECT(oldobj);

  JSObject *newobj = JS_InitClass(cx, obj, NULL, &JSDOMProcessingInstruction::_jsClass,
                                  NULL, 0,
                                  JSDOMProcessingInstruction::_JSPropertySpec, NULL,
                                  NULL, NULL);
  if (newobj) {
    InitNodeObject(cx, newobj);
  }

  return newobj;
}

// JavaScript Destructor
void JSDOMProcessingInstruction::JSDestructor(JSContext *cx, JSObject *obj) {
  JSDOMProcessingInstruction *p = (JSDOMProcessingInstruction*)JS_GetPrivate(cx, obj);
  if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMProcessingInstruction::getJSObject(JSContext *cx) {
  if (!cx) return NULL;
  if (!_JSinternal.o) {
    _JSinternal.o = newJSObject(cx);
    _JSinternal.c = cx;
    if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
  }
  return _JSinternal.o;
}

JSObject *JSDOMProcessingInstruction::newJSObject(JSContext *cx) {
  return JS_NewObject(cx, &JSDOMProcessingInstruction::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMProcessingInstruction::_JSPropertySpec[] = {
  { "target", JSDOMProcessingInstruction::JSVAR_target, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
  { "data", JSDOMProcessingInstruction::JSVAR_data, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
  { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMProcessingInstruction::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
  if (JSVAL_IS_INT(id)) {
    if (JSVAL_TO_INT(id) < JSDOMNode::JSVAR_LASTENUM)
      return JSDOMNode::JSGetProperty(cx, obj, id, vp);

    try {
      JSDOMProcessingInstruction *priv;
      switch(JSVAL_TO_INT(id)) {
      case JSVAR_target:
        priv = (JSDOMProcessingInstruction*)JS_GetPrivate(cx, obj);
        if (priv==NULL)
          *vp = JSVAL_NULL;
        else
          *vp = __STRING_TO_JSVAL(priv->getTarget());
        break;

      case JSVAR_data:
        priv = (JSDOMProcessingInstruction*)JS_GetPrivate(cx, obj);
        if (priv==NULL)
          *vp = JSVAL_NULL;
        else
          *vp = __STRING_TO_JSVAL(priv->getData());
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



JSDOMProcessingInstruction::JSDOMProcessingInstruction( DOMProcessingInstruction *pi, JSDOMDocument *doc ) : JSDOMNode( pi, doc ), _pi(pi)
{
}

JSString* JSDOMProcessingInstruction::getTarget()
{
  if (_pi == NULL) return NULL;
  XMLChToVXIchar target(_pi->getTarget());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, target.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

JSString* JSDOMProcessingInstruction::getData()
{
  if (_pi == NULL) return NULL;
  XMLChToVXIchar data(_pi->getData());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, data.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}
