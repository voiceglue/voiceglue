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

#include "JSDOMText.hpp"

// JavaScript class definition
JSClass JSDOMText::_jsClass = {
	"Text", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSDOMText::JSGetProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSDOMText::JSDestructor,
	0, 0, 0, 0, 
	0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMText::JSInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);

	jsval oldobj;
	if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMText::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
		return JSVAL_TO_OBJECT(oldobj);

	JSObject *newobj = JS_InitClass(cx, obj, NULL, &JSDOMText::_jsClass,
    	                                 NULL, 0,
    	                                 NULL, NULL,
    	                                 NULL, NULL);

	if (newobj) {
		InitCharacterDataObject(cx, newobj);
	}

	return newobj;
}

bool JSDOMText::InitTextObject(JSContext *cx, JSObject *obj)
{
	return InitCharacterDataObject(cx, obj);
}

// JavaScript Destructor
void JSDOMText::JSDestructor(JSContext *cx, JSObject *obj) {
	JSDOMText *p = (JSDOMText*)JS_GetPrivate(cx, obj);
	if (p) delete p;
}

JSBool JSDOMText::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return JSDOMCharacterData::JSGetProperty(cx, obj, id, vp);
}

// JavaScript Object Linking
JSObject *JSDOMText::getJSObject(JSContext *cx) {
	if (!cx) return NULL;
	if (!_JSinternal.o) {
		_JSinternal.o = newJSObject(cx);
		_JSinternal.c = cx;
		if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
	}
	return _JSinternal.o;
}

JSObject *JSDOMText::newJSObject(JSContext *cx) {
	return JS_NewObject(cx, &JSDOMText::_jsClass, NULL, NULL);
}


JSDOMText::JSDOMText( DOMText *text, JSDOMDocument *doc ) : JSDOMCharacterData(text, doc)
{
}
