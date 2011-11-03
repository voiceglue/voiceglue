
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

/***********************************************************************
 *
 * This is largely a wrapper around the VXIjsi interface.  The limited
 * responsibilities of this layer include:
 *
 *   (1) storing the VXIjsiContext and current scope name;
 *   (2) generating error.semantic exceptions for ECMAScript failures;
 *   (3) converting vxistring's to const wchar_t *;
 *   (4) converting VXIValue types.
 *
 ***********************************************************************/

#include "Scripter.hpp"
#include "CommonExceptions.hpp"
#include "VXML.h"
#include "VXIjsi.h"
#include <sstream>

// This is a simple conversion tool from VXIString -> vxistring.
static vxistring toString(const VXIString * s)
{
  if (s == NULL) return L"";
  const VXIchar * temp = VXIStringCStr(s);
  if (temp == NULL) return L"";
  return temp;
}

/*****************************************************************************
 * Constructor/Destructor - here is where the VXIjsiContext is managed.
 *****************************************************************************/

Scripter::Scripter(VXIjsiInterface *jsiapi) : jsi_api(jsiapi)
{
  if (jsi_api == NULL)
    maybe_throw_js_error(VXIjsi_RESULT_INVALID_ARGUMENT);
  maybe_throw_js_error(jsi_api->CreateContext(jsi_api, &jsi_context));
}


Scripter::~Scripter()
{
  VXIjsiResult err = jsi_api->DestroyContext(jsi_api, &jsi_context);
}

/*****************************************************************************
 * Raise javascript exeception on error
 ****************************************************************************/
void Scripter::maybe_throw_js_error(int err, const VXIchar *script) const
{
  switch (err) {
  case VXIjsi_RESULT_SUCCESS:
    return;

  case VXIjsi_RESULT_FAILURE:           // Normal failure
  case VXIjsi_RESULT_NON_FATAL_ERROR:   // Non-fatal non-specific error
  case VXIjsi_RESULT_SYNTAX_ERROR:      // JavaScript syntax error
  case VXIjsi_RESULT_SCRIPT_EXCEPTION:  // JavaScript exception thrown
  case VXIjsi_RESULT_SECURITY_VIOLATION:// JavaScript security violation
  {
    vxistring msg;
    const VXIMap *error = jsi_api->GetLastError(jsi_api, jsi_context);
    if (error) {
      const VXIString *s = reinterpret_cast<const VXIString *>(VXIMapGetProperty(error, L"message"));
      if (s) msg = VXIStringCStr(s);
    }
    throw VXIException::InterpreterEvent(EV_ERROR_ECMASCRIPT, msg);
  }

  default:
    throw VXIException::JavaScriptError();
  }
}

/*****************************************************************************
 * Wrappers around basic JSI interface. VXI only call JavaScript through here.
 *****************************************************************************/
void Scripter::MakeVar(const vxistring & name, const vxistring & expr)
{
  if (expr.empty()) {
    EvalScript(L"var " + name);
    return;
  }

  VXIjsiResult err = jsi_api->CreateVarExpr(jsi_api, jsi_context,
                                            name.c_str(), expr.c_str());

  // This handles variables such as x.y which are invalid arguments.
  if (err == VXIjsi_RESULT_INVALID_ARGUMENT && !name.empty())
    err = VXIjsi_RESULT_FAILURE;

  maybe_throw_js_error(err);
}


void Scripter::MakeVar(const vxistring & name, const VXIValue * value)
{
  VXIjsiResult err = jsi_api->CreateVarValue(jsi_api, jsi_context,
                                             name.c_str(), value);

  // This handles variables such as x.y which are invalid arguments.
  if (err == VXIjsi_RESULT_INVALID_ARGUMENT && !name.empty())
    err = VXIjsi_RESULT_FAILURE;

  maybe_throw_js_error(err);
}

void Scripter::MakeVar(const vxistring & name, xercesc::DOMDocument *doc)
{
  VXIPtr *ptr = VXIPtrCreate(doc);
  VXIjsiResult err = jsi_api->CreateVarDOM(jsi_api, jsi_context,
                                         name.c_str(), ptr);
  VXIPtrDestroy(&ptr);
  maybe_throw_js_error(err);
}


void Scripter::SetVar(const vxistring & name, const vxistring & expr)
{
  VXIjsiResult err = jsi_api->SetVarExpr(jsi_api, jsi_context,
                                         name.c_str(), expr.c_str());
  maybe_throw_js_error(err, name.c_str());
}

