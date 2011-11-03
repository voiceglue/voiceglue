
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

/*****************************************************************************
 *
 * JsiContext, class for managing JavaScript contexts 
 *
 * The JsiContext class represents a JavaScript context, a script
 * execution state. All JavaScript variables are maintained in a
 * context, and all scripts are executed in reference to a context
 * (for accessing variables and maintaining script side-effects). Each
 * context may have one or more scopes that are used to layer the
 * state information so that it is possible for clients to control the
 * lifetime of state information within the context.
 * SBjsiInterface, definition of the real SBjsi resource object
 *
 ****************************************************************************/


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include "SBjsiInternal.h"

#include "JsiRuntime.hpp"
#include "JsiContext.hpp"

#include <string.h>
#include <deque>

#include "SBjsiLog.h"
#include "SBjsiString.hpp"
#include "JsiCharCvt.hpp"

#include "dom/JSDOMNode.hpp"
#include "dom/JSDOMDocument.hpp"
#include "dom/JSDOMNodeList.hpp"
#include "dom/JSDOMNamedNodeMap.hpp"
#include "dom/JSDOMCharacterData.hpp"
#include "dom/JSDOMElement.hpp"
#include "dom/JSDOMAttr.hpp"
#include "dom/JSDOMText.hpp"
#include "dom/JSDOMComment.hpp"
#include "dom/JSDOMEntityReference.hpp"
#include "dom/JSDOMProcessingInstruction.hpp"
#include "dom/JSDOMCDATA.hpp"
#include "dom/JSDOMException.hpp"

// SpiderMonkey class that describes our scope objects which we use
// as contexts
static JSClass SCOPE_CLASS = {
    "__SBjsiScope", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JsiContext::SCOPE_CLASS_SetProperty,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  
    JsiContext::ScopeFinalize, JSCLASS_NO_OPTIONAL_MEMBERS
};

// SpiderMonkey class that describes our content objects which we use
// to point at VXIContent objects
static JSClass CONTENT_CLASS = {
    "__SBjsiContent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  
    JsiContext::ContentFinalize, JSCLASS_NO_OPTIONAL_MEMBERS
};

// SpiderMonkey class that describes our objects which we create from
// a VXIMap, just very simple objects
static JSClass MAP_CLASS = {
    "__SBjsiMap", 0,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  
    JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
};

// Names for JS objects we protect from garbage collection, only used
// for SpiderMonkey debugging purposes
static const char GLOBAL_SCOPE_NAME[]  = "__SBjsiGlobalScope";
static const wchar_t GLOBAL_SCOPE_NAME_W[]  = L"__SBjsiGlobalScope";
static const char SCRIPT_OBJECT_NAME[] = "__SBjsiScriptObject";
static const char PROTECTED_JSVAL_NAME[] = "__SBjsiProtectedJsval";

// Global variable we use for temporaries
static const char GLOBAL_TEMP_VAR[] = "__SBjsiTempVar";
static const wchar_t GLOBAL_TEMP_VAR_W[] = L"__SBjsiTempVar";


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#ifndef GET_VXICHAR_FROM_JSCHAR
#define GET_VXICHAR_FROM_JSCHAR(out, in) \
  JscharToVXIchar convert_##out(in); \
  const VXIchar *out = convert_##out.c_str()
#endif

#ifndef GET_JSCHAR_FROM_VXICHAR
#define GET_JSCHAR_FROM_VXICHAR(out, outlen, in) \
  VXIcharToJschar convert_##out(in); \
  const jschar *out = convert_##out.c_str(); \
  size_t outlen = convert_##out.length()
#endif

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Wrapper class around jsval that automatically protects the held
// jsval from garbage collection
class JsiProtectedJsval {
 public:
  // Constructor and destructor
  JsiProtectedJsval(JSContext *c) : context(c), val(JSVAL_VOID) { }
  ~JsiProtectedJsval() { Clear(); }

  // Clear the value
  VXIjsiResult Clear() { 
    VXIjsiResult rc;
    rc = (((JSVAL_IS_GCTHING(val)) && !JS_RemoveRoot(context, &val)) ?
    VXIjsi_RESULT_FATAL_ERROR : VXIjsi_RESULT_SUCCESS);
    val = JSVAL_VOID;
    return rc;
  }
  
  // Set the value
  VXIjsiResult Set(jsval v) {
    VXIjsiResult rc = Clear();
    val = v;
    if (JSVAL_IS_GCTHING(val) &&
        (!JS_AddNamedRoot(context, &val, PROTECTED_JSVAL_NAME) )){
      rc = VXIjsi_RESULT_OUT_OF_MEMORY; // blown away by GC already!
    }
    return rc;
  }

  // Accessor
  const jsval Get() const { return val; }

 private:
  // Disabled copy constructor and assignment operator
  JsiProtectedJsval(const JsiProtectedJsval &v);
  JsiProtectedJsval &operator=(const JsiProtectedJsval &v);

 private:
  JSContext  *context;
  jsval      val;
};


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Scope chain class, used to keep the scope chain independantly of
// the actual user visible scope chain to avoid crashes from invalid
// or malicious scripts (push a scope called "session", then overwrite
// that variable with an integer named "session", then try to do more
// work in that scope)
class JsiScopeChainNode {
 public:
  // Constructor and destructor
  JsiScopeChainNode(JSContext *context, JsiScopeChainNode *p, 
         const VXIchar *n) :
    pushAliasFlag(false), name(n), parent(p), child(NULL), jsVal(context) { }
  ~JsiScopeChainNode() { }

  // Creation method
  VXIjsiResult Create(JSObject *scopeObj) { 
    return jsVal.Set(OBJECT_TO_JSVAL(scopeObj)); }

  // Accessors
  const SBjsiString & GetName()   const { return name; }
  jsval                GetJsval()  { return jsVal.Get(); }
  JSObject           * GetJsobj()  { return JSVAL_TO_OBJECT(jsVal.Get()); }
  JsiScopeChainNode  * GetParent() { return parent; }
  JsiScopeChainNode  * GetChild()  { return child; }

  // Set the child scope
  void SetChild(JsiScopeChainNode *c) { child = c; }

  // Release this scope and all under it
  VXIjsiResult Release();
  
  // Alias functions
  VXIjsiResult PushAlias(JsiScopeChainNode *_alias) 
  { 
    VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
    if (_alias)
      aliasList.push_front(_alias);
    else
      rc = VXIjsi_RESULT_FAILURE;
    return rc;
  } 
  
  JsiScopeChainNode* PopAlias(void) 
  {
    JsiScopeChainNode *a = NULL;
    if (!aliasList.empty())
    {
      a = aliasList.front();
      aliasList.pop_front();  
    }
    return a;
  }
  bool HasAlias(void) { return !aliasList.empty(); }
  const SBjsiString & GetAliasName(void) const
  {
    if (!aliasList.empty())
      return aliasList.front()->GetName();
    return GetName();
  }
  
  void SetAliasScopeFlag(void)   { pushAliasFlag = true; }
  bool IsSetAliasScopeFlag(void) { return pushAliasFlag; }
  void ResetAliasScopeFlag(void) { pushAliasFlag = false; }
  
