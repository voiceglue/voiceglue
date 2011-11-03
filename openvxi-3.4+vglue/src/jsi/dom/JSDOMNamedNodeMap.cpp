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

#include "JSDOMNamedNodeMap.hpp"
#include "JSDOMNode.hpp"

// JavaScript class definition
JSClass JSDOMNamedNodeMap::_jsClass = {
	"NamedNodeMap", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSDOMNamedNodeMap::JSGetProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSDOMNamedNodeMap::JSDestructor,
	0, 0, 0, 0, 
	0, 0, 0, 0
};

// JavaScript Initialization Method
JSObject *JSDOMNamedNodeMap::JSInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);
	jsval oldobj;
	if (JS_TRUE == JS_LookupProperty(cx, obj, JSDOMNamedNodeMap::_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))
		return JSVAL_TO_OBJECT(oldobj);
	return JS_InitClass(cx, obj, NULL, &JSDOMNamedNodeMap::_jsClass,
    	                                 NULL, 0,
    	                                 JSDOMNamedNodeMap::_JSPropertySpec, JSDOMNamedNodeMap::_JSFunctionSpec,
    	                                 NULL, NULL);
}

// JavaScript Destructor
void JSDOMNamedNodeMap::JSDestructor(JSContext *cx, JSObject *obj) {
	JSDOMNamedNodeMap *p = (JSDOMNamedNodeMap*)JS_GetPrivate(cx, obj);
	if (p) delete p;
}

// JavaScript Object Linking
JSObject *JSDOMNamedNodeMap::getJSObject(JSContext *cx) {
	if (!cx) return NULL;
	if (!_JSinternal.o) {
		_JSinternal.o = newJSObject(cx);
		_JSinternal.c = cx;
		if (!JS_SetPrivate(cx, _JSinternal.o, this)) return NULL;
	}
	return _JSinternal.o;
}

JSObject *JSDOMNamedNodeMap::newJSObject(JSContext *cx) {
	return JS_NewObject(cx, &JSDOMNamedNodeMap::_jsClass, NULL, NULL);
}

// JavaScript Variable Table
JSPropertySpec JSDOMNamedNodeMap::_JSPropertySpec[] = {
	{ "length", JSVAR_length, JSPROP_ENUMERATE | JSPROP_READONLY, 0, 0},
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Get Property Wrapper
JSBool JSDOMNamedNodeMap::JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	if (JSVAL_IS_INT(id)) {
		JSDOMNamedNodeMap *priv;
		switch(JSVAL_TO_INT(id)) {
			case JSVAR_length:
				priv = (JSDOMNamedNodeMap*)JS_GetPrivate(cx, obj);
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
JSFunctionSpec JSDOMNamedNodeMap::_JSFunctionSpec[] = {
	{ "getNamedItem", JSFUNC_getNamedItem, 1, 0, 0 },
	{ "getNamedItemNS", JSFUNC_getNamedItemNS, 2, 0, 0 },
	{ "item", JSFUNC_item, 1, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};

// JavaScript Function Wrappers
JSBool JSDOMNamedNodeMap::JSFUNC_getNamedItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSDOMNamedNodeMap *p = (JSDOMNamedNodeMap*)JS_GetPrivate(cx, obj);
	if (argc < 1) return JS_FALSE;
	if (argc == 1) {
		if (JSVAL_IS_STRING(argv[0])) {
			JSDOMNode *n = p->getNamedItem(JSVAL_TO_STRING(argv[0]));
			*rval = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

JSBool JSDOMNamedNodeMap::JSFUNC_getNamedItemNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSDOMNamedNodeMap *p = (JSDOMNamedNodeMap*)JS_GetPrivate(cx, obj);
	if (argc < 2) return JS_FALSE;
	if (argc == 2) {
		if (JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) {
			JSDOMNode *n = p->getNamedItemNS(
				JSVAL_TO_STRING(argv[0]),
				JSVAL_TO_STRING(argv[1]));
			*rval = (n ? __objectp_TO_JSVal(cx,n) : JSVAL_NULL);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

JSBool JSDOMNamedNodeMap::JSFUNC_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSDOMNamedNodeMap *p = (JSDOMNamedNodeMap*)JS_GetPrivate(cx, obj);
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



JSDOMNamedNodeMap::JSDOMNamedNodeMap(DOMNamedNodeMap *map, JSDOMDocument *ownerDoc) 
    : _map(map), _ownerDoc(ownerDoc), _ownerNode(NULL)
{
}

JSDOMNamedNodeMap::JSDOMNamedNodeMap(DOMNamedNodeMap *map, JSDOMNode *ownerNode) 
    : _map(map), _ownerDoc(ownerNode->getOwnerDocument()), _ownerNode(ownerNode)
{
}


int JSDOMNamedNodeMap::getLength()
{
	return (_map ? _map->getLength() : 0);
}

JSDOMNode *JSDOMNamedNodeMap::getNamedItem( JSString *name )
{
	if (!_map) return NULL;

	GET_VXICHAR_FROM_JSCHAR(tmpval, JS_GetStringChars(name));
	VXIcharToXMLCh vxiname(tmpval);

	DOMNode *node = _map->getNamedItem(vxiname.c_str());
	return JSDOMNode::createNode(node, _ownerDoc);
}

JSDOMNode *JSDOMNamedNodeMap::item( int index )
{
	if (!_map) return NULL;
	if (index > (_map->getLength() - 1)) return NULL;
	return JSDOMNode::createNode(_map->item(index), _ownerDoc, _ownerNode);
}

JSDOMNode *JSDOMNamedNodeMap::getNamedItemNS( JSString *namespaceURI, JSString *localName )
{
	if (!_map) return NULL;

	GET_VXICHAR_FROM_JSCHAR(nsval, JS_GetStringChars(namespaceURI));
	VXIcharToXMLCh vxins(nsval);

	GET_VXICHAR_FROM_JSCHAR(nameval, JS_GetStringChars(localName));
	VXIcharToXMLCh vxiname(nameval);

	DOMNode *node = _map->getNamedItemNS(vxins.c_str(), vxiname.c_str());
	return JSDOMNode::createNode(node, _ownerDoc);
}
