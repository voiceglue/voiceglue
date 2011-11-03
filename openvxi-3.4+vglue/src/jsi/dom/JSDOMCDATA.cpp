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

#include "JSDOMCDATA.hpp"

// JavaScript class definition
JSClass JSDOMCDATA::_jsClass = {
	"CDATASection", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSDOMCDATA::JSGetProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSDOMCDATA::JSDestructor,
	0, 0, 0, 0, 
	0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMCDATA::JSInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);

	jsval oldobj;
	if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMCDATA::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
		return JSVAL_TO_OBJECT(oldobj);

	JSObject *newobj = JS_InitClass(cx, obj, NULL, &JSDOMCDATA::_jsClass,
    	                                 NULL, 0,
    	                                 NULL, NULL,
    	                                 NULL, NULL);

	if (newobj) {
		InitTextObject(cx, newobj);
	}

	return newobj;
}

// JavaScript Destructor
void JSDOMCDATA::JSDestructor(JSContext *cx, JSObject *obj) {
	JSDOMCDATA *p = (JSDOMCDATA*)JS_GetPrivate(cx, obj);
	if (p) delete p;
}

JSBool JSDOMCDATA::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	return JSDOMText::JSGetProperty(cx, obj, id, vp);
}

// JavaScript Object Linking
JSObject *JSDOMCDATA::getJSObject(JSContext *cx) {
	if (!cx) return NULL;
	if (!_JSinternal.o) {
		_JSinternal.o = newJSObject(cx);
		_JSinternal.c = cx;
		if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
	}
	return _JSinternal.o;
}

JSObject *JSDOMCDATA::newJSObject(JSContext *cx) {
	return JS_NewObject(cx, &JSDOMCDATA::_jsClass, NULL, NULL);
}


JSDOMCDATA::JSDOMCDATA( DOMCDATASection *cdata, JSDOMDocument *doc ) : JSDOMText(cdata,doc)
{
}