 private:
  SBjsiString        name;      // Name of this scope
  JsiScopeChainNode  *parent;    // Parent scope
  JsiScopeChainNode  *child;     // Child scope, may be NULL
  JsiProtectedJsval   jsVal;     // JS object for the scope
  // alias list
  std::deque<JsiScopeChainNode*> aliasList;
  bool               pushAliasFlag;
};


VXIjsiResult JsiScopeChainNode::Release()
{
  JsiScopeChainNode *node = this, *next = NULL, *alias = NULL;
  while (node) {
    // if node is an alias, just pop the alias and continue
    if (node->HasAlias())
    {
      alias = node->PopAlias();
      if (alias) alias->jsVal.Clear();
      continue;
    }
    // Release the lock on the underlying object
    node->jsVal.Clear();

    // Clear pointers for safety and advance to child scopes
    next = node->child;
    node->parent = NULL;
    node->child = NULL;
    node = next;

    // NOTE: the deletion of this node and the child nodes is handled
    // by the ScopeFinalize() method that is called when the JS
    // object is garbage collected
  }

  return VXIjsi_RESULT_SUCCESS;
}


// To support VXML 2.0 SPEC, that added the read-only attribute to a namespace.
// Therefore any attempt to modify, create any variable in this namespace will result
// in java script semantic error.  This is the callback that will be called
// every time set property function is called.  It will check for read-only flag
// of the current scope to determine if it is read-only, if so return false.
JSBool JsiContext::SCOPE_CLASS_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  uintN attrp;
  JSBool foundp;
  JsiScopeChainNode * prdata = (JsiScopeChainNode *) JS_GetPrivate(cx, obj);
  if (prdata)
  {
    // return true, if pushing alias scope
    if (prdata->IsSetAliasScopeFlag() )
      return JS_TRUE;
    GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, prdata->GetName().c_str());
    JSObject *pobj = (prdata->GetParent() ? prdata->GetParent()->GetJsobj() : prdata->GetJsobj());
    if (JS_GetUCPropertyAttributes(cx, pobj,
                           tmpname, tmpnamelen,
                           &attrp, &foundp))
    {
      if (foundp && (attrp & JSPROP_READONLY))
      {
        return JS_FALSE;
      }
    }
  }
  return JS_TRUE;
}

VXIjsiResult JsiContext::CheckWriteable(JSContext *cx, JSObject *obj, const VXIchar* varname)
{
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  uintN attrp;
  JSBool foundp;
  JsiScopeChainNode * prdata = (JsiScopeChainNode *) JS_GetPrivate(cx, obj);
  // evaluate var name to get the root, i.e a.b.c, then the root is a
  // if there is no root i.e, a then we don't need to evaluate
  if (wcschr(varname, L'.'))
  {
    VXIchar* rootvar, *state = NULL;
    VXIchar _varname[1024];
    wcscpy(_varname, varname);
#ifdef WIN32
    rootvar = wcstok(_varname, L".");
#else
    rootvar = wcstok(_varname, L".", &state);
#endif
    if (rootvar) {
      JsiProtectedJsval val(context);
      VXIjsiResult rc = EvaluateScript(rootvar, &val);
      JSObject *classobj = JSVAL_TO_OBJECT(val.Get());
      // if this object is scope class then get the private data,
      // otherwise just ignore it
      if ((rc == VXIjsi_RESULT_SUCCESS) && JS_InstanceOf(cx, classobj, &SCOPE_CLASS, NULL))
        prdata = (JsiScopeChainNode *) JS_GetPrivate(cx, classobj);
      val.Clear();
    }
  }    
  
  if (prdata)
  {
    // get scope from the current parent
    JSObject* pscope = (prdata->GetParent() ? prdata->GetParent()->GetJsobj() : prdata->GetJsobj());

    // retrieve read-only attribute 
    GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, prdata->GetName().c_str());
    if (JS_GetUCPropertyAttributes(cx, pscope,
                           tmpname, tmpnamelen,
                           &attrp, &foundp))
    {
      if (foundp && (attrp & JSPROP_READONLY))
        return rc = VXIjsi_RESULT_NON_FATAL_ERROR;
    }
  }
  return rc;
}

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Constructor, only does initialization that cannot fail
JsiContext::JsiContext() : SBjsiLogger(MODULE_SBJSI, NULL, 0),
  version(JSVERSION_DEFAULT), runtime(NULL), context(NULL),
  contextRefs(0), scopeChain(NULL), currentScope(NULL), logEnabled(true),
  maxBranches(0L), numBranches(0L), exception(NULL)
{
}


// Destructor
JsiContext::~JsiContext()
{
  Diag(SBJSI_LOG_CONTEXT, L"JsiContext::~JsiContext", 
  L"start 0x%p, JS context 0x%p", this, context);
    
  ClearException();

  if (context) {
    // Lock the context for access
#ifdef JS_THREADSAFE
    JS_ResumeRequest(context, contextRefs);
#else
    if (!AccessBegin())
      return;
#endif

    // Destroy the scope chain, which automatically unroots the global
    // scope to allow garbage collection of everything
    if (scopeChain)
      scopeChain->Release();

    // Release the lock, must be done before destroying the context
#ifdef JS_THREADSAFE
    JS_EndRequest(context);
#else
    AccessEnd();
#endif

    // Destroy the context, is set to NULL
    runtime->DestroyContext(&context);
  }

  Diag(SBJSI_LOG_CONTEXT, L"JsiContext::~JsiContext", L"end 0x%p", this);
}


// Creation method
VXIjsiResult JsiContext::Create(
    JsiRuntime        *rt, 
    long               contextSize, 
    long               mb,
    VXIlogInterface   *l,
    VXIunsigned        diagTagBase)
{
    ClearException();

    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG,
		       "JsiContext::Create() started");

    if ((rt == NULL) || (contextSize < 1) || (mb < 1) || (l == NULL))
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	    voiceglue_log ((char) LOG_ERR,
			   "JsiContext::Create(): invalid argument");
	return VXIjsi_RESULT_INVALID_ARGUMENT;
    };

    // Copy simple variables
    runtime = rt;
    maxBranches = mb;

    // Base logging class initialization
    SetLog(l, diagTagBase);

    Diag(SBJSI_LOG_CONTEXT, L"JsiContext::Create", L"start 0x%p", this);

    // Create the context
    VXIjsiResult rc = runtime->NewContext(contextSize, &context);
    if (rc != VXIjsi_RESULT_SUCCESS)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	    voiceglue_log ((char) LOG_ERR,
			   "JsiContext::Create(): NewContext() failure");
	return rc;
    };

    // Lock the context for access
#ifdef JS_THREADSAFE
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG,
		       "JsiContext::Create(): JS_THREADSAFE set");
    JS_BeginRequest(context);
#else
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG,
		       "JsiContext::Create(): JS_THREADSAFE not set");
    if (!AccessBegin())
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	    voiceglue_log ((char) LOG_ERR,
			   "JsiContext::Create(): AccessBegin() failure");
	rc = VXIjsi_RESULT_SYSTEM_ERROR;
    };