void Scripter::SetVarReadOnly(const vxistring & name)
{
  VXIjsiResult err = jsi_api->SetReadOnly(jsi_api, jsi_context,
                                          name.c_str());
  maybe_throw_js_error(err);
}


void Scripter::ClearVar(const vxistring & name)
{
  VXIjsiResult err = jsi_api->SetVarExpr(jsi_api, jsi_context,
                                         name.c_str(), L"undefined");
  maybe_throw_js_error(err);
}


void Scripter::SetValue(const vxistring & var, const VXIValue * value)
{
  VXIjsiResult err = jsi_api->SetVarValue(jsi_api, jsi_context,
                                          var.c_str(), value);
  maybe_throw_js_error(err);
}


VXIValue * Scripter::GetValue(const vxistring & name) const
{
  VXIValue* val = NULL;
  VXIjsiResult err = jsi_api->GetVar(jsi_api, jsi_context, name.c_str(), &val);

  switch (err) {
  case VXIjsi_RESULT_FAILURE:         // ECMAScript type Null
  case VXIjsi_RESULT_NON_FATAL_ERROR: // ECMAScript type Undefined
  case VXIjsi_RESULT_SUCCESS:
    break;
  default:
    maybe_throw_js_error(err);
  }

  return val;
}


void Scripter::SetString(const vxistring & name, const vxistring & val)
{
  vxistring line = val;

  // Normalize whitespace to ' '
  for (unsigned int i = 0; i < line.length(); ++i) {
    VXIchar c = line[i];
    if (c == '\n' || c == '\r' || c == '\t') line[i] = ' ';
  }

  // Escape the ' characters.
  const VXIchar APOSTROPHE = '\'';
  const VXIchar * const SLASH = L"\\";

  vxistring::size_type pos = line.find(APOSTROPHE);
  if (pos != vxistring::npos) {
    unsigned int size = line.length();
    do {
      line.insert(pos, SLASH);    // Convert ' -> \'
      if (++pos == size) break;   // Stop if last character was replaced.
      ++size;                     // Update the size.
      pos = line.find(APOSTROPHE, pos + 1);
    } while (pos != vxistring::npos);
  }

  // Pass the expression to ECMAScript.
  line = name + L" = '" + line + L"';";
  VXIjsiResult err = jsi_api->Eval(jsi_api, jsi_context, line.c_str(), NULL);
  maybe_throw_js_error(err);
}


void Scripter::PushScope(const vxistring & name, bool alias)
{
  VXIjsiScopeAttr scopeAttr 
    = alias ? VXIjsi_ALIAS_SCOPE : VXIjsi_NATIVE_SCOPE;

  VXIjsiResult err = jsi_api->PushScope(jsi_api, jsi_context, name.c_str(),
                                        scopeAttr);
  if (err == VXIjsi_RESULT_SUCCESS)
    scopeStack.push_back(name);

  maybe_throw_js_error(err);
}


void Scripter::PopScope()
{
  VXIjsiResult err = jsi_api->PopScope(jsi_api, jsi_context);
  if (err == VXIjsi_RESULT_SUCCESS)
    scopeStack.pop_back();

  maybe_throw_js_error(err);
}


bool Scripter::CurrentScope(const vxistring & name) const
{
  return name == scopeStack.back();
}


bool Scripter::IsVarDefined(const vxistring & name)
{
  switch (jsi_api->CheckVar(jsi_api, jsi_context, name.c_str())) {
  case VXIjsi_RESULT_SUCCESS:
    return true;
  case VXIjsi_RESULT_NON_FATAL_ERROR:    
  case VXIjsi_RESULT_FAILURE:
    return false;
  default:
    throw VXIException::JavaScriptError();
  }

  // We should never get here.
  return false;
}


bool Scripter::IsVarDeclared(const vxistring & name)
{
  switch (jsi_api->CheckVar(jsi_api, jsi_context, name.c_str())) {
  case VXIjsi_RESULT_FAILURE:
  case VXIjsi_RESULT_SUCCESS:
    return true;
  case VXIjsi_RESULT_NON_FATAL_ERROR:    
    return false;
  default:
    throw VXIException::JavaScriptError();
  }

  // We should never get here.
  return false;
}


bool Scripter::TestCondition(const vxistring & script)
{
  return (EvalScriptToInt(script) != 0);
}


