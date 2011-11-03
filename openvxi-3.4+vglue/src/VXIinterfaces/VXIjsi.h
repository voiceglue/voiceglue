
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

#ifndef _VXIJSI_H
#define _VXIJSI_H

#include "VXItypes.h"                  /* For VXIchar, VXIint, etc.  */
#include "VXIvalue.h"                  /* For VXIValue */

#include "VXIheaderPrefix.h"
#ifdef VXIJSI_EXPORTS
#define VXIJSI_API SYMBOL_EXPORT_DECL
#else
#define VXIJSI_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
struct VXIjsiContext;
#else
typedef struct VXIjsiContext { void * dummy; } VXIjsiContext;
#endif

/**
 * \defgroup VXIjsi ECMAScript Interface
 */

/*@{*/

/**
 * Result codes for interface methods
 *
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues)
 */
typedef enum VXIjsiResult {
  /** Fatal error, terminate call    */
  VXIjsi_RESULT_FATAL_ERROR       =  -100,
  /** I/O error                      */
  VXIjsi_RESULT_IO_ERROR           =   -8,
  /** Out of memory                  */
  VXIjsi_RESULT_OUT_OF_MEMORY      =   -7,
  /** System error, out of service   */
  VXIjsi_RESULT_SYSTEM_ERROR       =   -6,
  /** Errors from platform services  */
  VXIjsi_RESULT_PLATFORM_ERROR     =   -5,
  /** Return buffer too small        */
  VXIjsi_RESULT_BUFFER_TOO_SMALL   =   -4,
  /** Property name is not valid    */
  VXIjsi_RESULT_INVALID_PROP_NAME  =   -3,
  /** Property value is not valid   */
  VXIjsi_RESULT_INVALID_PROP_VALUE =   -2,
  /** Invalid function argument      */
  VXIjsi_RESULT_INVALID_ARGUMENT   =   -1,
  /** Success                        */
  VXIjsi_RESULT_SUCCESS            =    0,
  /** Normal failure, nothing logged */
  VXIjsi_RESULT_FAILURE            =    1,
  /** Non-fatal non-specific error   */
  VXIjsi_RESULT_NON_FATAL_ERROR    =    2,
  /** ECMAScript syntax error        */
  VXIjsi_RESULT_SYNTAX_ERROR       =   50,
  /** ECMAScript exception thrown    */
  VXIjsi_RESULT_SCRIPT_EXCEPTION   =   51,
  /** ECMAScript security violation  */
  VXIjsi_RESULT_SECURITY_VIOLATION =   52,
  /** Operation is not supported     */
  VXIjsi_RESULT_UNSUPPORTED        =  100
} VXIjsiResult;


/**
 * Attributes for scope name
 *
 */
typedef enum VXIjsiScopeAttr {
  /* The scope chain will be created as a real object */
  VXIjsi_NATIVE_SCOPE            =   1,
  /* Assign an alternative name to the currently active scope */
  VXIjsi_ALIAS_SCOPE             =   2
} VXIjsiScopeAttr;


/**
 * Abstract interface for interacting with a ECMAScript (JavaScript)
 * engine.  This provides functionality for creating ECMAScript
 * execution contexts, manipulating ECMAScript scopes, manipulating
 * variables within those scopes, and evaluating ECMAScript
 * expressions/scripts. <p>
 *
 * There is one ECMAScript interface per thread/line.
 */