#endif

    if (rc == VXIjsi_RESULT_SUCCESS)
    {
	// Attach this as private data to the context for use in callbacks
	JS_SetContextPrivate(context, this);

	// Set the callback to enforce maxBranches
#if JS_VERSION >= 180
#if JSUSESETOPERATIONCALLBACK >= 3
	JS_SetOperationCallback(context, JsiContext::BranchCallback,
				maxBranches);
#else
	JS_SetOperationCallback(context, JsiContext::BranchCallback);
#endif
#else
	JS_SetBranchCallback(context, JsiContext::BranchCallback);
#endif
  
	// Set the callback for reporting errors
	JS_SetErrorReporter(context, JsiContext::ErrorReporter);

	// Add JSOPTION_STRICT to enable strictness, which provides
	// additional warnings/errors.
	// 
	// TBD make these flags run-time configurable? Would be helpful for
	// migrating older apps to VoiceXML 2.0 April 2002 (April 2002
	// requires declaring variables prior to use, older VoiceXML does
	// not).
#if defined(JSI_MUST_DECLARE_VARS) || defined(JSI_STRICT)
	JS_SetOptions(context, JSOPTION_STRICT | JSOPTION_WERROR);
#endif

	// Create and intialize the global scope we require in the context,
	// the scope chain node object locks it to protect it from the
	// garbage collector
	JSObject *globalScope = JS_NewObject(context, &SCOPE_CLASS, NULL, NULL);
	if (!globalScope)
	{
	    rc = VXIjsi_RESULT_OUT_OF_MEMORY;
	}
	else
	{
	    Diag(SBJSI_LOG_SCOPE, L"JsiContext::Create", 
		 L"global scope 0x%p, context 0x%p", globalScope, context);

	    scopeChain = new JsiScopeChainNode
		(context, NULL, GLOBAL_SCOPE_NAME_W);
	    if (!scopeChain)
	    {
		rc = VXIjsi_RESULT_OUT_OF_MEMORY;
	    }
	    else
	    {
		rc = scopeChain->Create(globalScope);
		if (rc == VXIjsi_RESULT_SUCCESS)
		{
		    if (JS_SetPrivate(context, globalScope, scopeChain))
		    {
			currentScope = scopeChain;
		    }
		    else
		    {
			rc = VXIjsi_RESULT_FATAL_ERROR;
		    }
		}
		else
		{
		};
	    }
	}
    }

    // Initialize the standard JavaScript classes (Array, Math, String, etc.)
    if (rc == VXIjsi_RESULT_SUCCESS)
    {
	if (!JS_InitStandardClasses(context, currentScope->GetJsobj()))
	{
	    if (voiceglue_loglevel() >= LOG_ERR)
		voiceglue_log
		    ((char) LOG_ERR,
		     "JsiContext::Create(): JS_InitStandardClasses() failure");
	    rc = VXIjsi_RESULT_OUT_OF_MEMORY;
	};
    }

    // Set version only after there is a valid global scope object
    if ((rc == VXIjsi_RESULT_SUCCESS) &&
	(version != JSVERSION_DEFAULT))
    {
	JS_SetVersion(context, version);
    };

    if (rc == VXIjsi_RESULT_SUCCESS)
    {
	JSDOMNode::JSInit(context, currentScope->GetJsobj());
	if (JSDOMDocument::JSInit(context, currentScope->GetJsobj()) == NULL)
	{
	    if (voiceglue_loglevel() >= LOG_ERR)
		voiceglue_log
		    ((char) LOG_ERR,
		     "JsiContext::Create(): JSDOMDocument::JSInit() failure");
	};
	JSDOMNodeList::JSInit(context, currentScope->GetJsobj());
	JSDOMNamedNodeMap::JSInit(context, currentScope->GetJsobj());
	JSDOMCharacterData::JSInit(context, currentScope->GetJsobj());
	JSDOMElement::JSInit(context, currentScope->GetJsobj());
	JSDOMAttr::JSInit(context, currentScope->GetJsobj());
	JSDOMText::JSInit(context, currentScope->GetJsobj());
	JSDOMComment::JSInit(context, currentScope->GetJsobj());
	JSDOMCDATA::JSInit(context, currentScope->GetJsobj());
	JSDOMEntityReference::JSInit(context, currentScope->GetJsobj());
	JSDOMProcessingInstruction::JSInit(context, currentScope->GetJsobj());
	JSDOMException::JSInit(context, currentScope->GetJsobj());
    }

    // On failure, destroy the context here to avoid use of it
    if (rc != VXIjsi_RESULT_SUCCESS)
    {
	if (context)
	{
#ifdef JS_THREADSAFE
	    JS_EndRequest(context);
#endif
	    runtime->DestroyContext(&context);
	}
    }
#ifdef JS_THREADSAFE
    else
    {
	contextRefs = JS_SuspendRequest(context);
    }
#else

    if (!AccessEnd())
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	    voiceglue_log
		((char) LOG_ERR,
		 "JsiContext::Create(): AccessEnd() failure");
	rc = VXIjsi_RESULT_SYSTEM_ERROR;
    }
#endif

    Diag(SBJSI_LOG_CONTEXT, L"JsiContext::Create", 
	 L"end 0x%p, JS context 0x%p", this, context);
    if (voiceglue_loglevel() >= LOG_DEBUG)
	voiceglue_log ((char) LOG_DEBUG, "JsiContext::Create() finished");

    return rc;
}

void JsiContext::ClearException()
{
  if (exception)
    VXIValueDestroy(&exception);
}

