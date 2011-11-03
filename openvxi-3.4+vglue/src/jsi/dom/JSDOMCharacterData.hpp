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

#ifndef _JSDOMCHARACTERDATA_H_
#define _JSDOMCHARACTERDATA_H_

#include "JSDOMNode.hpp"

#include <xercesc/dom/DOMCharacterData.hpp>
using namespace xercesc;

class JSDOMCharacterData : public JSDOMNode
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMCharacterData(DOMCharacterData *chardata, JSDOMDocument *doc = NULL);
	virtual ~JSDOMCharacterData(){}

	// properties
	JSString* getData();
	int getLength();

	// methods
	JSString* substringData(int offset, int count);

protected:
	// JavaScript Variable IDs
	enum {
		JSVAR_data = JSDOMNode::JSVAR_LASTENUM,
		JSVAR_length,
		JSVAR_LASTENUM
	};

	static bool InitCharacterDataObject(JSContext *cx, JSObject *obj);
	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

private:	
	static JSClass _jsClass;
	static JSPropertySpec _JSPropertySpec[];
	static JSFunctionSpec _JSFunctionSpec[];

	static void JSDestructor(JSContext *cx, JSObject *obj);
	static JSBool JSFUNC_substringData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

private:
	DOMCharacterData *_chardata;
};

#endif
