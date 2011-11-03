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

#include "JSDOMCharacterData.hpp"
#include <xercesc/dom/DOMException.hpp>

// JavaScript class definition
JSClass JSDOMCharacterData::_jsClass = {
  "CharacterData", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JSDOMCharacterData::JSGetProperty, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, JSDOMCharacterData::JSDestructor,
  0, 0, 0, 0, 
  0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMCharacterData::JSInit(JSContext *cx, JSObject *obj) {
  if (obj==NULL)
    obj = JS_GetGlobalObject(cx);

  jsval oldobj;
  if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMCharacterData::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
    return JSVAL_TO_OBJECT(oldobj);

  JSObject *newobj = JS_InitClass(cx, obj, NULL, &JSDOMCharacterData::_jsClass,
                                  NULL, 0,
                                  JSDOMCharacterData::_JSPropertySpec, JSDOMCharacterData::_JSFunctionSpec,
                                  NULL, NULL);
  if (newobj) {
    InitNodeObject(cx, newobj);
  }

  return newobj;
}

bool JSDOMCharacterData::InitCharacterDataObject(JSContext *cx, JSObject *obj)
{
  JS_DefineProperties( cx, obj, JSDOMCharacterData::_JSPropertySpec );
  JS_DefineFunctions( cx, obj, JSDOMCharacterData::_JSFunctionSpec );

  return InitNodeObject(cx, obj);
}

// JavaScript Destructor
void JSDOMCharacterData::JSDestructor(JSContext *cx, JSObject *obj) {
  JSDOMCharacterData *p = (JSDOMCharacterData*)JS_GetPrivate(cx, obj);
  if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMCharacterData::getJSObject(JSContext *cx) {
  if (!cx) return NULL;
  if (!_JSinternal.o) {
    _JSinternal.o = newJSObject(cx);
    _JSinternal.c = cx;
    if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
  }
  return _JSinternal.o;
}

JSObject *JSDOMCharacterData::newJSObject(JSContext *cx) {
  return JS_NewObject(cx, &JSDOMCharacterData::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMCharacterData::_JSPropertySpec[] = {
    { "data", JSDOMCharacterData::JSVAR_data, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { "length", JSDOMCharacterData::JSVAR_length, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
    { 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMCharacterData::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
  if (JSVAL_IS_INT(id)) {
    if (JSVAL_TO_INT(id) < JSDOMNode::JSVAR_LASTENUM )
      return JSDOMNode::JSGetProperty(cx, obj, id, vp);

    try {
      JSDOMCharacterData *priv;
      switch(JSVAL_TO_INT(id)) {
      case JSVAR_data:
        priv = (JSDOMCharacterData*)JS_GetPrivate(cx, obj);
        if (priv==NULL)
          *vp = JSVAL_NULL;
        else
          *vp = __STRING_TO_JSVAL(priv->getData());
        break;

      case JSVAR_length:
        priv = (JSDOMCharacterData*)JS_GetPrivate(cx, obj);
        if (priv==NULL)
          *vp = JSVAL_NULL;
        else
          *vp = __int_TO_JSVal(cx,priv->getLength());
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
JSFunctionSpec JSDOMCharacterData::_JSFunctionSpec[] = {
    { "substringData", JSFUNC_substringData, 2, 0, 0 },
    { 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMCharacterData::JSFUNC_substringData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
  JSDOMCharacterData *p = (JSDOMCharacterData*)JS_GetPrivate(cx, obj);
  if (argc < 2) return JS_FALSE;
  if (argc == 2) {
    try {
      if (JSVAL_IS_NUMBER(argv[0]) && JSVAL_IS_NUMBER(argv[1])) {
        *rval = __STRING_TO_JSVAL(p->substringData(
        __JSVal_TO_int(argv[0]),
        __JSVal_TO_int(argv[1])));
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
  return JS_FALSE;
}



JSDOMCharacterData::JSDOMCharacterData( DOMCharacterData *chardata, JSDOMDocument *doc ) : JSDOMNode(chardata,doc), _chardata(chardata)
{
}

JSString* JSDOMCharacterData::getData()
{
  if (!_chardata)
    return NULL;
  XMLChToVXIchar data(_chardata->getData());
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, data.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}

int JSDOMCharacterData::getLength()
{
  return ( _chardata ? _chardata->getLength() : 0 );
}

JSString* JSDOMCharacterData::substringData(int offset, int count)
{
  if (!_chardata)
    return NULL;
  XMLChToVXIchar data(_chardata->substringData(offset, count));
  GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, data.c_str());
  return JS_NewUCStringCopyZ(_JSinternal.c, tmpvalue);
}
