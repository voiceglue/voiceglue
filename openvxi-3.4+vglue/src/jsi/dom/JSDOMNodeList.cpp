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

#include "JSDOMNodeList.hpp"
#include "JSDOMNode.hpp"

// JavaScript class definition
JSClass JSDOMNodeList::_jsClass = {
	"NodeList", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSDOMNodeList::JSGetProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSDOMNodeList::JSDestructor,
	0, 0, 0, 0, 
	0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMNodeList::JSInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);
	jsval oldobj;
	if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMNodeList::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
		return JSVAL_TO_OBJECT(oldobj);
	return JS_InitClass(cx, obj, NULL, &JSDOMNodeList::_jsClass,
    	                                 JSDOMNodeList::JSConstructor, 0,
    	                                 JSDOMNodeList::_JSPropertySpec, JSDOMNodeList::_JSFunctionSpec,
    	                                 NULL, NULL);
}

// JavaScript Constructor
JSBool JSDOMNodeList::JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSDOMNodeList *p = NULL;
	if (argc == 0) {
		p = new JSDOMNodeList;
	}

	if (!p || !JS_SetPrivate(cx, obj, p)) return JS_FALSE;
	p->_JSinternal.o = obj;
	p->_JSinternal.c = cx;
	*rval = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

// JavaScript Destructor
void JSDOMNodeList::JSDestructor(JSContext *cx, JSObject *obj) {
	JSDOMNodeList *p = (JSDOMNodeList*)JS_GetPrivate(cx, obj);
	if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMNodeList::getJSObject(JSContext *cx) {
	if (!cx) return NULL;
	if (!_JSinternal.o) {
		_JSinternal.o = newJSObject(cx);
		_JSinternal.c = cx;
		if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
	}
	return _JSinternal.o;
}

JSObject *JSDOMNodeList::newJSObject(JSContext *cx) {
	return JS_NewObject(cx, &JSDOMNodeList::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMNodeList::_JSPropertySpec[] = {
	{ "length", JSVAR_length, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMNodeList::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	if (JSVAL_IS_INT(id)) {
		JSDOMNodeList *priv;
		switch(JSVAL_TO_INT(id)) {
			case JSVAR_length:
				priv = (JSDOMNodeList*)JS_GetPrivate(cx, obj);
				if (priv==NULL)
					*vp = JSVAL_NULL;
				else
					*vp = __int_TO_JSVal(cx,priv->getLength());
				break;

		}
	}
	return JS_TRUE;
}

// JavaScript Function Table
JSFunctionSpec JSDOMNodeList::_JSFunctionSpec[] = {
	{ "item", JSFUNC_item, 1, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMNodeList::JSFUNC_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSDOMNodeList *p = (JSDOMNodeList*)JS_GetPrivate(cx, obj);
	if (argc < 1) return JS_FALSE;
	if (argc == 1) {
		if (JSVAL_IS_NUMBER(argv[0])) {
			JSDOMNode *n = p->item(__JSVal_TO_int(argv[0]));
			*rval = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}




JSDOMNodeList::JSDOMNodeList(DOMNodeList *nodeList, JSDOMDocument *ownerDoc) 
   : _nodeList( nodeList ), _ownerDoc(ownerDoc)
{
}

int JSDOMNodeList::getLength()
{
	return ( _nodeList ? _nodeList->getLength() : 0 );
}

JSDOMNode *JSDOMNodeList::item(int index)
{
	if (!_nodeList)
		return NULL;
	if (index > (_nodeList->getLength() - 1))
		return NULL;
	return JSDOMNode::createNode(_nodeList->item(index), _ownerDoc);
}
