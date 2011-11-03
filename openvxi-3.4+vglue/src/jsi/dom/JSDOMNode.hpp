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

#ifndef _JSDOMNODE_H
#define _JSDOMNODE_H

#include "JSDOM.hpp"

#include <xercesc/dom/DOMNode.hpp>
using namespace xercesc;

class JSDOMNodeList;
class JSDOMNamedNodeMap;
class JSDOMDocument;

class JSDOMNode
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	virtual JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMNode(){}
	JSDOMNode( DOMNode *node, JSDOMDocument *doc = NULL );
	virtual ~JSDOMNode(){}

	enum NodeType
	{
		ELEMENT_NODE = 1,
		ATTRIBUTE_NODE = 2,
		TEXT_NODE = 3,
		CDATA_SECTION_NODE = 4,
		ENTITY_REFERENCE_NODE = 5,
		ENTITY_NODE = 6,
		PROCESSING_INSTRUCTION_NODE = 7,
		COMMENT_NODE = 8,
		DOCUMENT_NODE = 9,
		DOCUMENT_TYPE_NODE = 10,
		DOCUMENT_FRAGMENT_NODE = 11, 
		NOTATION_NODE = 12
	};

	// methods
	bool hasChildNodes();
	bool hasAttributes();

	// properties
	JSString* getNodeName();
	JSString* getNodeValue();
	int getNodeType();
	JSDOMNode* getParentNode();
	JSDOMNodeList* getChildNodes();
	JSDOMNode* getFirstChild();
	JSDOMNode* getLastChild();
	JSDOMNode* getPreviousSibling();
	JSDOMNode* getNextSibling();
	JSDOMNamedNodeMap* getAttributes();
	virtual JSDOMDocument* getOwnerDocument();
	JSString* getNamespaceURI();
	JSString* getPrefix();
	JSString* getLocalName();

public:
	static JSDOMNode *createNode( DOMNode *node, JSDOMDocument *owner, JSDOMNode *ownerNode = NULL );

protected:
	static bool InitNodeObject( JSContext *cx, JSObject *obj );
	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

	enum {
		JSVAR_ELEMENT_NODE,
		JSVAR_ATTRIBUTE_NODE,
		JSVAR_TEXT_NODE,
		JSVAR_CDATA_SECTION_NODE,
		JSVAR_ENTITY_REFERENCE_NODE,
		JSVAR_ENTITY_NODE,
		JSVAR_PROCESSING_INSTRUCTION_NODE,
		JSVAR_COMMENT_NODE,
		JSVAR_DOCUMENT_NODE,
		JSVAR_nodeName,
		JSVAR_nodeValue,
		JSVAR_nodeType,
		JSVAR_parentNode,
		JSVAR_childNodes,
		JSVAR_firstChild,
		JSVAR_lastChild,
		JSVAR_previousSibling,
		JSVAR_nextSibling,
		JSVAR_attributes,
		JSVAR_ownerDocument,
		JSVAR_namespaceURI,
		JSVAR_prefix,
		JSVAR_localName,
		JSVAR_LASTENUM
	};

	struct _JSinternalStruct {
		JSObject *o;
		JSContext *c;
		_JSinternalStruct() : o(NULL) {};
		~_JSinternalStruct()
	    {
		//  if (o) JS_SetPrivate(NULL,o, NULL);
	    };
	};
	_JSinternalStruct _JSinternal;

	static JSClass _jsClass;
	static JSPropertySpec _JSPropertySpec[];
	static JSPropertySpec _JSPropertySpec_static[];
	static JSFunctionSpec _JSFunctionSpec[];

private:
	static JSBool JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static void JSDestructor(JSContext *cx, JSObject *obj);
	static JSBool JSFUNC_hasAttributes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool JSFUNC_hasChildNodes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

private:
	DOMNode *_node;
	JSDOMDocument *_owner;
};

#endif