VXIint Scripter::EvalScriptToInt(const vxistring & expr)
{
  VXIValue* val = NULL;
  VXIjsiResult err = jsi_api->Eval(jsi_api, jsi_context, expr.c_str(), &val);

  VXIint result;

  if (err == VXIjsi_RESULT_SUCCESS) {
    switch (VXIValueGetType(val)) {
    case VALUE_STRING: {
      const VXIchar* s = 
            VXIStringCStr(reinterpret_cast<const VXIString*>(val));
      result = ((s != NULL && *s != L'\0') ? 1 : 0);      
    } break;
    case VALUE_BOOLEAN:
      result = VXIint(VXIBooleanValue(reinterpret_cast<const VXIBoolean*>(val)));
      break;
    case VALUE_INTEGER:
      result = VXIIntegerValue(reinterpret_cast<const VXIInteger*>(val));
      break;
    case VALUE_FLOAT: {
      // Convert float down to VXIint.
      // For GCC 3.2.2, convert float to int not correctly produce result
      // therefore we must cast to unsigned long first
#if defined(__GNUC__) && ((__GNUC__ >= 3) && (__GLIBCPP__ >= 20030205))
      result = VXIint(VXIulong(VXIFloatValue(reinterpret_cast<const VXIFloat*>(val))));
#else
      result = VXIint(VXIFloatValue(reinterpret_cast<const VXIFloat*>(val)));
#endif
    } break;
#if (VXI_CURRENT_VERSION >= 0x00030000)
    case VALUE_DOUBLE: {
      // Convert double down to VXIint.
      // For GCC 3.2.2, convert double to int not correctly produce result
      // therefore we must cast to unsigned long first
#if defined(__GNUC__) && ((__GNUC__ >= 3) && (__GLIBCPP__ >= 20030205))
      result = VXIint(VXIulong(VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val))));
#else
      result = VXIint(VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val)));
#endif
    } break;
  case VALUE_ULONG:
    result = VXIint(VXIULongValue(reinterpret_cast<const VXIULong*>(val)));
    break;
#endif    
    case VALUE_MAP: {
      // if the map is not empty, return 1
      result = 0;
      const VXIMap* temp = reinterpret_cast<const VXIMap*>(val);
      if( temp && (VXIMapNumProperties(temp) > 0) ) result = 1;
    } break;
    default:
      err = VXIjsi_RESULT_FAILURE;
      break;
    }
  }

  VXIValueDestroy(&val);
  maybe_throw_js_error(err);

  return result;
}


void Scripter::EvalScriptToString(const vxistring & expr, vxistring & result)
{
  VXIValue * val = NULL;
  VXIjsiResult err = jsi_api->Eval(jsi_api, jsi_context, expr.c_str(), &val);

  std::basic_ostringstream<wchar_t> os;

  if (err == VXIjsi_RESULT_SUCCESS) {
    switch (VXIValueGetType(val)) {
    case VALUE_BOOLEAN:
      if(VXIBooleanValue(reinterpret_cast<const VXIBoolean*>(val)) == TRUE)
        os << L"true";
      else
        os << L"false";
      break;
    case VALUE_INTEGER:
      os << VXIIntegerValue(reinterpret_cast<const VXIInteger*>(val));
      break;
    case VALUE_FLOAT:
      os << VXIFloatValue(reinterpret_cast<const VXIFloat*>(val));
      break;
#if (VXI_CURRENT_VERSION >= 0x00030000)
    case VALUE_DOUBLE:
      os << VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val));
      break;
    case VALUE_ULONG:
      os << VXIULongValue(reinterpret_cast<const VXIULong*>(val));
      break;
#endif      
    case VALUE_STRING:
      std::fixed(os);
      os << VXIStringCStr(reinterpret_cast<const VXIString *>(val));
      break;
    default:
      err = VXIjsi_RESULT_FAILURE;
      break;
    }
  }

  VXIValueDestroy(&val);
  result = os.str();

  maybe_throw_js_error(err);
}


void Scripter::EvalScript(const vxistring & expr)
{
  VXIjsiResult err = jsi_api->Eval(jsi_api, jsi_context, expr.c_str(), NULL);
  maybe_throw_js_error(err);
}


VXIValue * Scripter::EvalScriptToValue(const vxistring & expr)
{
  VXIValue* val = NULL;
  VXIjsiResult err = jsi_api->Eval(jsi_api, jsi_context, expr.c_str(), &val);
  if (err == VXIjsi_RESULT_FAILURE) return NULL;  // from ECMAScript 'null'
  maybe_throw_js_error(err);
  return val;
}

