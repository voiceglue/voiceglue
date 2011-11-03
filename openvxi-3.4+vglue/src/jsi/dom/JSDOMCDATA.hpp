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

#ifndef _JSDOMCDATA_H_
#define _JSDOMCDATA_H_

#include "JSDOMText.hpp"

#include <xercesc/dom/DOMCDATASection.hpp>
using namespace xercesc;

class JSDOMCDATA : public JSDOMText
{
public:
	static JSObject *JSInit(JSContext *cx, JSObject *obj = NULL);
	JSObject *getJSObject(JSContext *cx);
	static JSObject *newJSObject(JSContext *cx);

	JSDOMCDATA(DOMCDATASection *cdata, JSDOMDocument *doc = NULL);
	virtual ~JSDOMCDATA(){}

protected:
	// JavaScript Variable IDs
	enum {
		JSVAR_LASTENUM = JSDOMText::JSVAR_LASTENUM
	};

	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

private:
	static JSClass _jsClass;

	static void JSDestructor(JSContext *cx, JSObject *obj);
};

#endif