typedef struct VXIjsiInterface {
  /**
   * Get the VXI interface version implemented
   *
   * @return  VXIint32 for the version number. The high high word is
   *          the major version number, the low word is the minor version
   *          number, using the native CPU/OS byte order. The current
   *          version is VXI_CURRENT_VERSION as defined in VXItypes.h.
   */
  VXIint32 (*GetVersion)(void);

  /**
   * Get the name of the implementation
   *
   * @return  Implementation defined string that must be different from
   *          all other implementations. The recommended name is one
   *          where the interface name is prefixed by the implementator's
   *          Internet address in reverse order, such as com.xyz.rec for
   *          VXIrec from xyz.com. This is similar to how VoiceXML 1.0
   *          recommends defining application specific error types.
   */
  const VXIchar* (*GetImplementationName)(void);

  /**
   * Create and initialize a new script context
   *
   * This creates a new context. Currently one context is created per
   * thread, but the implementation must support the ability to have
   * multiple contexts per thread.
   *
   * @param context  [OUT] Newly created context
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*CreateContext)(struct VXIjsiInterface  *pThis,
                                VXIjsiContext          **context);

  /**
   * Destroy a script context, clean up storage if required
   *
   * @param context  [IN] Context to destroy
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*DestroyContext)(struct VXIjsiInterface  *pThis,
                                 VXIjsiContext          **context);

  /**
   * Create a script variable relative to the current scope, initialized
   * to an expression
   *
   * NOTE: When there is an expression, the expression is evaluated,
   *  then the value of the evaluated expression (the final
   *  sub-expression) assigned. Thus an expression of "1; 2;" actually
   *  assigns 2 to the variable.
   *
   * @param context  [IN] ECMAScript context to create the variable within
   * @param name     [IN] Name of the variable to create
   * @param expr     [IN] Expression to set the initial value of the variable
   *                      (if NULL or empty the variable is set to ECMAScript
   *                      Undefined as required for VoiceXML 1.0 &lt;var&gt;)
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*CreateVarExpr)(struct VXIjsiInterface *pThis,
                                VXIjsiContext          *context,
                                const VXIchar          *name,
                                const VXIchar          *expr);

  /**
   * Create a script variable relative to the current scope, initialized
   *  to a VXIValue based value
   *
   * @param context  [IN] ECMAScript context to create the variable within
   * @param name     [IN] Name of the variable to create
   * @param value    [IN] VXIValue based value to set the initial value of
   *                      the variable (if NULL the variable is set to
   *                      ECMAScript Undefined as required for VoiceXML 1.0
   *                      &lt;var&gt;). VXIMap is used to pass ECMAScript objects.
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*CreateVarValue)(struct VXIjsiInterface *pThis,
                                 VXIjsiContext          *context,
                                 const VXIchar          *name,
                                 const VXIValue         *value);

  /**
   * Create a script variable to a DOMDocument relative to the current scope,
   * initialized to a DOMDocument.
   *
   * NOTE: Implementors must be aware of the exact contents of the VXIPtr
   *       passed by the interpreter.  i.e., If the interpreter uses
   *       a Xerces DOMDocument, so must the implementation of this
   *       interface.
   *
   * @param context  [IN] ECMAScript context to set the variable within
   * @param name     [IN] Name of the variable to set
   * @param doc      [IN] DOMDocument to be assigned.  The implementation
   *                      MUST NOT delete the contained document, as it
   *                      is owned by the interpreter.
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*CreateVarDOM)(struct VXIjsiInterface *pThis,
                             VXIjsiContext          *context,
                             const VXIchar          *name,
							 VXIPtr                 *doc );

  /**
   * Set a script variable to an expression relative to the current scope
   *
   * NOTE: The expression is evaluated, then the value of the
   *  evaluated expression (the final sub-expression) assigned. Thus
   *  an expression of "1; 2;" actually assigns 2 to the variable.
   *
   * @param context  [IN] ECMAScript context to set the variable within
   * @param name     [IN] Name of the variable to set
   * @param expr     [IN] Expression to be assigned
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*SetVarExpr)(struct VXIjsiInterface *pThis,
                             VXIjsiContext          *context,
                             const VXIchar          *name,
                             const VXIchar          *expr);

  /**
    * set a script variable read-only to the current scope
    *  
    * @param context  [IN] ECMAScript context in which the variable
    *                      has been created
    * @param name     [IN] Name of the variable to set as read only.
    *
    * @return VXIjsi_RESULT_SUCCESS on success
    */
   VXIjsiResult (*SetReadOnly)(struct VXIjsiInterface *pThis,
                               VXIjsiContext          *context,
                               const VXIchar          *name);

  /**
   * Set a script variable to a value relative to the current scope
   *
   * @param context  [IN] ECMAScript context to set the variable within
   * @param name     [IN] Name of the variable to set
   * @param value    [IN] VXIValue based value to be assigned. VXIMap is
   *                      used to pass ECMAScript objects.
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*SetVarValue)(struct VXIjsiInterface *pThis,
                              VXIjsiContext          *context,
                              const VXIchar          *name,
                              const VXIValue         *value);

  /**
   * Get the value of a variable
   *
   * @param context  [IN]  ECMAScript context to get the variable from
   * @param name     [IN]  Name of the variable to get
   * @param value    [OUT] Value of the variable, returned as the VXI
   *                       type that most closely matches the variable's
   *                       ECMAScript type. This function allocates this
   *                       for return on success (returns a NULL pointer
   *                       otherwise), the caller is responsible for
   *                       destroying it via VXIValueDestroy( ). VXIMap
   *                       is used to return ECMAScript objects.
   *
   * @return VXIjsi_RESULT_SUCCESS on success, VXIjsi_RESULT_FAILURE if the
   *         variable has a ECMAScript value of Null,
   *         VXIjsi_RESULT_NON_FATAL_ERROR if the variable is not
   *         defined (ECMAScript Undefined), or another error code for
   *         severe errors
   */
  VXIjsiResult (*GetVar)(struct VXIjsiInterface  *pThis,
                         const VXIjsiContext     *context,
                         const VXIchar           *name,
                         VXIValue               **value);

  /**
   * Check whether a variable is defined (not ECMAScript Undefined)
   *
   * NOTE: A variable with a ECMAScript Null value is considered defined
   *
   * @param context  [IN]  ECMAScript context to check the variable in
   * @param name     [IN]  Name of the variable to check
   *
   * @return VXIjsi_RESULT_SUCCESS on success (variable is defined),
   *         VXIjsi_RESULT_FAILURE if the variable has value undefined,
   *         VXIjsi_RESULT_NON_FATAL_ERROR if the variable does not exist,
   *         or another error code for severe errors
   */
  VXIjsiResult (*CheckVar)(struct VXIjsiInterface *pThis,
                           const VXIjsiContext    *context,
                           const VXIchar          *name);

  /**
   * Execute a script, optionally returning any execution result
   *
   * @param context  [IN]  ECMAScript context to execute within
   * @param expr     [IN]  Buffer containing the script text
   * @param value    [OUT] Result of the script execution, returned
   *                       as the VXI type that most closely matches
   *                       the variable's ECMAScript type. Pass NULL
   *                       if the result is not desired. Otherwise
   *                       this function allocates this for return on
   *                       success when there is a return value (returns
   *                       a NULL pointer otherwise), the caller is
   *                       responsible for destroying it via
   *                       VXIValueDestroy( ). VXIMap is used to return
   *                       ECMAScript objects.
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*Eval)(struct VXIjsiInterface  *pThis,
                       VXIjsiContext           *context,
                       const VXIchar           *expr,
                       VXIValue               **result);

  /**
   * Push a new context onto the scope chain (add a nested scope)
   *
   * @param context  [IN] ECMAScript context to push the scope onto
   * @param name     [IN] Name of the scope, used to permit referencing
   *                      variables from an explicit scope within the
   *                      scope chain, such as "myscope.myvar" to access
   *                      "myvar" within a scope named "myscope"
   * @param attr     [IN] Attribute that defines how the scope will be pushed.
   *                      The possible values are: 
   *                      VXIjsi_NATIVE_SCOPE: create a real scope
   *                                           and link to the scope chain
   *                      VXIjsi_ALIAS_SCOPE: create an alias for the currently active
   *                                          scope. If PopScope() is called later it
   *                                          will be an NO-OP
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*PushScope)(struct VXIjsiInterface *pThis,
                            VXIjsiContext          *context,
                            const VXIchar          *name,
                            const VXIjsiScopeAttr   attr);
  
  /**
   * Pop a context from the scope chain (remove a nested scope)
   *
   * @param context  [IN] ECMAScript context to pop the scope from
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*PopScope)(struct VXIjsiInterface *pThis,
                           VXIjsiContext          *context);

  /**
   * Reset the scope chain to the global scope (pop all nested scopes)
   *
   * @param context  [IN] ECMAScript context to pop the scopes from
   *
   * @return VXIjsi_RESULT_SUCCESS on success
   */
  VXIjsiResult (*ClearScopes)(struct VXIjsiInterface *pThis,
                              VXIjsiContext          *context);


  /**
   * Returns a VXIMap containing info on the last error.
   * The returned map is only valid if a previous call resulted in
   * an error.  Additionally, the map may be destroyed when the
   * the next call happens, so shoul not be retained.<br>
   *
   * Keys:<br>
   *   lineNumber<br>
   *   message<br>
   *
   * @return VXIMap containing error information, or NULL.
   */
  const VXIMap *(*GetLastError)(struct VXIjsiInterface *pThis, VXIjsiContext *context);

} VXIjsiInterface;

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
