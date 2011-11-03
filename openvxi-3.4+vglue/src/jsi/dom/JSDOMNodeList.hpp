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

#ifndef _JSDOMNODELIST_H_
#define _JSDOMNODELIST_H_

#include "JSDOMNode.hpp"

#include <xercesc/dom/DOMNodeList.hpp>
using namespace xercesc;

class JSDOMDocument;

class JSDOMNodeList
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMNodeList(){}
	JSDOMNodeList(DOMNodeList *nodeList, JSDOMDocument *ownerDoc);

	// properties
	int getLength();

	// methods
	JSDOMNode *item( int index );

protected:
	enum {
		JSVAR_length,
		JSVAR_LASTENUM
	};

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

private:
	static JSClass _jsClass;
	static JSPropertySpec _JSPropertySpec[];
	static JSFunctionSpec _JSFunctionSpec[];

	static JSBool JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static void JSDestructor(JSContext *cx, JSObject *obj);
	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
	static JSBool JSFUNC_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

private:
	DOMNodeList *_nodeList;
	JSDOMDocument *_ownerDoc;
};


#endif