// Create a script variable relative to the current scope
VXIjsiResult JsiContext::CreateVar(const VXIchar  *name, 
            const VXIchar  *expr)
{
  ClearException();
  if ((name == NULL) || (name[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;


  // If this is a fully qualified name, just do a SetVar() as that
  // handles explicitly scoped variables and this doesn't. No
  // functional effect to doing so, the only difference is that
  // CreateVar() masks vars of the same name from prior scopes when
  // SetVar() just sets the old one, having an explicit scope makes
  // that irrelevant since then JS knows exactly what to do.
  if (wcschr(name, L'.'))
    return SetVar(name, expr);

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;
  // Evaluate the expression, need to eval before setting so we can
  // deal with things like "1; 2;" which can't be handled if we just
  // make a temporary script
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  JsiProtectedJsval val(context);
  
  // Check if the current scope is writeable
  rc = CheckWriteable(context, currentScope->GetJsobj(), name);
  
  if ((rc == VXIjsi_RESULT_SUCCESS) && (expr != NULL) && (expr[0] != 0))
    rc = EvaluateScript(expr, &val);

  // Set the variable in the current scope directly, ensures we mask
  // any var of the same name from earlier scopes
  if (rc == VXIjsi_RESULT_SUCCESS) {
    if (IsValidVarName(context, currentScope->GetJsobj(), name))
    {
      GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
      if (!JS_DefineUCProperty(context, currentScope->GetJsobj(), 
        tmpname, tmpnamelen, val.Get(), NULL, NULL,
        JSPROP_ENUMERATE) )
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    }
    else
      rc = VXIjsi_RESULT_SYNTAX_ERROR;
  }

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Create a script variable relative to the current scope
VXIjsiResult JsiContext::CreateVar(const VXIchar  *name, 
            const VXIValue *value)
{
  ClearException();
  if ((name == NULL) || (name[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  // If this is a fully qualified name, just do a SetVar() as that
  // handles explicitly scoped variables and this doesn't. No
  // functional effect to doing so, the only difference is that
  // CreateVar() masks vars of the same name from prior scopes when
  // SetVar() just sets the old one, having an explicit scope makes
  // that irrelevant since then JS knows exactly what to do.
  if (wcschr(name, L'.'))
    return SetVar(name, value);

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // Convert the value to a JavaScript variable
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  JsiProtectedJsval val(context);

  // Check if the current scope is writeable
  rc = CheckWriteable(context, currentScope->GetJsobj(), name);
  
  if ((rc == VXIjsi_RESULT_SUCCESS) && value)
    rc = VXIValueToJsval(context, value, &val);

  // Set the variable in the current scope directly, ensures we mask
  // any var of the same name from earlier scopes
  if (rc == VXIjsi_RESULT_SUCCESS) {
    // check if variable name is valid
    if (IsValidVarName(context, currentScope->GetJsobj(), name))
    {
      GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
      if (!JS_DefineUCProperty(context, currentScope->GetJsobj(), 
            tmpname, tmpnamelen, val.Get(), NULL, NULL,
            JSPROP_ENUMERATE))
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    }
    else
      rc = VXIjsi_RESULT_SYNTAX_ERROR;
  }

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Set a script variable relative to the current scope
VXIjsiResult JsiContext::SetVar(const VXIchar     *name, 
         const VXIchar     *expr)
{
  ClearException();
  if ((name == NULL) || (name[0] == 0) ||
      (expr == NULL) || (expr[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  JsiProtectedJsval val(context);
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

  // Evaluate the expression, need to eval before setting so we can
  // deal with things like "1; 2;" which can't be handled if we just
  // make a temporary script
  if (rc == VXIjsi_RESULT_SUCCESS)
    rc = EvaluateScript(expr, &val);

  // assign the var
  if (rc == VXIjsi_RESULT_SUCCESS)
    rc = AssignVar(name, val);

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


VXIjsiResult JsiContext::CreateVar(
   const VXIchar  *name, 
   DOMDocument    *doc)
{
  ClearException();
  if ((name == NULL) || (name[0] == 0) || (doc == NULL))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  JSDOMDocument *jsdoc = new JSDOMDocument(doc);

  // Convert the value to a JavaScript variable
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  JsiProtectedJsval val(context);

  // Check if the current scope is writeable
  rc = CheckWriteable(context, currentScope->GetJsobj(), name);

  if (rc == VXIjsi_RESULT_SUCCESS)
    val.Set(OBJECT_TO_JSVAL(jsdoc->getJSObject(context)));

  // Set the variable in the current scope directly, ensures we mask
  // any var of the same name from earlier scopes
  if (rc == VXIjsi_RESULT_SUCCESS) {
    // check if variable name is valid
    if (IsValidVarName(context, currentScope->GetJsobj(), name))
    {
      GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
      if (!JS_DefineUCProperty(context, currentScope->GetJsobj(), 
            tmpname, tmpnamelen, val.Get(), NULL, NULL,
            JSPROP_ENUMERATE))
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    }
    else
      rc = VXIjsi_RESULT_SYNTAX_ERROR;
  }

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Set a script variable relative to the current scope
VXIjsiResult JsiContext::SetVar(const VXIchar   *name, 
         const VXIValue  *value)
{
  ClearException();
  if ((name == NULL) || (name[0] == 0) || (value == NULL))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // Convert the value to a JavaScript variable
  JsiProtectedJsval val(context);
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

  if (rc == VXIjsi_RESULT_SUCCESS)
    rc = VXIValueToJsval(context, value, &val);

  // assign the var
  if (rc == VXIjsi_RESULT_SUCCESS)
    rc = AssignVar(name, val);

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}

VXIjsiResult JsiContext::AssignVar(const VXIchar *name, JsiProtectedJsval &val)
{
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

  // make sure the var is writable
  rc = CheckWriteable(context, currentScope->GetJsobj(), name);
  if (rc == VXIjsi_RESULT_SUCCESS) {
    // What we'd like to do is lookup the variable, then set it to the
    // jsval we got above. (The lookup is complex because it could be
    // a partially or fully qualified name, with potential scope
    // chain/prototype chain lookups.) However, that isn't possible in
    // SpiderMonkey. So instead we set a temporary variable in the
    // global scope, then assign the specified object to that, then
    // destroy the temporary.
    if (JS_DefineProperty(context, currentScope->GetJsobj(),
          GLOBAL_TEMP_VAR, val.Get(), NULL, NULL, 0)) {
      // Do the copy using a temporary script
      SBjsiString script(name);
      script += L"=";
      script += GLOBAL_TEMP_VAR_W;
      rc = EvaluateScript(script.c_str(), NULL);

      // Destroy the temporary variable
      jsval ignored = JSVAL_VOID;
      if (!JS_DeleteProperty2(context, currentScope->GetJsobj(), 
         GLOBAL_TEMP_VAR, &ignored))
        rc = VXIjsi_RESULT_FATAL_ERROR;
    } else {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    }
  }

  return rc;
}

// Set the given scope namespace to read-only attribute
VXIjsiResult JsiContext::SetReadOnly(const VXIchar *name) const
{
  const_cast<JsiContext *>(this)->ClearException();
  if ((name == NULL) || (name[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;
  
  // setting the attribute of the given object's name
  // to read-only status.  By evaluating the object's name we'll
  // get to the correct scope.
  JSBool foundp; 
  JsiProtectedJsval val(context);
  VXIjsiResult rc = EvaluateScript(name, &val);
  
  // Check if this  variable is an object
  if ((rc == VXIjsi_RESULT_SUCCESS) && JSVAL_IS_OBJECT(val.Get()))
  {
    // Check if it is an instance of SCOPE_CLASS object
    JSObject *robj = JSVAL_TO_OBJECT(val.Get());    
    if (JS_InstanceOf(context, robj, &SCOPE_CLASS, NULL))
    {
      // find the parent node of the current scope because JS_SetUCPropertyAttributes
      // only works from top-down
      JsiScopeChainNode * prdata = (JsiScopeChainNode *) JS_GetPrivate(context, robj);
      if (prdata && prdata->GetParent())
      {
        // Set the property to read-only
        JSObject *pobj = prdata->GetParent()->GetJsobj(); 
        GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
        if (JS_SetUCPropertyAttributes(context, pobj, tmpname, tmpnamelen, 
            JSPROP_ENUMERATE | JSPROP_READONLY, &foundp))
        {
          if (!foundp)
            rc = VXIjsi_RESULT_FAILURE;         
        } 
        else
          rc = VXIjsi_RESULT_FAILURE;             
      }
    }
  }
    
  // Must unroot before unlocking
  val.Clear();
    
  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;
  
  return rc;
}


// Get the value of a variable
VXIjsiResult JsiContext::GetVar(const VXIchar     *name,
         VXIValue         **value) const
{
  const_cast<JsiContext *>(this)->ClearException();
  if ((name == NULL) || (name[0] == 0) || (value == NULL))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  *value = NULL;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // Get the variable, we need to evaluate instead of just calling
  // JS_GetUCProperty() in order to have it automatically look back
  // through the scope chain, as well as to permit explicit scope
  // references like "myscope.myvar"
  JsiProtectedJsval val(context);
  VXIjsiResult rc = EvaluateScript(name, &val);

  if (rc == VXIjsi_RESULT_SUCCESS) {
    rc = JsvalToVXIValue(context, val.Get(), value);
    if (rc == VXIjsi_RESULT_SYNTAX_ERROR)
      Error(405, L"%s%s", L"Invalid evaluation of javascript scope variable", name);
  }
  else if ((rc == VXIjsi_RESULT_SYNTAX_ERROR) ||
     (rc == VXIjsi_RESULT_SCRIPT_EXCEPTION))
    rc = VXIjsi_RESULT_NON_FATAL_ERROR;

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Check whether a variable is defined (not void, could be NULL)
VXIjsiResult JsiContext::CheckVar(const VXIchar   *name) const
{
  const_cast<JsiContext *>(this)->ClearException();
  if ((name == NULL) || (name[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // Get the variable, we need to evaluate instead of just calling
  // JS_GetUCProperty() in order to have it automatically look back
  // through the scope chain, as well as to permit explicit scope
  // references like "myscope.myvar". We disable logging because
  // we don't want JavaScript error reports about this var being
  // undefined.
  JsiProtectedJsval val(context);
  VXIjsiResult rc = EvaluateScript(name, &val, false);

  if ((rc == VXIjsi_RESULT_SYNTAX_ERROR) ||
      (rc == VXIjsi_RESULT_SCRIPT_EXCEPTION) ||
      (rc == VXIjsi_RESULT_NON_FATAL_ERROR) )
    rc = VXIjsi_RESULT_NON_FATAL_ERROR;
  else if ((rc == VXIjsi_RESULT_SUCCESS) &&
	  (JSVAL_IS_VOID(val.Get()))) {   // JavaScript undefined
      rc = VXIjsi_RESULT_FAILURE;
  }

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Execute a script, optionally returning any execution result
VXIjsiResult JsiContext::Eval(const VXIchar       *expr,
             VXIValue           **result)
{
  if ((expr == NULL) || (expr[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (result)
    *result = NULL;

  // Execute the script
  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  JsiProtectedJsval val(context);
  VXIjsiResult rc = EvaluateScript(expr, &val);
 
  if (result && (rc == VXIjsi_RESULT_SUCCESS) &&
      (val.Get() != JSVAL_VOID))
    rc = JsvalToVXIValue(context, val.Get(), result);

  // Must unroot before unlocking
  val.Clear();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Push a new context onto the scope chain (add a nested scope)
VXIjsiResult JsiContext::PushScope(const VXIchar *name, const VXIjsiScopeAttr attr)
{
  ClearException();
  if (!name || !name[0])
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // Create an object for the scope, the current scope is its parent
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  if (attr == VXIjsi_NATIVE_SCOPE){
    JSObject *scope = JS_NewObject(context, &SCOPE_CLASS, NULL, 
            currentScope->GetJsobj());
    if (!scope) {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else {
      Diag(SBJSI_LOG_SCOPE, L"JsiContext::PushScope", 
            L"scope %s (0x%p), context 0x%p", name, scope, context);
        
      // The scope chain node object locks it to protect this from
      // garbage collection in case someone (possibly a malicious
      // script) destroys the data member pointing at this in the global
      // scope
      JsiScopeChainNode *newScope = 
        new JsiScopeChainNode(context, currentScope, name);
      if (newScope) {
        rc = newScope->Create(scope);
        if ((rc == VXIjsi_RESULT_SUCCESS) &&
            (!JS_SetPrivate(context, scope, newScope) ))
          rc = VXIjsi_RESULT_FATAL_ERROR;
      } else {
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
      }
  
      // Set this scope as a property of the parent scope
      if (rc == VXIjsi_RESULT_SUCCESS) {
        GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
        if (JS_DefineUCProperty(context, currentScope->GetJsobj(), tmpname,
            tmpnamelen, newScope->GetJsval(), NULL, NULL, JSPROP_ENUMERATE) ) {
          // Add it to the scope chain and set it as the current scope
          currentScope->SetChild(newScope);
          currentScope = newScope;
        } else {
          rc = VXIjsi_RESULT_FATAL_ERROR;
        }
      }
    }
  }
  else if (attr == VXIjsi_ALIAS_SCOPE)
  {
    JSObject *aliasScope = JS_NewObject(context, &SCOPE_CLASS, NULL, 
            currentScope->GetJsobj());
    if (!aliasScope) {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else {
      Diag(SBJSI_LOG_SCOPE, L"JsiContext::PushScope", 
            L"alias scope %s (0x%p), context 0x%p", name, aliasScope, context);
      // The scope chain node object locks it to protect this from
      // garbage collection in case someone (possibly a malicious
      // script) destroys the data member pointing at this in the global
      // scope
      JsiScopeChainNode *newAliasScope = 
        new JsiScopeChainNode(context, currentScope->GetParent(), name);
      if (newAliasScope) {
        rc = newAliasScope->Create(aliasScope);
        if ((rc == VXIjsi_RESULT_SUCCESS) &&
            !JS_SetPrivate(context, aliasScope, newAliasScope))
          rc = VXIjsi_RESULT_FATAL_ERROR;
      } else {
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
      }
  
      // Set this scope as a property of the parent scope
      if (rc == VXIjsi_RESULT_SUCCESS) {
        GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, name);
        if (JS_DefineUCProperty(context, currentScope->GetJsobj(), tmpname,
            tmpnamelen, newAliasScope->GetJsval(), NULL, NULL, JSPROP_ENUMERATE)) {
          // set the alias scope to current scope by evaluating a javascript
          SBjsiString aliasScript(name);
          aliasScript += " = ";
          aliasScript += currentScope->GetName().c_str();
          currentScope->SetAliasScopeFlag();
          rc = EvaluateScript(aliasScript.c_str(), NULL);
          currentScope->ResetAliasScopeFlag();
          // Add it to the scope chain and set it as the current scope
          if (rc == VXIjsi_RESULT_SUCCESS)
            rc = currentScope->PushAlias(newAliasScope);
        } else {
          rc = VXIjsi_RESULT_FATAL_ERROR;
        }
      }
    }
  }
  else
    rc = VXIjsi_RESULT_INVALID_ARGUMENT;
    
  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Pop a context from the scope chain (remove a nested scope)
VXIjsiResult JsiContext::PopScope()
{
  ClearException();

  // Don't pop up past the global scope
  if (currentScope == scopeChain)
    return VXIjsi_RESULT_NON_FATAL_ERROR;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  // make sure we retrieve correct native scope name and alias
  const VXIchar* scopeName = (currentScope->HasAlias() ? 
                              currentScope->GetAliasName().c_str() : currentScope->GetName().c_str());
  
  Diag(SBJSI_LOG_SCOPE, L"JsiContext::PopScope", 
  L"scope %s (0x%p), context 0x%p", 
  scopeName, currentScope->GetJsobj(), context);

  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  
  // if the current scope is an alias, we pop the alias then unlock and return
  if (currentScope->HasAlias())
  {
    JsiScopeChainNode *oldAlias = currentScope->PopAlias();
    // Release the old scope variable within the parent scope to permit
    // it to be garbage collected
    rc = (oldAlias == NULL ? VXIjsi_RESULT_FAILURE : rc);
    if (rc == VXIjsi_RESULT_SUCCESS)
    {
      jsval rval;
      GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, oldAlias->GetName().c_str());
      if (!JS_DeleteUCProperty2(context, currentScope->GetJsobj(), tmpname,
             tmpnamelen, &rval))
        rc = VXIjsi_RESULT_FATAL_ERROR;

      // Now finish releasing the old scope, the actual wrapper object is
      // freed by the scope finalize method when garbage collection occurs
      rc = oldAlias->Release();
    }
    
    if (!AccessEnd())
      return VXIjsi_RESULT_SYSTEM_ERROR;
    return rc;
  }
  
  // Revert to the parent scope
  JsiScopeChainNode *oldScope = currentScope;
  currentScope = currentScope->GetParent();
  currentScope->SetChild(NULL);

  // Release the old scope variable within the parent scope to permit
  // it to be garbage collected
  jsval rval;
  GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, oldScope->GetName().c_str());
  if (!JS_DeleteUCProperty2(context, currentScope->GetJsobj(), tmpname,
             tmpnamelen, &rval))
    rc = VXIjsi_RESULT_FATAL_ERROR;

  // Now finish releasing the old scope, the actual wrapper object is
  // freed by the scope finalize method when garbage collection occurs
  rc = oldScope->Release();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Reset the scope chain to the global scope (pop all nested scopes)
VXIjsiResult JsiContext::ClearScopes()
{
  ClearException();

  // See if there is anything to do
  if (currentScope == scopeChain)
    return VXIjsi_RESULT_SUCCESS;

  if (!AccessBegin())
    return VXIjsi_RESULT_SYSTEM_ERROR;

  Diag(SBJSI_LOG_SCOPE, L"JsiContext::ClearScopes", L"context 0x%p", context);

  // Revert to the global scope
  JsiScopeChainNode *oldScope = scopeChain->GetChild();
  currentScope = scopeChain;
  currentScope->SetChild(NULL);

  // Release the old child scope variable within the global scope to
  // permit it and all its descendants to be garbage collected
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  jsval rval;
  GET_JSCHAR_FROM_VXICHAR(tmpname, tmpnamelen, oldScope->GetName().c_str());
  if (!JS_DeleteUCProperty2(context, currentScope->GetJsobj(), tmpname,
             tmpnamelen, &rval))
    rc = VXIjsi_RESULT_FATAL_ERROR;
  
  // Now finish releasing the old scope, the actual wrapper object is
  // freed by the scope finalize method when garbage collection occurs
  rc = oldScope->Release();

  if (!AccessEnd())
    rc = VXIjsi_RESULT_SYSTEM_ERROR;

  return rc;
}


// Script evaluation
VXIjsiResult JsiContext::EvaluateScript(const VXIchar *script, 
           JsiProtectedJsval *retval,
           bool loggingEnabled) const
{
  if ((script == NULL) || (script[0] == 0))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  // Reset our private data for evaluation callbacks
  EvaluatePrepare(loggingEnabled);
  if (retval)
    retval->Clear();

  // Compile the script
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  GET_JSCHAR_FROM_VXICHAR(tmpscript, tmpscriptlen, script);
  JSScript *jsScript = JS_CompileUCScript(context, currentScope->GetJsobj(), 
                                           tmpscript, tmpscriptlen, NULL, 1);
  if (!jsScript)
    rc = VXIjsi_RESULT_SYNTAX_ERROR;
  else {
    // Create a script object and root it to protect the script from
    // garbage collection, note that once this object exists it owns
    // the jsScript and thus we must not free it ourselves
    JSObject *jsScriptObj = JS_NewScriptObject(context, jsScript);
    if (!jsScriptObj || !JS_AddNamedRoot(context, &jsScriptObj, SCRIPT_OBJECT_NAME)) {
      JS_DestroyScript(context, jsScript);
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else {
      // Evaluate the script
      jsval val = JSVAL_VOID;
      if (JS_ExecuteScript(context, currentScope->GetJsobj(), jsScript, &val)) {
        if (retval)
          rc = retval->Set(val);
      } else if (exception) {
        rc = VXIjsi_RESULT_SCRIPT_EXCEPTION;
      } else if (numBranches > maxBranches) {
        rc = VXIjsi_RESULT_SECURITY_VIOLATION;
      } else {
        rc = VXIjsi_RESULT_NON_FATAL_ERROR;
      }
      
      // Release the script object
      if (!JS_RemoveRoot(context, &jsScriptObj))
        rc = VXIjsi_RESULT_FATAL_ERROR;
    }
  }
  
  return rc;
}

#include <iostream>
// Convert JS values to VXIValue types
VXIjsiResult JsiContext::JsvalToVXIValue (JSContext *context,
            const jsval val,
            VXIValue **value)
{
  if (value == NULL)
    return VXIjsi_RESULT_INVALID_ARGUMENT;
  
  *value = NULL;
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

  if (JSVAL_IS_STRING(val)) {
    GET_VXICHAR_FROM_JSCHAR(tmpval, JS_GetStringChars(JSVAL_TO_STRING(val)));
    *value = (VXIValue *) VXIStringCreate(tmpval);
  
  } else if (JSVAL_IS_INT(val))
    *value = (VXIValue *) VXIIntegerCreate(JSVAL_TO_INT (val));
  
  else if (JSVAL_IS_DOUBLE(val))
  // older version does not support double, unsigned long, etc.
#if (VXI_CURRENT_VERSION < 0x00030000)
    *value = (VXIValue *) VXIFloatCreate((double) *JSVAL_TO_DOUBLE(val));
#else    
    *value = (VXIValue *) VXIDoubleCreate((double) *JSVAL_TO_DOUBLE(val));
#endif
  
  else if (JSVAL_IS_BOOLEAN(val))
    *value = (VXIValue *) 
      VXIBooleanCreate((JSVAL_TO_BOOLEAN(val) ? TRUE : FALSE));
  
  else if (JSVAL_IS_NULL(val))  // JavaScript null
    rc = VXIjsi_RESULT_FAILURE;

  else if (JSVAL_IS_OBJECT(val)) {
    JSObject *obj = JSVAL_TO_OBJECT(val);
    if (JS_InstanceOf(context, obj, &SCOPE_CLASS, NULL))
    {
      // Not allowed to convert scope variable to VXIvalue
      rc = VXIjsi_RESULT_SYNTAX_ERROR;   
    }
    else if (JS_IsArrayObject(context, obj)) {
      // Array, create a VXIVector
      jsuint elements = 0;
      VXIVector *vec = NULL;
      if (!JS_GetArrayLength(context, obj, &elements)) {
        rc = VXIjsi_RESULT_FATAL_ERROR;
      } 
      else if ((vec = VXIVectorCreate()) == NULL) {
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
      } 
      else {
        // Traverse through the elements in the vector (could be empty,
        // in which case we want to return an empty VXIVector)
        for (jsuint i = 0; 
            ((rc == VXIjsi_RESULT_SUCCESS) && (i < elements)); i++) {
          // Convert the element to a VXIValue, then insert
          jsval elVal = JSVAL_VOID;
          if (JS_GetElement(context, obj, i, &elVal)) {
            VXIValue *elValue;
            rc = JsvalToVXIValue(context, elVal, &elValue);
            if ((rc == VXIjsi_RESULT_SUCCESS) &&
                (VXIVectorAddElement(vec, elValue) != 
                  VXIvalue_RESULT_SUCCESS)) {
              rc = VXIjsi_RESULT_OUT_OF_MEMORY;
              VXIValueDestroy(&elValue);
            }
          } else {
            rc = VXIjsi_RESULT_FATAL_ERROR;
          }
        }

        if (rc == VXIjsi_RESULT_SUCCESS)
          *value = (VXIValue *) vec;
        else
          VXIVectorDestroy(&vec);
      }

    } 
    else if (JS_InstanceOf(context, obj, &CONTENT_CLASS, NULL)) {
      // Content object, retrieve the VXIContent
      *value = (VXIValue *) JS_GetPrivate(context, obj);
      if (*value == NULL)
        rc = VXIjsi_RESULT_FATAL_ERROR;
      else 
        *value = VXIValueClone(*value);

    } else {
      // Regular object, create a VXIMap
      JSIdArray *props = JS_Enumerate(context, obj);
      VXIMap *vmap = NULL;
      if (!props) {
        rc = VXIjsi_RESULT_FATAL_ERROR;
      } else if ((vmap = VXIMapCreate()) == NULL) {
        rc = VXIjsi_RESULT_OUT_OF_MEMORY;
      } else {
        // Traverse through the properties in the object (could be empty,
        // in which case we want to return an empty VXIMap)
        jsint i = 0;
        for (i = 0;
            ((rc == VXIjsi_RESULT_SUCCESS) && (i < props->length)); i++) {
          // Get the property name, looks funky but this was
          // recommended by one of the SpiderMonkey contributors in a
          // newsgroup thread
          jsval prName = JSVAL_VOID, prVal = JSVAL_VOID;
          if (JS_IdToValue(context, props->vector[i], &prName)) {
            const jschar *name = NULL;
            size_t namelen = 0;
            if (JSVAL_IS_STRING(prName)) {
              name = JS_GetStringChars(JSVAL_TO_STRING(prName));
              namelen = JS_GetStringLength(JSVAL_TO_STRING(prName));
            } else if (JSVAL_IS_INT(prName)) {
              JSString *str = JS_ValueToString(context, prName);
              name = JS_GetStringChars(str);
              namelen = JS_GetStringLength(str);
            }
      
            // Lookup the property, convert to a value, then add to the map
            if (name &&
                JS_GetUCProperty(context, obj, name, namelen, &prVal)) {
              VXIValue *prValue = NULL;
              GET_VXICHAR_FROM_JSCHAR(tmpname, name);
              rc = JsvalToVXIValue(context, prVal, &prValue);
              if ((rc == VXIjsi_RESULT_SUCCESS) &&
                  (VXIMapSetProperty(vmap, tmpname, prValue) 
                                != VXIvalue_RESULT_SUCCESS)) {
                rc = VXIjsi_RESULT_OUT_OF_MEMORY;
                VXIValueDestroy(&prValue);
              }
              if (rc == VXIjsi_RESULT_NON_FATAL_ERROR)
                rc = VXIjsi_RESULT_SUCCESS;
            } else {
              rc = VXIjsi_RESULT_FATAL_ERROR;
            }
          } else {
            rc = VXIjsi_RESULT_FATAL_ERROR;
          }
        }

        if (rc == VXIjsi_RESULT_SUCCESS)
          *value = (VXIValue *) vmap;
        else
          VXIMapDestroy(&vmap);
      }

      // Free the ID array
      if (props)
        JS_DestroyIdArray(context, props);
    }
  }

  else                             // JavaScript undefined (VOID)
    rc = VXIjsi_RESULT_NON_FATAL_ERROR;

  // Check for out of memory during VXIValue type create
  if ((rc == VXIjsi_RESULT_SUCCESS) && (*value == NULL))
    rc = VXIjsi_RESULT_OUT_OF_MEMORY;

  return rc;
}


// Convert VXIValue types to JS values
VXIjsiResult JsiContext::VXIValueToJsval(JSContext *context,
            const VXIValue *value,
            JsiProtectedJsval *val)
{
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

  if ((value == NULL) || (val == NULL))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  switch (VXIValueGetType(value)) {
  case VALUE_BOOLEAN: {
    VXIbool i = VXIBooleanValue((const VXIBoolean *) value);
    rc = val->Set(BOOLEAN_TO_JSVAL(i));
    } break;

  case VALUE_INTEGER: {
    VXIint32 i = VXIIntegerValue((const VXIInteger *) value);
    if (INT_FITS_IN_JSVAL(i))
      rc = val->Set(INT_TO_JSVAL(i));
    else
      rc = VXIjsi_RESULT_NON_FATAL_ERROR;
    } break;

  case VALUE_FLOAT: {
    jsdouble *d = JS_NewDouble(context, 
         (double) VXIFloatValue((const VXIFloat *) value));
    if (d)
      rc = val->Set(DOUBLE_TO_JSVAL(d));
    else
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } break;

  // older version does not support double, unsigned long, etc.
#if (VXI_CURRENT_VERSION >= 0x00030000)
  case VALUE_DOUBLE: {
    jsdouble *d = JS_NewDouble(context, 
         (double) VXIDoubleValue((const VXIDouble *) value));
    if (d)
      rc = val->Set(DOUBLE_TO_JSVAL(d));
    else
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } break;
#endif
    
  case VALUE_STRING: {
    GET_JSCHAR_FROM_VXICHAR(tmpvalue, tmpvaluelen, 
          VXIStringCStr((const VXIString *) value));
    JSString *s = JS_NewUCStringCopyZ(context, tmpvalue);
    if (s)
      rc = val->Set(STRING_TO_JSVAL(s));
    else
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } break;

  case VALUE_PTR:
    // Not supported in JavaScript
    rc = VXIjsi_RESULT_NON_FATAL_ERROR;
    break;

  case VALUE_CONTENT: {
    // Create an object for the content, attach the passed content as
    // private data. We set the return value prior to setting the
    // private data to avoid getting garbage collected in the
    // meantime.
    VXIValue *c = NULL;
    JSObject *content = JS_NewObject(context, &CONTENT_CLASS, NULL, NULL);
    if (!content || 
        (val->Set(OBJECT_TO_JSVAL(content)) != VXIjsi_RESULT_SUCCESS) ||
        ((c = VXIValueClone(value)) == NULL)) {
      val->Clear();
      rc = VXIjsi_RESULT_OUT_OF_MEMORY; // JS object gets garbage collected
    } else if (!JS_SetPrivate(context, content, c)) {
      val->Clear();
      rc = VXIjsi_RESULT_FATAL_ERROR;
    }
  } break;

  case VALUE_MAP: {
    // Create an object for the map, temporarily root it so the data
    // added underneath is not garbage collected on us
    JSObject *mapObj = JS_NewObject(context, &MAP_CLASS, NULL, NULL);
    if (!mapObj) {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else if ((rc = val->Set(OBJECT_TO_JSVAL(mapObj))) ==
    VXIjsi_RESULT_SUCCESS ) {
      // Walk the map and add object properties, the map may be empty
      const VXIchar *prop;
      const VXIValue *propValue;
      VXIMapIterator *it = VXIMapGetFirstProperty((const VXIMap *) value, 
               &prop, &propValue);
      while (it  && (rc == VXIjsi_RESULT_SUCCESS)) {
        JsiProtectedJsval propVal(context);
        rc = VXIValueToJsval(context, propValue, &propVal);
        if (rc == VXIjsi_RESULT_SUCCESS) {
        GET_JSCHAR_FROM_VXICHAR(tmpprop, tmpproplen, prop);
        if (!JS_DefineUCProperty(context, mapObj, tmpprop, tmpproplen,
              propVal.Get(), NULL, NULL, 
              JSPROP_ENUMERATE))
          rc = VXIjsi_RESULT_OUT_OF_MEMORY;
        }
  
        if (VXIMapGetNextProperty(it, &prop, &propValue) != 
             VXIvalue_RESULT_SUCCESS) {
          VXIMapIteratorDestroy(&it);
          it = NULL;
        }
      }

      if (it)
        VXIMapIteratorDestroy(&it);
    }
  } break;

  case VALUE_VECTOR: {
    // Create a vector for the map, temporarily root it so the data
    // added underneath is not garbage collected on us
    JSObject *vecObj = JS_NewArrayObject(context, 0, NULL);
    if (!vecObj) {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else if ((rc = val->Set(OBJECT_TO_JSVAL(vecObj))) ==
                 VXIjsi_RESULT_SUCCESS) {
      // Walk the map and add object properties, the map may be empty
      const VXIVector *vec = (const VXIVector *) value;
      VXIunsigned i = 0;
      const VXIValue *elValue;
      while ((rc == VXIjsi_RESULT_SUCCESS) &&
            ((elValue = VXIVectorGetElement(vec, i)) != NULL )) {
        JsiProtectedJsval elVal(context);
        rc = VXIValueToJsval(context, elValue, &elVal);
        if ((rc == VXIjsi_RESULT_SUCCESS) &&
            (!JS_DefineElement(context, vecObj, i, elVal.Get(), NULL,
              NULL, JSPROP_ENUMERATE) ))
          rc = VXIjsi_RESULT_OUT_OF_MEMORY;

        i++;
      }
    }
  } break;

  default:
    rc = VXIjsi_RESULT_UNSUPPORTED;
  }

  if (rc != VXIjsi_RESULT_SUCCESS)
    val->Clear();

  return rc;
}


// Static callback for finalizing scopes
void JS_DLL_CALLBACK 
JsiContext::ScopeFinalize(JSContext *context, JSObject *obj)
{
  // Get this for logging the destruction
  JsiContext *pThis = (JsiContext *) JS_GetContextPrivate(context);

  // Get the scope node from the private data and delete it
  JsiScopeChainNode *scopeNode = 
    (JsiScopeChainNode *) JS_GetPrivate(context, obj);
  if (scopeNode) {
    if (pThis)
      pThis->Diag(SBJSI_LOG_SCOPE, L"JsiContext::ScopeFinalize",
          L"scope garbage collected %s (0x%p), context 0x%p", 
          scopeNode->GetName().c_str(), obj, context);

    delete scopeNode;
  }
}


// Static callback for finalizing content objects
void JS_DLL_CALLBACK JsiContext::ContentFinalize(JSContext *cx, JSObject *obj)
{
  // Get the scope name from the private data and delete it
  VXIContent *content = (VXIContent *) JS_GetPrivate(cx, obj);
  if (content) {
    VXIValue *tmp = (VXIValue *) content;
    VXIValueDestroy (&tmp);
  }
}


// Static callback for enforcing maxBranches
#if JS_VERSION >= 180
JSBool
JsiContext::BranchCallback(JSContext *context)
#else
JSBool JS_DLL_CALLBACK 
JsiContext::BranchCallback(JSContext *context, JSScript *script)
#endif
{
  if (!context)
    return JS_FALSE;

  // Get ourself from our private data
  JsiContext *pThis = (JsiContext *) JS_GetContextPrivate(context);
  if (!pThis) {
    // Severe error
    return JS_FALSE;
  }

  pThis->numBranches++;
  JSBool rc = JS_TRUE;
  if (pThis->numBranches > pThis->maxBranches) {
    rc = JS_FALSE;
    pThis->Error(JSI_WARN_MAX_BRANCHES_EXCEEDED, L"%s%ld", 
        L"maxBranches", pThis->maxBranches);
  }

  return rc;
}


// Static callback for reporting JavaScript errors
void JS_DLL_CALLBACK JsiContext::ErrorReporter(JSContext *context, 
            const char *message,
            JSErrorReport *report)
{
  if (!context)
    return;

  // Get ourself from our private data
  JsiContext *pThis = (JsiContext *) JS_GetContextPrivate(context);
  if (!pThis)
    return;

  // Convert the ASCII string to wide characters
  wchar_t wmessage[1024];
  size_t len = strlen(message);
  if (len > 1023)
    len = 1023;
  for (size_t i = 0; i < len; i++)
    wmessage[i] = (wchar_t) message[i];
  wmessage[len] = 0;

  // Save exception information, ownership of the value is passed
  if (pThis->exception)
    VXIValueDestroy(&pThis->exception);

  jsval ex = JSVAL_VOID;
  VXIValue *exception = NULL;
  if ((JS_IsExceptionPending(context)) &&
      (JS_GetPendingException(context, &ex)) &&
      (JsvalToVXIValue(context, ex, &exception) == VXIjsi_RESULT_SUCCESS)) {
    pThis->exception = exception;

    // update the message value with something more meaningful
    VXIMapSetProperty(
        reinterpret_cast<VXIMap *>(exception),
        L"message", 
        reinterpret_cast<VXIValue *>(VXIStringCreate(wmessage)));
  }

  if (pThis->logEnabled) {
    // Determine the error number to log
    unsigned int errNum;
    if (JSREPORT_IS_WARNING(report->flags))
      errNum = JSI_WARN_SM_SCRIPT_WARNING;
    else if (JSREPORT_IS_EXCEPTION(report->flags))
      errNum = JSI_ERROR_SM_SCRIPT_EXCEPTION;
    else if (JSREPORT_IS_STRICT(report->flags))
      errNum = JSI_WARN_SM_SCRIPT_STRICT;
    else
      errNum = JSI_ERROR_SM_SCRIPT_ERROR;
    
    // Log the error
    GET_VXICHAR_FROM_JSCHAR(tmpuclinebuf, report->uclinebuf);
    GET_VXICHAR_FROM_JSCHAR(tmpuctokenptr, report->uctokenptr);

    pThis->Error(errNum, L"%s%s%s%ld%s%s%s%s", 
        L"errmsg", wmessage,
        L"line", report->lineno, 
        L"linetxt", (tmpuclinebuf ? tmpuclinebuf : L""), 
        L"tokentxt", (tmpuctokenptr ? tmpuctokenptr : L""));
  }
}

// Check if the variable name is valid at declaration
bool JsiContext::IsValidVarName(JSContext *context, JSObject *obj, const VXIchar* vname)
{
  bool ret = true;
  SBjsiString tscript(L"var ");
  tscript += vname;
  GET_JSCHAR_FROM_VXICHAR(tmpscript, tmpscriptlen, tscript.c_str());
    // check if the variable name is valid
  JSScript *jsScript = JS_CompileUCScript(context, obj, 
                                           tmpscript, tmpscriptlen, NULL, 1);
  if (!jsScript) 
    ret = false;
  else
    JS_DestroyScript(context, jsScript);
  return ret;
}
