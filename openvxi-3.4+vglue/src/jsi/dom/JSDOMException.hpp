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

#ifndef _JSDOMEXCEPTION_H
#define _JSDOMEXCEPTION_H

#include <jsapi.h>

class JSDOMException
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	virtual JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMException(int code);
	virtual ~JSDOMException(){}

	enum ExceptionCode
	{
		INDEX_SIZE_ERR = 1,
		DOMSTRING_SIZE_ERR = 2,
		NO_MODIFICATION_ALLOWED_ERR = 7,
		NOT_FOUND_ERR = 8,
		NOT_SUPPORTED_ERR = 9,
		INVALID_STATE_ERR = 11
	};

	// properties
	int getCode();

protected:
	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

	enum {
		JSVAR_INDEX_SIZE_ERR,
		JSVAR_DOMSTRING_SIZE_ERR,
		JSVAR_NO_MODIFICATION_ALLOWED_ERR,
		JSVAR_NOT_FOUND_ERR,
		JSVAR_NOT_SUPPORTED_ERR,
		JSVAR_INVALID_STATE_ERR,
		JSVAR_code,
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

	static JSClass _jsClass;
	static JSPropertySpec _JSPropertySpec[];
	static JSPropertySpec _JSPropertySpec_static[];

private:
	static JSBool JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static void JSDestructor(JSContext *cx, JSObject *obj);

private:
	int _code;
};

#endif
