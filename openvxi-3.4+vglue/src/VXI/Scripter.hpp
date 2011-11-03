
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

#ifndef _EXE_CONT
#define _EXE_CONT

#include "VXIvalue.h"
#include <vector>

extern "C" struct VXIjsiInterface;
extern "C" struct VXIjsiContext;

#include <xercesc/dom/DOMDocument.hpp>

/**
 * ECMA Script evaluation engine.
 *
 * Using the VXIjsi interface, this layer is employed by the VoiceXML 
 * Interpreter to set and retrieve script variables and evaluate expressions.<p>
 *
 * All functions, except the destructor, may throw exceptions.  These fall
 * into two classes.   The interpreter events represent semantic errors
 * which could in principal be handled by the application.  And the severe
 * JavaScriptErrors indicate serious errors with the VXIjsi layer.<p>
 * 
 *   VXIException::InterpreterEvent(EV_ERROR_ECMASCRIPT)<br>
 *   VXIException::JavaScriptError();<br>
 */
class Scripter {

public:
  Scripter(VXIjsiInterface* jsiapi);
  ~Scripter();

  /** 
   * Determines whether or not the named variable is defined.
   * 
   * @return true if variable has been defined
   *         false otherwise
   */
  bool IsVarDefined(const vxistring & name);

  /** 
   * Determines whether or not the named variable is declared.
   * 
   * @return true if variable has been declared
   *         false otherwise
   */
  bool IsVarDeclared(const vxistring & name);

  /** 
   * Creates a new variablevariable to the indicated expression.
   */
  void MakeVar(const vxistring & name, const vxistring & expr);

  /** 
   * Creates a new variablevariable to the indicated expression.
   */
  void MakeVar(const vxistring & name, const VXIValue * value);

  /**
   * Sets an existing variable to the indicated DOMDocument.
   */
  void MakeVar(const vxistring & name, xercesc::DOMDocument *doc);

  /** 
   * Sets an existing variable to the indicated expression.
   */
  void SetVar(const vxistring & name, const vxistring & expr);

  /**
   * Sets a variable to read-only.
   */
  void SetVarReadOnly(const vxistring & name);

  /** 
   * Sets a variable to 'undefined'.
   */
  void ClearVar(const vxistring & name);

  /** 
   * Creates a new variable or sets an existing variable to the indicated
   * string.
   */
  void SetString(const vxistring & var, const vxistring & val);

  /** 
   * Creates a new variable or sets an existing variable to the indicated
   * value type.  This may be used for complex types such as Object.
   */
  void SetValue(const vxistring & var, const VXIValue * value);

  /** 
   * Retrieves a variable from ECMAScript.  
   * 
   * The conversion is as follows:<br>
   *   Undefined, Null       --> NULL pointer<br>
   *   String                --> VXIString<br>
   *   Boolean               --> VXIFloat or VXIInteger {true, false} -> {1, 0}<br>
   *   Number                --> VXIFloat or VXIDouble or VXIInteger<br>
   *   Object                --> VXIMap<br><br>
   * 
   * NOTE: The caller must free the resulting VXIValue.
   */
  VXIValue* GetValue(const vxistring & name) const;

  /** 
   * Pushes a new variable scope onto the stack with the given name.  New
   * variables will be defined in this scope, unless they are explicitly
   * referenced using <scope>.<variable_name> as in application.lastresult$.
   */
  void PushScope(const vxistring & name, bool alias = false);

  /** 
   * Pops the current scope, destroying it and any associated variables.
   */
  void PopScope();

  /** 
   * Checks the name of the current scope against the argument.
   * 
   * @return true if the current scope matches the requested name.
   *         false otherwise
   */
  bool CurrentScope(const vxistring & name) const;

  /** 
   * Evaluates an ECMAScript script.
   */
  void EvalScript(const vxistring & script);

  /** 
   * Evaluates an ECMAScript script, and converts the result to an integer.
   */
  VXIint EvalScriptToInt(const vxistring & script);

  /** 
   * Evaluates an ECMAScript script, and converts the result to a string.
   */
  void EvalScriptToString(const vxistring & script, vxistring & result);

  /** 
   * Evaluates an ECMAScript script, and converts the result to a string.
   * NOTE: The caller must free the resulting VXIValue.
   */
  VXIValue * EvalScriptToValue(const vxistring & script);

  /** 
   * Evaluates an expression as a boolean.
   */
  bool TestCondition(const vxistring & script);

private:
  void maybe_throw_js_error(int err, const VXIchar *script = NULL) const;

  Scripter(const Scripter &);                 // no implementation
  Scripter & operator=(const Scripter &);     // no implementation

private:
  typedef std::vector<vxistring> SCOPESTACK;
  SCOPESTACK scopeStack;

  VXIjsiInterface * jsi_api;
  VXIjsiContext * jsi_context;
};

#endif
