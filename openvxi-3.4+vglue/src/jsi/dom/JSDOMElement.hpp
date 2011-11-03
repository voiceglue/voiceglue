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

#ifndef _JSDOMELEMENT_H
#define _JSDOMELEMENT_H

#include "JSDOMNode.hpp"
#include "JSDOMAttr.hpp"
#include "JSDOMNodeList.hpp"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
using namespace xercesc;

class JSDOMAttr;
class JSDOMNodeList;

class JSDOMElement : public JSDOMNode
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	virtual JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMElement( DOMElement *elem, JSDOMDocument *doc = NULL );
	virtual ~JSDOMElement(){}

	// properties
	JSString* getTagName();

	// methods
	JSString* getAttribute(JSString* name) ;
	JSDOMAttr* getAttributeNode(JSString* name) ;
	JSDOMNodeList* getElementsByTagName(JSString* name);
	JSString* getAttributeNS(JSString* namespaceURI, JSString* localName);
	JSDOMAttr* getAttributeNodeNS(JSString* namespaceURI, JSString* localName);
	JSDOMNodeList* getElementsByTagNameNS(JSString* namespaceURI, JSString* localName);
	bool hasAttribute(JSString* name);
	bool hasAttributeNS(JSString* namespaceURI, JSString* localName);

protected:
	// JavaScript Variable IDs
	enum {
		JSVAR_tagName = JSDOMNode::JSVAR_LASTENUM,
		JSVAR_LASTENUM
	};

	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

private:
	static JSClass _jsClass;
	static JSPropertySpec _JSPropertySpec[];
	static JSFunctionSpec _JSFunctionSpec[];

	static void JSDestructor(JSContext *cx, JSObject *obj);
	static JSBool JSFUNC_getAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getAttributeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getAttributeNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getAttributeNodeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getElementsByTagName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_getElementsByTagNameNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_hasAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_hasAttributeNS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

private:
	DOMElement *_elem;
};

#endif
