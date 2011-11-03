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

#ifndef _JSDOMNAMEDNODEMAP_H_
#define _JSDOMNAMEDNODEMAP_H_

#include "JSDOM.hpp"

#include <xercesc/dom/DOMNamedNodeMap.hpp>
using namespace xercesc;

class JSDOMNode;
class JSDOMDocument;

class JSDOMNamedNodeMap
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMNamedNodeMap(DOMNamedNodeMap *map, JSDOMDocument *ownerDoc);
	JSDOMNamedNodeMap(DOMNamedNodeMap *map, JSDOMNode *ownerNode);

	// properties
	int getLength();

	// methods
	JSDOMNode *getNamedItem( JSString *name );
	JSDOMNode *item( int index );
	JSDOMNode *getNamedItemNS( JSString *namespaceURI, JSString *localName );

protected:
	struct _JSinternalStruct {
	    JSObject *o;
	    JSContext *c;
	    _JSinternalStruct() : o(NULL) {};
	    ~_JSinternalStruct()
	    {
		// if (o) JS_SetPrivate(NULL,o, NULL);
	    };
	};
	_JSinternalStruct _JSinternal;

	// JavaScript Variable IDs
	enum {
		JSVAR_length,
		JSVAR_LASTENUM
	};

	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

private:
	static JSClass _jsClass;

	static JSFunctionSpec _JSFunctionSpec[];
	static JSPropertySpec _JSPropertySpec[];

	static void JSDestructor(JSContext *cx, JSObject *obj);
	static JSBool JSFUNC_getNamedItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getNamedItemNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

private:
	DOMNamedNodeMap *_map;
	JSDOMDocument *_ownerDoc;
	JSDOMNode *_ownerNode;
};


#endif
