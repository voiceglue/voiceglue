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

#include "JSDOM.hpp"

JSBool js_throw_dom_error(JSContext* cx, JSObject* obj, int code)
{
  // if we get errors during error reporting we report those
  if (JS_AddNamedRoot(cx,&code,"jsint")) {
    jsval dummy;

    // We can't use JS_EvaluateScript since the stack would be wrong
    JSFunction *func;
    JSObject* fobj;
    const char* fbody="throw new DOMException(code);";
    const char* argnames[]={"code"};
    if ((func=JS_CompileFunction(cx, obj, NULL,
          1, argnames,
          fbody, strlen(fbody),
          NULL, 0)) != NULL) {
      // root function
      if ( ((fobj = JS_GetFunctionObject(func)) != NULL)
          && (JS_AddNamedRoot(cx, &fobj, "fobj")) ) {
        jsval args[]={INT_TO_JSVAL(code)};
        JS_CallFunction(cx, obj, func, 1, args, &dummy);
        JS_RemoveRoot(cx, &fobj);
      }
    }
    JS_RemoveRoot(cx,&code);
  }
  
  return JS_FALSE;
}


JSBool js_throw_error(JSContext* cx, JSObject* obj, const char* msg)
{
  JSString* jsstr;
  // if we get errors during error reporting we report those
  if ( ((jsstr=JS_NewStringCopyZ(cx, msg)) != NULL)
       && (JS_AddNamedRoot(cx,&jsstr,"jsstr")) ) {
    jsval dummy;
    // We can't use JS_EvaluateScript since the stack would be wrong
    JSFunction *func;
    JSObject* fobj;
    const char* fbody="throw new Error(msg);";
    const char* argnames[]={"msg"};
    if ((func=JS_CompileFunction(cx, obj, NULL,
          1, argnames,
          fbody, strlen(fbody),
          NULL, 0)) != NULL) {
      // root function
      if ( ((fobj = JS_GetFunctionObject(func)) != NULL)
          && (JS_AddNamedRoot(cx, &fobj, "fobj")) ) {
        jsval args[]={STRING_TO_JSVAL(jsstr)};
        JS_CallFunction(cx, obj, func, 1, args, &dummy);
        JS_RemoveRoot(cx, &fobj);
      }
    }
    JS_RemoveRoot(cx,&jsstr);
  }
  
  return JS_FALSE;
}
