/*****************************************************************************
 *****************************************************************************
 *
 * JsiRuntime, class for managing JavaScript runtime environments
 *
 * The JsiRuntime class represents a JavaScript interpreter runtime
 * environment, which is the overall JavaScript engine execution space
 * (compiler, interpreter, decompiler, garbage collector, etc.) which
 * can hold zero or more JavaScript contexts (JsiContext objects).
 *
 *****************************************************************************
 ****************************************************************************/

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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#ifndef _JSI_RUNTIME_H__
#define _JSI_RUNTIME_H__

#include "VXIjsi.h"              // For VXIjsiResult codes
#include "SBjsiLogger.hpp"      // For SBjsiLogger base class

#ifndef HAVE_SPIDERMONKEY
#error Need Mozilla SpiderMonkey to build this ECMAScript integration
#endif
#include <jspubtd.h>             // SpiderMonkey JavaScript typedefs (JS...)

#if JS_VERSION >= 180
#define JS_DLL_CALLBACK		 // SpiderMonkey 1.8+ no longer uses
#endif
#ifndef JS_DLL_CALLBACK
#define JS_DLL_CALLBACK CRT_CALL // For SpiderMonkey 1.5 RC 3 and earlier
#endif

#ifndef JS_THREADSAFE
extern "C" struct VXItrdMutex;
#endif

extern "C" struct VXIlogInterface;

class JsiRuntime : SBjsiLogger {
 public:
  // Constructor and destructor
  JsiRuntime( );
  virtual ~JsiRuntime( );

  // Creation method
  VXIjsiResult Create (long              runtimeSize,
		       VXIlogInterface  *log,
		       VXIunsigned       diagTagBase);
  
 public:
  // These are only for JsiContext use!

  // Get a new JavaScript context for the runtime
  VXIjsiResult NewContext (long contextSize, JSContext **context);

  // Destroy a JavaScript context for the runtime
  VXIjsiResult DestroyContext (JSContext **context);

#ifndef JS_THREADSAFE
  // Flag that we are beginning and ending access of this runtime
  // (including any access of a context within this runtime), used to
  // ensure thread safety. For simplicity, return true on success,
  // false on failure (mutex error).
  bool AccessBegin( ) const;
  bool AccessEnd( ) const;
#endif
  
 private:
  // Static callback for diagnostic logging of garbage collection
  static JSBool JS_DLL_CALLBACK GCCallback (JSContext *cx, JSGCStatus status);

  // Disable the copy constructor and assignment operator
  JsiRuntime (const JsiRuntime &rt);
  JsiRuntime & operator= (const JsiRuntime &rt);

 private:
  VXIlogInterface  *log;         // For logging
  JSRuntime        *runtime;     // JavaScript runtime environment

#ifndef JS_THREADSAFE
  VXItrdMutex      *mutex;       // For thread safe evals
#endif
};

#endif  // _JSI_RUNTIME_H__
