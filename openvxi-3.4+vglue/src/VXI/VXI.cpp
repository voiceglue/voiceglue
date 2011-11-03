
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

#include "VXI.hpp"
#include "DocumentParser.hpp"
#include "SimpleLogger.hpp"
#include "ValueLogging.hpp"            // for VXIValue dumping
#include "Scripter.hpp"                // for use in ExecutionContext
#include "GrammarManager.hpp"          // for use in ExecutionContext
#include "Counters.hpp"                // for EventCounter
#include "PropertyList.hpp"            // for PropertyList
#include "PromptManager.hpp"           // for PromptManager
#include "AnswerParser.hpp"            // for AnswerParser
#include "VXML.h"                      // for node & attribute names
#include "VXIrec.h"                    // for parameters from rec answer
#include "VXIlog.h"                    // for event IDs
#include "VXIobject.h"                 // for Object type
#include "VXIinet.h"
#include "DialogEventCounter.hpp"
#include "TokenList.hpp"
#include "AccessControl.hpp"

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

#include <sstream>
#include <algorithm>                   // for sort, set_intersection, etc.
#include <xercesc/dom/DOMDocument.hpp>

static const wchar_t * const SCOPE_Session       = L"session";
static const wchar_t * const SCOPE_Defaults      = L"$_platDefaults_";
static const wchar_t * const SCOPE_Application   = L"application";
static const wchar_t * const SCOPE_Document      = L"document";
static const wchar_t * const SCOPE_Dialog        = L"dialog";
static const wchar_t * const SCOPE_Local         = L"local";
static const wchar_t * const SCOPE_Anonymous     = L"$_execAnonymous_";

static const wchar_t * const SESSION_INET        = L"com.vocalocity.inet";

static const wchar_t * const GENERIC_DEFAULTS    = L"*";

const int DEFAULT_MAX_EXE_STACK_DEPTH = 50;
const int DEFAULT_MAX_EVENT_RETHROWS  = 6;
const int DEFAULT_MAX_EVENT_COUNT     = 12;

// ------*---------*---------*---------*---------*---------*---------*---------
static vxistring toString(const VXIString * s)
{
  if (s == NULL) return L"";
  const VXIchar * temp = VXIStringCStr(s);
  if (temp == NULL) return L"";
  return temp;
}

static vxistring toString(const VXIchar * s)
{
  if (s == NULL) return L"";
  return s;
}

/*
 * Implementation of the PromptManager PromptTranslator interface.
 */
class VXIPromptTranslator : public PromptTranslator {
public:
  virtual VXIValue * EvaluateExpression(const vxistring & expression) {
    if (expression.empty()) return NULL;
    return jsi.EvalScriptToValue(expression); 
  }

  virtual void SetVariable(const vxistring & name, const vxistring & value) {
    if (name.empty()) return;
    jsi.MakeVar(name, (VXIValue*)NULL);
    jsi.SetString(name, value); 
  }

  virtual bool TestCondition(const vxistring & script) {
    return jsi.TestCondition(script);
  }

  virtual void EvalScript(const vxistring & script) {
    jsi.EvalScript(script);
  }

  VXIPromptTranslator(Scripter & j) : jsi(j) { }
  virtual ~VXIPromptTranslator() { }

private:
  Scripter & jsi;
};

/*
 * Implementation of the AnswerParser AnswerTranslator interface.
 */
class VXIAnswerTranslator : public AnswerTranslator {
public:
  virtual void EvaluateExpression(const vxistring & expression) {
    if (expression.empty()) return;
    jsi.EvalScript(expression); 
  }

  virtual void SetString(const vxistring & var, const vxistring & val) {
    jsi.SetString(var, val); 
  }

  VXIAnswerTranslator(Scripter & j) : jsi(j) { }
  virtual ~VXIAnswerTranslator() { }

private:
  Scripter & jsi;
};

// Thrown after a successful recognition.
//
class AnswerInformation {
public:
  VXMLElement element;
  VXMLElement dialog;

  AnswerInformation(VXMLElement & e, VXMLElement & d)
    : element(e), dialog(d)  { }
};

class JumpReturn {
public:
  JumpReturn() { }
};

class JumpItem {
public:
  VXMLElement item;
  JumpItem(const VXMLElement & i) : item(i) { }
};

class JumpDialog {
public:
  VXMLElement dialog;
  JumpDialog(const VXMLElement d) : dialog(d)  { }
};

class JumpDoc {
public:
  VXMLElement  defaults;
  VXMLDocument application;
  vxistring    applicationURI;
  VXMLDocument document;
  vxistring    documentURI;
  VXMLElement  documentDialog;
  bool         isSubdialog;
  bool         isSubmitElement;
  PropertyList properties;
  
  JumpDoc(const VXMLElement & def, const VXMLDocument & app,
          const vxistring & appURI, const VXMLDocument & doc, const vxistring & docURI,
          const VXMLElement & docdial, bool issub, bool issubmit, const PropertyList & p)
    : defaults(def), application(app), applicationURI(appURI),
      document(doc), documentURI(docURI), documentDialog(docdial), isSubdialog(issub), 
      isSubmitElement(issubmit), properties(p)
  { }

  ~JumpDoc() { }
};

//#############################################################################
// ExecutionContext & Utilities
//#############################################################################

class ExecutionContext {
public:
  // These are used by the main run loops, event handling, and subdialog.
  // Each of these gets initialized by InstallDocument.

  VXMLElement    platDefaults;

  VXMLDocument   application;
  vxistring      applicationName;
  vxistring      applicationURI;
  VXIMapHolder   appProperties;
  
  VXMLDocument   document;
  vxistring      documentName;
  vxistring      documentURI;

  VXMLElement    currentDialog;
  VXMLElement    currentFormItem;

  // Limited purpose members.
  VXMLElement    eventSource;        // For prompting (enumerate) during events
  VXMLElement    lastItem;           // Set by <subdialog>

public: // These are used more generally.
  Scripter       script;
  GrammarManager gm;
  PromptTracker  promptcounts;
  DialogEventCounter eventcounts;
  PropertyList   properties;

  TokenList formitems;

  bool playingPrompts;

  ExecutionContext * next;

  ExecutionContext(VXIrecInterface * r, VXIjsiInterface * j,
                   const SimpleLogger & l, ExecutionContext * n)
    : script(j), gm(r, l), next(n), playingPrompts(true), properties(l) { }
    // may throw VXIException::OutOfMemory()

  ~ExecutionContext() { }
};

//#############################################################################
// Creation and Run
//#############################################################################

VXI::VXI()
  : parser(NULL), log(NULL), inet(NULL), rec(NULL), jsi(NULL), tel(NULL),
    exe(NULL), stackDepth(0), sdParams(NULL), sdResult(NULL), sdEvent(NULL),
    updateDefaultDoc(true), mutex(), uriPlatDefaults(), uriBeep(), uriCurrent(),
    lineHungUp(false), stopRequested(false), haveExternalEvents(false),defAccessControl(true)
{
  try {
    parser = new DocumentParser();
  }
  catch (const VXIException::OutOfMemory &) {
    parser = NULL;
    throw;
  }

  try {
    pm = new PromptManager();
  }
  catch (...) {
    delete parser;
    parser = NULL;
    pm = NULL;
    throw;
  }

  if (pm == NULL) {
    delete parser;
    parser = NULL;
    throw VXIException::OutOfMemory();
  }
}


VXI::~VXI()
{
  while (exe != NULL) {
    ExecutionContext * temp = exe->next;
    delete exe;
    exe = temp;
  }

  delete pm;
  delete parser;
}

void VXI::hangup_log()
{
    if (voiceglue_loglevel() >= LOG_DEBUG)
    {
	std::ostringstream logstring;
	logstring << "Detected line hangup";
	voiceglue_log ((char) LOG_DEBUG, logstring);
    };
}

// Determine if the user has already hung-up
// if it is the case, throw the exit element to indicate
// end of call
void VXI::CheckLineStatus()
{
  VXItelStatus status;
  int telResult = tel->GetStatus(tel, &status);

  bool needToThrow = false;
  if (telResult != VXItel_RESULT_SUCCESS) {
    needToThrow = true;
    log->LogError(201);
  }
  else if (status == VXItel_STATUS_INACTIVE)
    needToThrow = true;

  if (needToThrow) {
    hangup_log();
    mutex.Lock();
    bool alreadyHungup = lineHungUp;
    lineHungUp = true;
    mutex.Unlock();
    if (alreadyHungup) {
      log->LogDiagnostic(1, L"VXI::CheckLineStatus - Call has been hung-up, "
                         L"exiting...");
      throw VXIException::Exit(NULL);
    }
    else
      throw VXIException::InterpreterEvent(EV_TELEPHONE_HANGUP);
  }
}


bool VXI::SetRuntimeProperty(PropertyID id, const VXIValue * value)
{
  bool set = false;

  mutex.Lock();

  switch (id) {
  case VXI::BeepURI:
    if (VXIValueGetType(value) == VALUE_STRING) {
      const VXIchar * uri = VXIStringCStr(reinterpret_cast<const VXIString *>(value));
      if (uri == NULL) uriBeep.erase();
      else uriBeep = uri;
	  set = true;
    }
    break;

  case VXI::PlatDefaultsURI:
    if (VXIValueGetType(value) == VALUE_STRING) {
      const VXIchar * uri = VXIStringCStr(reinterpret_cast<const VXIString *>(value));
      if (uri == NULL) uriPlatDefaults.erase();
      else uriPlatDefaults = uri;
      updateDefaultDoc = true;
	  set = true;
    }
    break;

  case VXI::DefaultAccessControl:
    if (VXIValueGetType(value) == VALUE_INTEGER) {
      int def = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(value));
	  defAccessControl = def ? true : false;
      set = true;
    }
	break;
  }

  mutex.Unlock();
  return set;
}


void VXI::GetRuntimeProperty(PropertyID id, vxistring & value) const
{
  mutex.Lock();

  switch (id) {
  case VXI::BeepURI:           value = uriBeep;          break;
  case VXI::PlatDefaultsURI:   value = uriPlatDefaults;  break;
  }

  mutex.Unlock();
}

void VXI::GetRuntimeProperty(PropertyID id, int &value) const
{
  mutex.Lock();

  switch (id) {
  case VXI::DefaultAccessControl: value = defAccessControl; break;
  }

  mutex.Unlock();
}

void VXI::DeclareStopRequest(bool doStop)
{
  mutex.Lock();
  stopRequested = doStop;
  mutex.Unlock();
}

bool VXI::DeclareExternalEvent(const VXIchar * event, const VXIchar * message)
{
  // Check event
  if (event == NULL || *event == L'\0' || *event == L'.') return 1;
  bool lastWasPeriod = false;

  // This could be stricter
  for (const VXIchar * i = event; *i != L'\0'; ++i) {
    VXIchar c = *i;
    if (c == L'.') {
      if (lastWasPeriod) 
		  return false;
      lastWasPeriod = true;
    }
	else {
      if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r') 
		  return false;
      lastWasPeriod = false;
    }
  }

  mutex.Lock();
  externalEvents.push_back(event);
  if (message == NULL || message == L'\0')
    externalMessages.push_back(event);
  else
    externalMessages.push_back(message);
  haveExternalEvents = true;
  mutex.Unlock();

  return true;
}

bool VXI::ClearExternalEventQueue()
{
  mutex.Lock();
  if( !externalEvents.empty() )
  {
    haveExternalEvents = false;
    externalEvents.clear();
    externalMessages.clear();          
  }
  mutex.Unlock();
  return true;
}

int VXI::Run(const VXIchar * initialDocument,
             const VXIchar * sessionScript,
             SimpleLogger * resourceLog,
             VXIinetInterface * resourceInet,
             VXIcacheInterface * resourceCache,
             VXIjsiInterface * resourceJsi,
             VXIrecInterface * resourceRec,
             VXIpromptInterface * resourcePrompt,
             VXItelInterface * resourceTel,
             VXIobjectInterface * resourceObject,
             VXIValue ** resultValue)
{
  // (1) Check arguments.

  // (1.1) Check external resources
  if (resourceLog == NULL || resourceInet   == NULL || resourceJsi == NULL ||
      resourceRec == NULL || resourcePrompt == NULL || resourceTel == NULL)
    return 1;

  log = resourceLog;
  inet = resourceInet;
  jsi = resourceJsi;
  rec = resourceRec;
  tel = resourceTel;

  if (stopRequested) {
    DeclareStopRequest(false);
    if (log->IsLogging(2)) {
      log->StartDiagnostic(2) << L"VXI::Run( STOP HAS BEEN REQUESTED ) exiting..."; 
      log->EndDiagnostic();
    }
    return 4;
  }

  // These may be NULL.
  object = resourceObject;
  cache  = resourceCache;

  if (!pm->ConnectResources(log, resourcePrompt, this)) return 1;
    
  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::Run(" << initialDocument << L")";
    log->EndDiagnostic();
  }

  // (1.2) Check document
  if (initialDocument == NULL || wcslen(initialDocument) == 0) {
    log->LogError(201);
    return 1;
  }

  // (2) Delegate real work to RunDocumentLoop & handle the serious errors.
  int exitCode;
  try {
    exitCode = RunDocumentLoop(initialDocument, 
                              (sessionScript ? sessionScript : L""), 
                              resultValue);
    pm->PlayAll();
  }
  catch (VXIException::InterpreterEvent & e) {
    log->LogError(207, SimpleLogger::EXCEPTION, e.GetValue().c_str());
    exitCode = 0;
  }
  catch (const VXIException::Exit & e) {
    *resultValue = e.exprResult;
    exitCode = 0;
  }
  catch (const VXIException::Fatal &) {
    log->LogError(209);
    exitCode = -2;
  }
  catch (const VXIException::OutOfMemory &) {
    PopExecutionContext();
    log->LogError(202);
    exitCode = 1;
  }
  catch (const VXIException::JavaScriptError &) {
    log->LogError(212);
    exitCode = -2;
  }
  catch (const VXIException::StopRequest &) {
    exitCode = 4;
  }
  catch (const JumpDialog &) {
    log->LogError(999, SimpleLogger::MESSAGE, L"unexpected jump to a dialog");
    exitCode = -2;
  }
  catch (const JumpDoc &) {
    log->LogError(999, SimpleLogger::MESSAGE,L"unexpected jump to a document");
    exitCode = -2;
  }
  catch (const JumpItem &) {
    log->LogError(999, SimpleLogger::MESSAGE, L"unexpected jump to an item");
    exitCode = -2;
  }
  catch (const JumpReturn &) {
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"unexpected jump from a return element");
    exitCode = -2;
  }

  // Clean up execution contexts.
  try {
    while (exe != NULL)
      PopExecutionContext();
  }
  catch (const VXIException::JavaScriptError &) {
    log->LogError(212);
    exitCode = -2;
  }

  // Clean up event flags.
  lineHungUp = false;
  stopRequested = false;
  //haveExternalEvents = false;
  //externalEvents.clear();
  //externalMessages.clear();

  return exitCode;
}


int VXI::RunDocumentLoop(const vxistring & initialDocument,
                         const vxistring & sessionScript,
                         VXIValue ** resultValue)
{
  // (1) Load the document containing the default handlers.
  if (updateDefaultDoc) {
    log->LogDiagnostic(2, L"VXI::RunDocumentLoop - loading defaultDoc.");

    // (1.1) Get URI if possible.
    vxistring defaultsUri;
    GetRuntimeProperty(VXI::PlatDefaultsURI, defaultsUri);
  
    // (1.2) Load platform defaults.
    try {
      VXIMapHolder domDefaultProp;

      if (log->IsLogging(4) && !defaultsUri.empty()) {
        log->StartDiagnostic(4) << L"VXI: Loading platform defaults <"
                                << defaultsUri << L">";
        log->EndDiagnostic();
      }

      AttemptDocumentLoad(defaultsUri, VXIMapHolder(NULL), domDefaultDoc,
                          domDefaultProp, true);
    }
    catch (const VXIException::InterpreterEvent & e) {
      log->LogError(221, SimpleLogger::EXCEPTION, e.GetValue().c_str());
      return 3;
    }
    
    // Only reset this flag if the load has succeeded
    updateDefaultDoc = false;
  }

  // (2) Create a new execution context and initialize the defaults.

  // (2.1) Create new execution context.
  if (!PushExecutionContext(sessionScript)) return -1;

  // (3) Execute the document.
  bool firstTime = true;
  VXIint32 loopCount = 0;
  while (1) {
    if (stopRequested) throw VXIException::StopRequest();

    try {
      if (firstTime) {
        firstTime = false;
        // Jump to the initial document.  Any events which occur while
        // loading the initial document are handled by the defaults.
        log->LogDiagnostic(2, L"VXI::RunDocumentLoop - loading initial "
                           L"document.");

        try {
          PerformTransition(exe->platDefaults, initialDocument);
        }
        catch (const VXIException::InterpreterEvent & e) {
          DoEvent(exe->platDefaults, e);
          PopExecutionContext();
          throw VXIException::Exit(NULL);
        }
      }

      log->LogDiagnostic(2, L"VXI::RunDocumentLoop - new document");

      RunInnerLoop();
      break;
    }
    // Execute a new document
    catch (JumpDoc & e) {
      // if <subdialog>, we need to create a new context, while
      // storing the current.
      if (e.isSubdialog)
        if (!PushExecutionContext(sessionScript)) return -1;

      if (log->IsLogging(4)) {
        if (e.isSubdialog)
          log->StartDiagnostic(4) << L"VXI: Subdialog transition <"
                                  << e.documentURI << L">";
        else
          log->StartDiagnostic(4) << L"VXI: Document transition <"
                                  << e.documentURI << L">";
        log->EndDiagnostic();
      }

      InstallDocument(e);
    }
    // Handle <return>
    catch (JumpReturn &) {
      // clean up the current context, and restore the original.
      PopExecutionContext();
      if (log->IsLogging(4)) {
        log->StartDiagnostic(4) << L"VXI: Subdialog returned to <"
                                  << exe->documentURI << L">";
        log->EndDiagnostic();
      }
    }
    // Handle <exit> by exiting the document loop.
    catch (const VXIException::Exit & e) {
      if (resultValue != NULL) *resultValue = e.exprResult;
      break;
    }
  }

  // Done with document execution...clean up.
  PopExecutionContext();
  return 0;
}


void VXI::InstallDocument(JumpDoc & e)
{
  // (1) Check to see what needs to be initialized.
  //  If the transition is from leaf-to-root occurs in <submit> then
  //  the root context is initialized, otherwise it is preserved.
  bool leafToRoot = !e.isSubmitElement && (!e.documentURI.empty()) 
                    && (exe->applicationURI == e.documentURI);
  bool reinitApplication
    = e.isSubdialog ||
      (e.applicationURI.empty() && exe->applicationURI.empty()) ||
      (!leafToRoot && (e.applicationURI != exe->applicationURI));

  // (2) Set the easy stuff.
  exe->platDefaults   = e.defaults;
  exe->application    = e.application;
  exe->applicationURI = e.applicationURI;
  exe->document       = e.document;
  exe->documentURI    = e.documentURI;
  exe->currentDialog  = e.documentDialog;
  exe->currentFormItem  = VXMLElement();
  exe->eventSource    = VXMLElement();
  exe->properties     = e.properties;

  exe->gm.ReleaseGrammars();

  VXMLElement documentRoot = exe->document.GetRoot();
  VXMLElement applicationRoot = exe->application.GetRoot();

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::InstallDocument(documentURI:[" << e.documentURI
                            << L"], appURI:[" << e.applicationURI << L"])";
    log->EndDiagnostic();
  }
  
  // (3) Load grammars.  The grammars are reloaded using the current
  // understanding of the properties.  During activation, the actual
  // properties may differ slightly.

  VXIPromptTranslator translator(exe->script);
  try {
    exe->documentName = L"";
    exe->gm.LoadGrammars(documentRoot, exe->documentName, exe->properties);
    pm->PreloadPrompts(documentRoot, exe->properties, translator);
  }
  catch (const VXIException::InterpreterEvent & e) {
    DoEvent(documentRoot, e);
    throw VXIException::Exit(NULL);
  }

  try {
    vxistring temp;
    exe->gm.LoadGrammars(applicationRoot, temp, exe->properties);
    // don't want to preload prompts if the  root application already loaded
    if( reinitApplication )
      pm->PreloadPrompts(applicationRoot, exe->properties, translator);
  }
  catch (const VXIException::InterpreterEvent & e) {
    DoEvent(applicationRoot, e);
    throw VXIException::Exit(NULL);
  }

  try {
    vxistring temp;
    exe->gm.LoadGrammars(exe->platDefaults, temp, exe->properties, true);
    pm->PreloadPrompts(exe->platDefaults, exe->properties, translator);
  }
  catch (const VXIException::InterpreterEvent & e) {
    DoEvent(exe->platDefaults, e);
    throw VXIException::Exit(NULL);
  }

  // (4) Clear existing ECMA script scopes.

  Scripter & script = exe->script;

  if (script.CurrentScope(SCOPE_Anonymous)) script.PopScope();
  if (script.CurrentScope(SCOPE_Local))     script.PopScope();
  if (script.CurrentScope(SCOPE_Dialog))    script.PopScope();
  if (script.CurrentScope(SCOPE_Document))  script.PopScope();

  if (script.CurrentScope(SCOPE_Application) && reinitApplication)
    script.PopScope();

  if (reinitApplication && !script.CurrentScope(SCOPE_Defaults)) {
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"ECMAScript scope inconsistent");
    throw VXIException::Fatal();
  }

  // (5) And set the new ones.
  try {
    if (script.CurrentScope(SCOPE_Defaults)) {
      script.PushScope(SCOPE_Application);
      exe->script.MakeVar(L"lastresult$", L"undefined");
      if (!exe->applicationURI.empty()) {
        ProcessRootScripts(applicationRoot);
      }
    }
  }
  catch (const VXIException::InterpreterEvent & e) {
    DoEvent(applicationRoot, e);
    throw VXIException::Exit(NULL);
  }

  try {
    script.PushScope(SCOPE_Document, leafToRoot || exe->applicationURI.empty());
    if (!leafToRoot) {
      ProcessRootScripts(documentRoot);
    }
  }
  catch (const VXIException::InterpreterEvent & e) {
    DoEvent(documentRoot, e);
    throw VXIException::Exit(NULL);
  }
}


void VXI::PerformTransition(const VXMLElement & elem,
                            const vxistring & rawURI,
                            VXIMap * rawSubmitData,
                            bool isSubdialog,
                            bool isSubmitElement)
{
  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::PerformTransition(" << rawURI << L")";
    log->EndDiagnostic();
  }

  VXIMapHolder submitData(rawSubmitData);

  // (1) Determine fetch properties for document load.

  // (1.1) Create empty fetch object.  Now we need to fill this in.
  VXIMapHolder fetchobj;
  if (fetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();
   
  // (1.2) Set URIs for the Jump.
  vxistring uri((rawURI.empty() ? L"" : rawURI));
  vxistring fragment;
  const VXIchar * tempStr = L"";

  // (1.2.1) Divide raw URI into uri + fragment.
  exe->properties.GetFetchobjURIs(elem, fetchobj, uri, fragment);

  // (1.2.2) Handle the degenerate case.
  if (uri.empty() && fragment.empty()) {
    log->StartDiagnostic(0) << L"VXI::PerformTransition - invalid URI, \""
                            << (rawURI.empty() ? L"NULL" : rawURI.c_str()) << L"\"";
    log->EndDiagnostic();
    throw VXIException::InterpreterEvent(EV_ERROR_BADURI);
  }

  // (1.2.3) In the fragment only case, just go to the indicated item.
  if (uri.empty()) {
    VXMLElement targetElement = FindDialog(elem, fragment);
    if (targetElement == 0) {
      log->StartDiagnostic(0) << L"VXI::PerformTransition - non-existent "
                                 L"dialog, \"" << (rawURI.empty() ? L"NULL" : rawURI.c_str()) << L"\"";
      log->EndDiagnostic();
      throw VXIException::InterpreterEvent(EV_ERROR_BADDIALOG);
    }

    if (!isSubdialog)
      throw JumpDialog(targetElement);
    else {
      PropertyList subprop(exe->properties);
      subprop.PopPropertyLevel(FIELD_PROP);
      vxistring absoluteURI(L"");
      tempStr = exe->properties.GetProperty(PropertyList::AbsoluteURI);
      if (tempStr) absoluteURI = tempStr;
      
      throw JumpDoc(exe->platDefaults, exe->application, exe->applicationURI,
                    exe->document, absoluteURI, targetElement, isSubdialog, isSubmitElement,
                    subprop);
    }
  }

  // (1.3) Get remaining fetch properties. At this point, we now need
  //       to assemble seperate fetch properties for fetchaudio, up
  //       until now all the prep work is identical.
  VXIMapHolder fetchAudioFetchobj(NULL);
  fetchAudioFetchobj = fetchobj;
  if (fetchAudioFetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();

  exe->properties.GetFetchobjCacheAttrs(elem, PropertyList::Document,fetchobj);
  exe->properties.GetFetchobjCacheAttrs(elem, PropertyList::Audio,
                                        fetchAudioFetchobj);

  if (submitData.GetValue() != NULL) {
    if (!exe->properties.GetFetchobjSubmitAttributes(elem, submitData,
                                                     fetchobj))
    {
      // This should never occur.
      log->StartDiagnostic(0) << L"VXI::PerformTransition - couldn't set "
                                 L"the submit attributes.";
      log->EndDiagnostic();
      throw VXIException::InterpreterEvent(EV_ERROR_BADURI);
    }
    submitData.Release(); // This map is now owned by fetchobj.
  }

  // (2) Load Document.

  // (2.1) Start fetch audio.

  vxistring fetchaudio;
  if (!elem.GetAttribute(ATTRIBUTE_FETCHAUDIO, fetchaudio))
    fetchaudio = toString(exe->properties.GetProperty(L"fetchaudio"));

  if (!fetchaudio.empty()) {
    // Get All properties for fetching audio
    VXIMapHolder fillerProp(VXIMapClone(fetchAudioFetchobj.GetValue()));
    exe->properties.GetProperties(fillerProp);

    pm->PlayAll(); // Finish playing already queued prompts
    pm->PlayFiller(fetchaudio, fillerProp);
  }

  // (2.2) Load document and verify that dialog exists.

  // (2.2.1) Load the VoiceXML document.
  VXMLDocument document;
  VXMLElement documentDialog;
  VXIMapHolder documentFetchProps;
  AttemptDocumentLoad(uri, fetchobj, document, documentFetchProps);

  // (2.2.2) If there was a fragment, does the named dialog exist?
  VXMLElement documentRoot = document.GetRoot();
  documentDialog = FindDialog(documentRoot, fragment);
  if (documentDialog == 0) {
    vxistring msg;
    if (fragment.empty())
      msg = L"no dialog element found in " + uri;
    else
      msg = L"named dialog " + fragment + L" not found in " + uri;
    log->StartDiagnostic(0) << L"VXI::PerformTransition - " << msg;
    log->EndDiagnostic();
    throw VXIException::InterpreterEvent(EV_ERROR_BADDIALOG, msg);
  }
   
  // (3) Get Document language & find associated defaults.

  // (3.1) Create a new property list containing the document properties.
  PropertyList newProperties(*log);

// WARNING: The defaults document has multiple languages and a language-independent '*' group.
// This should really read the '*' group and then overlay the set specific to the active language.

  // (3.2) Extract the language setting.
  const VXIchar * language = newProperties.GetProperty(PropertyList::Language);
  if (language == NULL) language = GENERIC_DEFAULTS;

  // (3.3) Find the language defaults.
  VXMLElement defaults;

  VXMLElement defaultsRoot = domDefaultDoc.GetRoot();
  for (VXMLNodeIterator it(defaultsRoot); it; ++it) {
    VXMLNode child = *it;

    // Look for a language node.
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    if (elem.GetName() != DEFAULTS_LANGUAGE) continue;

    vxistring id;
    elem.GetAttribute(ATTRIBUTE_ID, id);
    if (id == language || (id == GENERIC_DEFAULTS && defaults == 0))
      defaults = elem;
  }
  if (defaults != 0)
    newProperties.SetProperties(defaults, DEFAULTS_PROP, VXIMapHolder(NULL));

  // newProperties should now contain the properties from the platform defaults plus
  // the properties at document level.
  newProperties.SetProperties(documentRoot, DOC_PROP, documentFetchProps);

  // (3.4) Copy the document properties in and find the absolute URI. 
  vxistring absoluteURI(L"");
  vxistring applicationURI(L"");

  tempStr = newProperties.GetProperty(PropertyList::AbsoluteURI, DOC_PROP);
  if (tempStr == NULL) 
    absoluteURI = uri.empty() ? L"" : uri.c_str();
  else
    absoluteURI = tempStr;

  // (4) Load the application.
  VXMLDocument application;

  // (4.1) Get the application URI.

  vxistring appuri;
  documentRoot.GetAttribute(ATTRIBUTE_APPLICATION, appuri);
  if (!appuri.empty() ) {
    if ( (exe->applicationName.empty() || appuri != exe->applicationName)) {
      // set applcation name
      exe->applicationName = appuri;      
      // (4.2) Initialize application fetch parameters.
      VXIMapHolder appFetchobj;
      if (appFetchobj.GetValue() == NULL)
        throw VXIException::OutOfMemory();

      vxistring appFragment;
      newProperties.GetFetchobjURIs(documentRoot, appFetchobj, appuri,
                                  appFragment);
      if (appuri.empty()) {
        log->LogError(214);
        throw VXIException::InterpreterEvent(EV_ERROR_APP_BADURI);
      }
      newProperties.GetFetchobjCacheAttrs(documentRoot, PropertyList::Document,
                                        appFetchobj);

      // (4.3) Load the application and its properties (we must then restore 
      // document properties) also let document converter 
      // know it's root application
      VXIMapClear(exe->appProperties.GetValue());      
      AttemptDocumentLoad(appuri, appFetchobj, application, exe->appProperties,
                          false, true);

      // (4.3.1) If an application root document specifies an application root
      //         element error.semantic is thrown.   
      VXMLElement documentRootCheck = application.GetRoot();

      vxistring appuriCheck;
      documentRootCheck.GetAttribute(ATTRIBUTE_APPLICATION, appuriCheck);
      if (!appuriCheck.empty())
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
            L"application root element exists in application root document");
      
      newProperties.SetProperties(application.GetRoot(), APP_PROP, exe->appProperties);

      // (4.4) Get the absolute URI for the application root. 
      // transitioning to new application
      tempStr = newProperties.GetProperty(PropertyList::AbsoluteURI, APP_PROP);
      applicationURI = (tempStr == NULL ? L"" : tempStr);

      // (4.4.1) Generate the final event.
      throw JumpDoc(defaults, application, applicationURI,
                  document, absoluteURI, documentDialog, isSubdialog,
                  isSubmitElement, newProperties);
    }
    else {
      // (4.5) leaf-to-leaf transition with same root application, and since
      // root application has already been loaded, we re-use the previous loaded
      // root document and uri
      newProperties.SetProperties(exe->application.GetRoot(), APP_PROP, exe->appProperties);
      // (4.5.1) Generate the final event.
      throw JumpDoc(defaults, exe->application, exe->applicationURI,
                  document, absoluteURI, documentDialog, isSubdialog,
                  isSubmitElement, newProperties);
    }
  }
  else {
    // (5) Transition to another app, i.e: root app. uri is not the same or empty then
    // we're no longer the same application, remove the application name
    if (!exe->applicationName.empty()) 
      exe->applicationName = L"";
    
    // (5.1) Generate the final event.
    throw JumpDoc(defaults, application, applicationURI,
                  document, absoluteURI, documentDialog, isSubdialog,
                  isSubmitElement, newProperties);
  }
}


// Finds the named dialog in the document.  If the name is empty, the first
// item is returned.
//
VXMLElement VXI::FindDialog(const VXMLElement & start, const vxistring & name)
{
  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::FindDialog(" << name << L")";
    log->EndDiagnostic();
  }

  // (0) find the root node.
  VXMLElement doc;
  for (doc = start; doc != 0 && doc.GetName() != NODE_VXML;
       doc = doc.GetParent());
  if (doc == 0) return VXMLElement();

  // (1) Walk through all elements at this level and find match.
  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    // (1.1) Only <form> & <menu> elements are considered.
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    VXMLElementType nodeName = elem.GetName();
    if (nodeName != NODE_FORM && nodeName != NODE_MENU) continue;

    // (1.2) If no dialog was specified, return the first one.
    if (name.empty()) return elem;

    // (1.3) Otherwise, look for an exact match.
    vxistring id;
    if (!elem.GetAttribute(ATTRIBUTE__ITEMNAME, id)) continue;

    if (name == id)
      return elem;
  }

  // (2) User attempted to GOTO to non-existant dialog or have an empty doc!
  log->LogDiagnostic(2, L"VXI::FindDialog - no match found.");

  return VXMLElement();
}


//#############################################################################
// Document Loop
//#############################################################################

// return success/failure (can't throw error here as caller needs
//  a chance to clean up
// Also initialize new context (session scope)
bool VXI::PushExecutionContext(const vxistring & sessionScript)
{
  log->LogDiagnostic(2, L"VXI::PushExecutionContext()");

  if (log->IsLogging(4)) {
    log->StartDiagnostic(4) << L"VXI::PushExecutionContext - stackDepth: "
                            << stackDepth << L", max: " << DEFAULT_MAX_EXE_STACK_DEPTH;
    log->EndDiagnostic();
  }

  // (1) Catch recursive <subdialog> case...
  if (stackDepth >= DEFAULT_MAX_EXE_STACK_DEPTH) {
    log->LogError(211);
    return false;
  }

  // (2) Create new execution context.
  ExecutionContext * ep = new ExecutionContext(rec, jsi, *log, exe);
  if (ep == NULL) throw VXIException::OutOfMemory();

  exe = ep;
  ++stackDepth;

  // (3) Init new context from channel (i.e. set up 'session' scope)
  exe->script.PushScope(SCOPE_Session);
  if (!sessionScript.empty()) exe->script.EvalScript(sessionScript);

  exe->script.SetVarReadOnly(SCOPE_Session);

  log->LogDiagnostic(2, L"VXI::PushExecutionContext - session variables "
                     L"initialized");

  // (4) Init new context defaults (i.e. set up platform defaults)

  // (4.1) Find generic language properties.
  VXMLElement defaultsRoot = domDefaultDoc.GetRoot();
  for (VXMLNodeIterator it(defaultsRoot); it; ++it) {
    VXMLNode child = *it;
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    if (elem.GetName() != DEFAULTS_LANGUAGE) continue;

    vxistring id;
    elem.GetAttribute(ATTRIBUTE_ID, id);
    if (id != GENERIC_DEFAULTS) continue;
    exe->platDefaults = elem;
    break;
  }

  if (exe->platDefaults == 0) {
    log->LogError(223);
    PopExecutionContext();
    return false;
  }

  // (4.2) Install defaults.  We only need to worry about the properties (for
  // document load) and the ECMA variables and scripts (for catch handlers on
  // load failure).  The grammars & prompts may be safely ignored.

  exe->properties.SetProperties(exe->platDefaults, DEFAULTS_PROP,
                                VXIMapHolder());
  exe->script.PushScope(SCOPE_Defaults);
  ProcessRootScripts(exe->platDefaults);

  log->LogDiagnostic(2, L"VXI::PushExecutionContext - platform defaults "
                     L"initialized");

  return true;
}


void VXI::PopExecutionContext()
{
  if (log->IsLogging(4)) {
    log->StartDiagnostic(4) << L"VXI::PopExecutionContext - current stackDepth: "
                            << stackDepth << L", max: " << DEFAULT_MAX_EXE_STACK_DEPTH;
    log->EndDiagnostic();
  }

  if (exe == NULL) return;
  ExecutionContext * current = exe;

  exe = current->next;
  --stackDepth;

  delete current;
}

void VXI::AttemptDocumentLoad(const vxistring & uri,
                              const VXIMapHolder & uriProperties,
                              VXMLDocument & doc,
                              VXIMapHolder & docProperties,
                              bool isDefaults,
                              bool isRootApp)
{
  // Reset current uri
  uriCurrent = uri;
  
  // (1) Create map to store document properties.
  if (docProperties.GetValue() == NULL) 
    throw VXIException::OutOfMemory();

  // (2) Fetch the document
  VXIbyte *cbuffer = NULL;
  VXIulong bufferSize = 0;
  int result = parser->FetchDocument(uri.c_str(), uriProperties, inet, cache,
                                     *log, doc, docProperties, 
                                     isDefaults, isRootApp,
                                     &cbuffer, &bufferSize);
  
  // If we didn't have a parse error, dump the VoiceXML document if
  // configured to do so, otherwise free the content buffer
  if ((result != 4) && (cbuffer)) {
    VXIString *key = NULL, *value = NULL;
    if ((log->IsLogging(SimpleLogger::DOC_DUMPS)) &&
        (log->LogContent(VXI_MIME_XML, cbuffer, bufferSize, &key, &value))) {

      vxistring diagUri(uri);
      const VXIValue * val = VXIMapGetProperty(docProperties.GetValue(),
                                               INET_INFO_ABSOLUTE_NAME);
      if (val != NULL && VXIValueGetType(val) == VALUE_STRING)
        diagUri = VXIStringCStr(reinterpret_cast<const VXIString *>(val));

      vxistring temp(L"URL");
      temp += L": ";
      temp += diagUri.c_str();
      temp += L", ";
      temp += VXIStringCStr(key);
      temp += L": ";
      temp += VXIStringCStr(value);
      log->LogDiagnostic(SimpleLogger::DOC_DUMPS, temp.c_str());
    }

      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "Released http_get buffer "
		    << Pointer_to_Std_String((const void *) cbuffer);
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };
    delete [] cbuffer;
    cbuffer = NULL;
    if (key) VXIStringDestroy(&key);
    if (value) VXIStringDestroy(&value);
  }
  
  // (3) Handle error conditions.
  switch (result) {
  case -1: // Out of memory
    throw VXIException::OutOfMemory();
  case 0:  // Success
    break;
  case 1: // Invalid parameter
  case 2: // Unable to open URI
    log->LogError(203);
    // now look at the http status code to throw the 
    // compiliant error.badfetch.http.<response code> 
    if (docProperties.GetValue()) {
      // retrieve response code
      VXIint iv = 0;
      const VXIValue *v = 
        VXIMapGetProperty(docProperties.GetValue(), INET_INFO_HTTP_STATUS);
      if( v && VXIValueGetType(v) == VALUE_INTEGER &&
          ((iv = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v))) > 0)
         )
      {
        std::basic_stringstream<VXIchar> respStream;
        respStream << EV_ERROR_BADFETCH << L".http.";
        respStream << iv;
        
        throw VXIException::InterpreterEvent(respStream.str().c_str(), uri);    
      }
    }                      
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);  
  case 3: // Unable to read from URI
    log->LogError(204);
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  case 4: // Unable to parse contents of URI
  { 
    VXIString *key = NULL, *value = NULL;
    if ((cbuffer) && 
        ((log->IsLogging(SimpleLogger::DOC_PARSE_ERROR_DUMPS)) ||
         (log->IsLogging(SimpleLogger::DOC_DUMPS))) &&
        (log->LogContent(VXI_MIME_XML, cbuffer, bufferSize, &key, &value)))
      log->LogError(205, VXIStringCStr(key), VXIStringCStr(value));
    else
      log->LogError(205);

    if (cbuffer)
    {
	if (voiceglue_loglevel() >= LOG_DEBUG)
	{
	    std::ostringstream logstring;
	    logstring << "Released http_get buffer "
		      << Pointer_to_Std_String((const void *) cbuffer);
	    voiceglue_log ((char) LOG_DEBUG, logstring);
	};
	delete [] cbuffer;
    };
    if (key) VXIStringDestroy(&key);
    if (value) VXIStringDestroy(&value);
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  }
  default:
    log->LogError(206);
    throw VXIException::Fatal();
  }

  if (doc.GetRoot() == 0) {
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"able to fetch initial document but node empty");
    throw VXIException::Fatal();
  }
}


void VXI::ProcessRootScripts(VXMLElement& doc)
{
  if (doc == 0) return;

  log->LogDiagnostic(2, L"VXI::ProcessRootScripts()");

  // Do <var> <script> and <meta> <property> elements
  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    VXMLElementType nodeName = elem.GetName();
    if (nodeName == NODE_VAR)
      var_element(elem);
    else if (nodeName == NODE_META)
      meta_element(elem);
    else if (nodeName == NODE_SCRIPT)
      script_element(elem);
    else if (nodeName == NODE_DATA)
      data_element(elem);
  }

  log->LogDiagnostic(2, L"VXI::ProcessRootScripts - done");
}


//#############################################################################
// Dialog Loop
//#############################################################################

// There are two cases in which this routine may be entered.
//   A. After a new document is loaded  (lastItem == 0)
//   B. After a return from <subdialog>
// and three ways in which the loop may be re-entered.
//   1. Jump to new dialog.
//   2. Jump to new form item in the existing dialog.
//   3. Jump to new dialog after recognizing a document scope grammar.
//
// The ECMA script scopes are reset accordingly:
//   anonymous: A B 1 2 3
//   local:     A   1 2 3
//   dialog:    A   1   3

void VXI::RunInnerLoop()
{
  log->LogDiagnostic(2, L"VXI::RunInnerLoop()");

  if (exe->currentDialog == 0) {
    log->LogError(999, SimpleLogger::MESSAGE, L"no current active document");
    return;
  }

  exe->currentFormItem = exe->lastItem; // next item to process; <goto> etc.
  exe->lastItem = VXMLElement();        // reset after <subdialog>
  bool newDialog = (exe->currentFormItem == 0);

  exe->playingPrompts = true;           // flag for prompting after events

  // Initialize flags
  bool unprocessedAnswer = false;

  // Run the dialog loop
  while (1) {
    if (stopRequested) throw VXIException::StopRequest();

    try {
      try {
        // (1) Adjust scope if necessary
        if (exe->script.CurrentScope(SCOPE_Anonymous)) exe->script.PopScope();
        if (exe->script.CurrentScope(SCOPE_Local))  exe->script.PopScope();

        // (2) Initialize dialog (if necessary)
        if (newDialog) {
          newDialog = false;
          exe->playingPrompts = true;

          // (2.1) Reset ECMA script scope.
          if (exe->script.CurrentScope(SCOPE_Dialog)) exe->script.PopScope();
          exe->script.PushScope(SCOPE_Dialog);

          // (2.2) Do 'initialization phase' from FIA.
          VXIMapHolder params(sdParams);
          sdParams = NULL;
          FormInit(exe->currentDialog, params);

          // (2.3) Do 'select phase' from FIA if the item is not already known
          if (exe->currentFormItem == 0) {
            DoInnerJump(exe->currentDialog, L"");
            break;
          }
        }

        // (3) The loop cases.

        // (3.1) Re-entering loop with an unprocessed recognition result.
        if (unprocessedAnswer == true) {
          unprocessedAnswer = false;
          HandleRemoteMatch(exe->currentDialog, exe->currentFormItem);
        }

        // (3.2) Re-entering loop after returning from a <subdialog>.
        else if (sdResult != NULL) {
          VXIMapHolder temp(sdResult);
          sdResult = NULL;
          ProcessReturn(exe->currentDialog, exe->currentFormItem, temp);
        }
        else if (sdEvent != NULL) {
          // The sdEvent is deallocated in the catch (below).
          throw VXIException::InterpreterEvent(*sdEvent);
        }

        // (3.3) Each time we enter collect phase, we get fresh local scope.
        // All filled and catched triggered form here will execute in this
        // scope.  The final local scope is popped when we leave.
        else {
          if (exe->script.CurrentScope(SCOPE_Local)) exe->script.PopScope();
          exe->script.PushScope(SCOPE_Local);

          mutex.Lock();
          if (haveExternalEvents) {
            vxistring event, message;
            if (externalEvents.size() > 0) {
              event = externalEvents.front();
              externalEvents.pop_front();
              message = externalMessages.front();
              externalMessages.pop_front();
            }
            if (externalEvents.empty()) haveExternalEvents = false;
            mutex.Unlock();
            throw VXIException::InterpreterEvent(event, message,
                                                 exe->currentDialog);
          }
          mutex.Unlock();
          
          // Do the 'collect phase & process phase' from the FIA.
          CollectPhase(exe->currentDialog, exe->currentFormItem);
        }
      }
      // Handles document events.
      catch (const VXIException::InterpreterEvent & e) {
        // Cleanup sdEvent (if necessary).
        if (sdEvent != NULL) {
          delete sdEvent;
          sdEvent = NULL;
        }
        
        if (log->IsLogging(2)) {
          log->StartDiagnostic(2) << L"VXI::RunInnerLoop - got exception: "
                                  << e.GetValue();
          log->EndDiagnostic();
        }

        if (exe->currentFormItem != 0)
          DoEvent(exe->currentFormItem, e);
        else
          DoEvent(exe->currentDialog, e);
      }

      DoInnerJump(exe->currentDialog, L"");
      break;
    }
    // Handles a goto a dialog in the same document.
    catch (JumpDialog & e) {
      exe->currentDialog = e.dialog;
      exe->currentFormItem = VXMLElement();
      newDialog = true;
    }
    // Handles <goto nextitem="...">.
    catch (const JumpItem & e) { 
      // sets the next form item to execute.
      exe->currentFormItem = e.item;
    }
    // Something down the call stack received a recognition.  it
	// will be handled on the next iteration of the loop.
    catch (AnswerInformation & e) {
      if (exe->currentDialog != e.dialog) {
        exe->currentDialog = e.dialog;
        newDialog = true;
      }
      exe->currentFormItem = e.element;
      unprocessedAnswer = true;
    }
  } // while (1)

  log->LogDiagnostic(2, L"VXI::RunInnerLoop - done");
}


void VXI::ProcessReturn(const VXMLElement& form, const VXMLElement & item,
                        VXIMapHolder & result)
{
  log->LogDiagnostic(2, L"VXI::ProcessReturn()");

  vxistring filled;
  item.GetAttribute(ATTRIBUTE__ITEMNAME, filled);

  exe->script.SetValue(filled, reinterpret_cast<VXIValue*>(result.GetValue()));
  ProcessFilledElements(filled, form);
}


// Perform initialization associated with property tags and form level
// variables.  Reset the event and prompts counts.
//
void VXI::FormInit(const VXMLElement & form, VXIMapHolder & params)
{
  log->LogDiagnostic(2, L"VXI::FormInit()");

  // (1) Set the form properties.
  exe->properties.SetProperties(form, DIALOG_PROP, VXIMapHolder());

  // (2) Clear the prompt & event counts when the form is entered.
  exe->promptcounts.Clear();
  exe->eventcounts.Clear();

  exe->formitems.clear();
  vxistring itemname;
  form.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);

  // (3) Walk through the form nodes.  Set up variables as necessary.
  for (VXMLNodeIterator it(form); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
 
    // (3.1) Handle <var> and <script> elements.
    try {
      VXMLElementType nodeName = elem.GetName();
      if (nodeName == NODE_VAR) {
        if (params.GetValue() != NULL) {
          // (3.1.1) Set matching variables to the value in the param list.
          // Each located parameter gets removed from the map.
          vxistring name;
          elem.GetAttribute(ATTRIBUTE_NAME, name);
          if (!name.empty()) {
            const VXIValue * value = VXIMapGetProperty(params.GetValue(),
                                                     name.c_str());
            if (value != NULL) {
              exe->script.MakeVar(name, value);
              VXIMapDeleteProperty(params.GetValue(), name.c_str());
              continue;
            }
          }
        }

        // (3.1.2) Otherwise, follow the normal proceedure.
        var_element(elem);
        continue;
      }
      else if (nodeName == NODE_SCRIPT)
        script_element(elem);
      else if (nodeName == NODE_DATA)
        data_element(elem);

      // (3.2) Ignore anything which is not a form item.
      if (!IsFormItemNode(elem)) continue;

      // (3.3) Initialize variables for each form item.
      vxistring name;
      vxistring expr;
      elem.GetAttribute(ATTRIBUTE__ITEMNAME, name);

      // (3.4) Throw on duplicate form item names
      if (exe->script.IsVarDeclared(name)) {
        vxistring msg = L"Duplicate form item name: " + name;
        log->LogError(217, SimpleLogger::MESSAGE, name.c_str());
        throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, msg);
      }

      elem.GetAttribute(ATTRIBUTE_EXPR, expr);
      exe->script.MakeVar(name, expr);

      nodeName = elem.GetName();
      if (nodeName == NODE_FIELD || nodeName == NODE_RECORD ||
          nodeName == NODE_TRANSFER || nodeName == NODE_INITIAL)
        exe->script.EvalScript(vxistring(L"var ") + name + L"$ = new Object();");

      exe->formitems.push_back(name);
    }
    catch( VXIException::InterpreterEvent &e ) {
      DoEvent(form, e);
      if (exe->script.CurrentScope(SCOPE_Anonymous)) 
        exe->script.PopScope();
    }
  }

  // (4) Did all incoming parameters get used?
  if (params.GetValue() != NULL && VXIMapNumProperties(params.GetValue()) != 0) {
    vxistring unused;
    const VXIchar *key;
    const VXIValue *value;
    VXIMapIterator *vi = VXIMapGetFirstProperty(params.GetValue(), &key,
                                                  &value);
    while (vi) {
      if (unused.empty()) unused = vxistring(L"Unused param(s): " ) + key;
	  else unused += L", " + vxistring(key);
      if (VXIMapGetNextProperty(vi, &key, &value) != VXIvalue_RESULT_SUCCESS){
        VXIMapIteratorDestroy(&vi);
        vi = NULL;
      }
    }

	log->LogError(218, SimpleLogger::MESSAGE, unused.c_str());
    throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
  }

  log->LogDiagnostic(2, L"VXI::FormInit - Done");
}


// Returns true if the element is a form item.
//
// This is the list from section 6.2 of the VXML 1.0 specification with one
// addition - <menu>.
//
bool VXI::IsFormItemNode(const VXMLElement & doc)
{
  VXMLElementType nodeName = doc.GetName();
  if (nodeName == NODE_FIELD  || nodeName == NODE_INITIAL   ||
      nodeName == NODE_RECORD || nodeName == NODE_TRANSFER  ||
      nodeName == NODE_OBJECT || nodeName == NODE_SUBDIALOG ||
      nodeName == NODE_MENU   || nodeName == NODE_BLOCK )
    return true;

  return false;
}


bool VXI::IsInputItemNode(const VXMLElement & doc)
{
  VXMLElementType nodeName = doc.GetName();
  if (nodeName == NODE_FIELD     || nodeName == NODE_RECORD ||
      nodeName == NODE_TRANSFER  || nodeName == NODE_OBJECT ||
      nodeName == NODE_SUBDIALOG || nodeName == NODE_MENU)
    return true;

  return false;
}


// Finds the named form item within the dialog.  If the name is empty, the
// first non-filled item is returned.
//
void VXI::DoInnerJump(const VXMLElement & elem, const vxistring & item)
{
  if (elem == 0) return;

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::DoInnerJump(" << item << L")";
    log->EndDiagnostic();
  }

  // find form.
  VXMLElement current = elem;

  while (1) {
    VXMLElementType nodeName = current.GetName();
    if (nodeName == NODE_MENU)
      throw JumpItem(current); // Menu is a special case.
    if (nodeName == NODE_FORM)
      break;

    const VXMLElement & parent = current.GetParent();
    if (parent == 0) {
      log->LogError(225, SimpleLogger::MESSAGE, item.c_str());
      throw VXIException::Fatal();
    }

    current = parent;
  };

  // (2) If the next item is specified (such as from a previous <goto>, look
  //     for an exact match.
  if (!item.empty()) {
    for (VXMLNodeIterator it(current); it; ++it) {
      VXMLNode child = *it;

      if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);

      if (!IsFormItemNode(elem)) continue;
      vxistring name;
      if (!elem.GetAttribute(ATTRIBUTE__ITEMNAME, name)) continue;
      if (item == name) throw JumpItem(elem);
    }
  }

  // (3) Otherwise, find the first non-filled item with a valid condition.
  else {
    for (VXMLNodeIterator it(current); it; ++it) {
      VXMLNode child = *it;

      if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);

      if (!IsFormItemNode(elem)) continue;

      // Must use itemname here, as could be implicit name
      vxistring itemname;
      if (!elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemname)) {
        log->LogError(999, SimpleLogger::MESSAGE,
                      L"unnamed item found on local jump");
        throw VXIException::Fatal();
      }

      if (exe->script.IsVarDefined(itemname)) continue;

      // OK if var is undefined, check condition
      vxistring cond;
      elem.GetAttribute(ATTRIBUTE_COND, cond);
      if (cond.empty() || exe->script.TestCondition(cond))
        throw JumpItem(elem);
    }
  }

  // log->LogDiagnostic(2, L"VXI::DoInnerJump - no match found.");
}

//#############################################################################
// Utility functions
//#############################################################################

// do_event() - top level call into event handler; deals with 
// event counts and defaults.
//
void VXI::DoEvent(const VXMLElement & item,
                  const VXIException::InterpreterEvent & e)
{
  // (0) Initial logging
  if (item == 0) {
    log->LogDiagnostic(0, L"VXI::DoEvent - invalid argument, ignoring event");
    return;
  }
  else if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::DoEvent(" << e.GetValue() << L")";
    log->EndDiagnostic();
  }

  // (1) Disable queuing of prompts outside of event handler.
  exe->playingPrompts = false;

  VXIint32 numRethrows = 0;
  VXIException::InterpreterEvent event = e;

  do {
    try {
      // (2) Increments counts associated with this event.
      exe->eventcounts.Increment(item, event.GetValue());

      // (3) Process the current event.
      exe->eventSource = item;
      bool handled = do_event(item, event);
      exe->eventSource = VXMLElement();

      // (4) If the event was handled (a suitable <catch> was found),
	  // this will complete event handling, and return to the FIA.
      if (handled) {
        if (log->IsLogging(2)) {
          log->StartDiagnostic(2) << L"VXI::DoEvent - event processed.";
          log->EndDiagnostic();
        }
        return;
      }

      // (5) No one willing to handle this event.  Exit.
      vxistring exitmessage(L"Unhandled exception: ");
      exitmessage += event.GetValue();
      VXIString * val = VXIStringCreate(exitmessage.c_str());
      if (val == NULL) throw VXIException::OutOfMemory();
      throw VXIException::Exit(reinterpret_cast<VXIValue*>(val));
    }
    // (5) The catch had a <throw> element inside.  Must process the new event.
    catch (const VXIException::InterpreterEvent & e) {
      event = e;
    }
  } while (++numRethrows < DEFAULT_MAX_EVENT_RETHROWS);
  // (7) Probable loop - catch X throws X?  Quit handling after a while. 

  log->LogError(216);
  vxistring exitmessage(L"Unhandled exception (suspected infinite loop)");
  VXIString * val = VXIStringCreate(exitmessage.c_str());
  if (val == NULL) throw VXIException::OutOfMemory();
  throw VXIException::Exit(reinterpret_cast<VXIValue*>(val));
}


bool VXI::do_event(const VXMLElement & item,
                   const VXIException::InterpreterEvent & e)
{
  vxistring event = e.GetValue();
  if (event.empty()) return false;

  // (1) Define the variables for the best match.
  int bestCount = 0;
  VXMLElement bestMatch;

  DocumentLevel stage = DOCUMENT;
  bool done = false;

  // Start from current item in document.
  VXMLElement currentNode = item;

  // (2) Get the count for the current event.
  int eventCount = exe->eventcounts.GetCount(item, event);
  if (eventCount == 0)
    return false;  // this shouldn't happen if increment was called

  if ( event[event.length() - 1] != '.' )
    event += L".";

  do {
    // (3) Walk through all nodes at this level looking for catch elements.
    for (VXMLNodeIterator it(currentNode); it; ++it) {
      VXMLNode child = *it;

      if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);

      // (3.1) Can this node catch events?
      VXMLElementType nodeName = elem.GetName();
      if (nodeName != NODE_CATCH) continue;

      // (3.2) Evaluate catch count;
      vxistring attr;
      int catchCount = 1;
      if (elem.GetAttribute(ATTRIBUTE_COUNT, attr)) {
        VXIchar * temp;
        catchCount = int(wcstol(attr.c_str(), &temp, 10));
      }
      if ( catchCount > eventCount ) continue;

      // (3.3) Evaluate its 'cond' attribute.
      elem.GetAttribute(ATTRIBUTE_COND, attr);
      if (!attr.empty() && !exe->script.TestCondition(attr)) continue;

      // (3.4) Get the catch's event list
      vxistring catchEvent;
      elem.GetAttribute(ATTRIBUTE_EVENT, catchEvent);

      // (3.5) Check if this handler can handle the thrown event
      bool canHandle = false;
      if ( catchEvent.empty() || catchEvent == L"." ) {
        canHandle = true;
      }
      else {
        // Is this handler either an exact match, or a
        // prefix match of the thrown event?
	    TokenList catchList(catchEvent);

        TokenList::iterator i;
        for (i = catchList.begin(); i != catchList.end(); ++i) {
          catchEvent = *i;
          if ( catchEvent[catchEvent.length() - 1] != '.' )
            catchEvent += L".";
          if ( wcsncmp( catchEvent.c_str(), event.c_str(), catchEvent.length() ) == 0 ) {
            canHandle = true;
            break;
          }
        }
      }

      // (3.6) Keep the handler with the highest count less than or 
      // equal the event count
      if ( canHandle && ( catchCount > bestCount ) ) {
        bestCount = catchCount;
        bestMatch = elem;
      }
    }

    // (4) Decide where to search next.
    const VXMLElement & parent = currentNode.GetParent();
    if (parent != 0)
      currentNode = parent;
    else {
      if (stage == DOCUMENT) {
        stage = APPLICATION;
        currentNode = exe->application.GetRoot();
        if (currentNode != 0) continue;
        // Otherwise, fall through to application level.
      }
      if (stage == APPLICATION) {
        stage = DEFAULTS;

        // We resort to this level _only_ if no match has been found.
        if (bestCount < 1) {
          vxistring language = 
            toString(exe->properties.GetProperty(PropertyList::Language));

          // We clear the current node.  It will be set either to the global
          // language (*) or to an exact match.
          currentNode = VXMLElement();

          VXMLElement defaultsRoot = domDefaultDoc.GetRoot();
          for (VXMLNodeIterator it(defaultsRoot); it; ++it) {
            VXMLNode child = *it;

            // Look for a language node.
            if (child.GetType() != VXMLNode::Type_VXMLElement) continue;

            const VXMLElement & elem 
              = reinterpret_cast<const VXMLElement &>(child);
            if (elem.GetName() != DEFAULTS_LANGUAGE) continue;

            vxistring id;
            elem.GetAttribute(ATTRIBUTE_ID, id);
            if (id == language || (id == GENERIC_DEFAULTS && currentNode == 0))
              currentNode = elem;
          }

          if (currentNode != 0) continue;
        }
      }

      done = true;
    }
  } while (!done);

  if (bestCount == 0)
    return false;

  // Compliance Note:
  //
  // Because of the 'as-if-by-copy' semantic, we now execute the catch content,
  // including relative URIs, in the local scope.  So nothing special needs to
  // be done.

  VXIMapHolder vars;
  VXIValue * temp = NULL;

  temp = reinterpret_cast<VXIValue *>(VXIStringCreate(e.GetValue().c_str()));
  if (temp == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(vars.GetValue(), L"_event", temp);

  if (e.GetMessage().empty())
    temp = reinterpret_cast<VXIValue *>(VXIPtrCreate(NULL));
  else
    temp =reinterpret_cast<VXIValue*>(VXIStringCreate(e.GetMessage().c_str()));

  if (temp == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(vars.GetValue(), L"_message", temp);
  
  execute_content(bestMatch, vars, e.GetActiveDialog());
  return true;
}


// Top level call into executable content section.
// Called from <block>,<catch>, and <filled>
//
void VXI::execute_content(const VXMLElement& doc, const VXIMapHolder & vars,
                          const VXMLElement& activeDialog)
{
  log->LogDiagnostic(2, L"VXI::execute_content()");

  // (1) Due to the complex algorithm for throw element to address
  // "as-if-by-copy" semantics, the anonymous scope must be retained
  if (exe->script.CurrentScope(SCOPE_Anonymous) && doc.GetName() != NODE_CATCH) {
    // (1.1) Add a new scope and allow anonymous variables to be defined. 
    exe->script.PopScope();
    exe->script.PushScope(SCOPE_Anonymous);
  }
  
  // (1.2) Add anonymous scope if not exist
  if( !exe->script.CurrentScope(SCOPE_Anonymous) )
    exe->script.PushScope(SCOPE_Anonymous);    
  
  // (2) Set externally specified variables (if necessary).
  if (vars.GetValue() != NULL) {
    const VXIchar * key;
    const VXIValue * value;
    VXIMapIterator * i = VXIMapGetFirstProperty(vars.GetValue(), &key, &value);
    if (VXIValueGetType(value) == VALUE_PTR)
      exe->script.MakeVar(key, L"");    // Set to ECMAScript undefined
    else
      exe->script.MakeVar(key, value);

    while (VXIMapGetNextProperty(i, &key, &value) == VXIvalue_RESULT_SUCCESS) {
      if (VXIValueGetType(value) == VALUE_PTR)
        exe->script.MakeVar(key, L"");  // Set to ECMAScript undefined
      else
        exe->script.MakeVar(key, value);
    }

    VXIMapIteratorDestroy(&i);
  }

  // (3) Walk through the children and execute each node.
  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<VXMLElement &>(child);
    executable_element(elem, activeDialog);
  }
}


// Executable element dispatch
//
void VXI::executable_element(const VXMLElement & elem, const VXMLElement & activeDialog)
{
  if (elem == 0) {
    log->LogError(999, SimpleLogger::MESSAGE, L"empty executable element");
    return;
  }

  VXMLElementType nodeName = elem.GetName();

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::executable_element - " << nodeName;
    log->EndDiagnostic();
  }

  if (nodeName == NODE_VAR)
    var_element(elem);
  else if (nodeName == NODE_ASSIGN)
    assign_element(elem);
  else if (nodeName == NODE_CLEAR)
    clear_element(elem);
  else if (nodeName == NODE_DISCONNECT)
    disconnect_element(elem);
  else if (nodeName == NODE_EXIT)
    exit_element(elem);
  else if (nodeName == NODE_GOTO)
    goto_element(elem);
  else if (nodeName == NODE_IF)
    if_element(elem);
  else if (nodeName == NODE_LOG)
    log_element(elem);
  else if (nodeName == NODE_PROMPT)
    executable_prompt(elem);
  else if (nodeName == NODE_REPROMPT)
    reprompt_element(elem, activeDialog);
  else if (nodeName == NODE_RETURN)
    return_element(elem);
  else if (nodeName == NODE_SCRIPT)
    script_element(elem);
  else if (nodeName == NODE_SUBMIT)
    submit_element(elem);
  else if (nodeName == NODE_THROW)
    throw_element(elem);
  else if (nodeName == NODE_DATA)
    data_element(elem);
  else
    if (voiceglue_loglevel() >= LOG_INFO)
    {
	std::ostringstream logstring;
	logstring << "unexpected executable element = ";
	logstring << nodeName;
	voiceglue_log ((char) LOG_INFO, logstring);
    };
  //log->LogError(999, SimpleLogger::MESSAGE,L"unexpected executable element");
}


/* 
 * Process <var> elements in current interp context.
 *
 * This differs from assign in that it makes new var in current scope
 * assign follows scope chain lookup for var name (and throws 
 *   error if fails)
 *
 * This is also used for initialiation of guard vars in field items
 *
 * <var> processing is compliated by the need to check for <param> values
 *  from subdialog calls.
 */
void VXI::var_element(const VXMLElement & doc)
{
  vxistring name;
  vxistring expr;
  doc.GetAttribute(ATTRIBUTE_NAME, name);
  doc.GetAttribute(ATTRIBUTE_EXPR, expr);

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::var_element(name=\"" << name
                            << L"\" expr = \"" << expr << L"\")";
    log->EndDiagnostic();
  }

  if (name.empty()) return;

// VXML 2.0 Spec (5.3.1) 
  if( name.find( L"." ) != vxistring::npos )
  {
    log->LogError(219, SimpleLogger::URI, name.c_str());
    throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                        L"variables cannot be declared with scope prefix "
                        L"and ECMAScript script variable names must not include a dot");
  }

  exe->script.MakeVar(name, expr);
}

 
// Process <assign> elements in current interpreter context
//
void VXI::assign_element(const VXMLElement & doc)
{
  vxistring name;
  doc.GetAttribute(ATTRIBUTE_NAME, name);
  if (name.empty()) return;

  vxistring expr;
  doc.GetAttribute(ATTRIBUTE_EXPR, expr);

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::assign_element(name=\"" << name
                            << L"\" expr = \"" << expr << L"\"";
    log->EndDiagnostic();
  }

  exe->script.SetVar(name, expr);
}



// Handler for meta elements. Do nothing for now.
//
void VXI::meta_element(const VXMLElement & doc)
{
  // vxistring & name = doc.GetAttribute("name");
  // vxistring & name = doc.GetAttribute("content");
  // vxistring & name = doc.GetAttribute("http-equiv");
}


// Handler for clear elements.  This may resets all form items or a user
// specified subset.
//
void VXI::clear_element(const VXMLElement & doc)
{
  log->LogDiagnostic(2, L"VXI::clear_element()");

  // (1) Get the namelist.
  vxistring namelist;
  doc.GetAttribute(ATTRIBUTE_NAMELIST, namelist);

  // (2) Handle the easy case: empty namelist --> clear all
  if (namelist.empty()) {
    // (2.1) Clear prompt and event counts.
    exe->promptcounts.Clear();
    exe->eventcounts.Clear(false);

    // (2.2) The list of form items resides in the slot map.
    TokenList::iterator i;
    TokenList & formitems = exe->formitems;
    for (i = formitems.begin(); i != formitems.end(); ++i)
      exe->script.ClearVar(*i);

    return;
  }

  // (3) Handle case where user specifies form items.

  TokenList names(namelist);
  for (TokenList::const_iterator i = names.begin(); i != names.end(); ++i) {
    // (3.1) Check that the name is a real form item.  A linear search should
    //       be sufficently fast.
    TokenList::iterator j;
    TokenList & formitems = exe->formitems;
    for (j = formitems.begin(); j != formitems.end(); ++j)
      if (*i == *j) break;

    exe->script.ClearVar(*i);

    if (j != formitems.end()) {
      // (3.2) Clear the associated counters for form items.
      exe->promptcounts.Clear(*i);
      exe->eventcounts.Clear(*i);
    }
  }
}

void VXI::data_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::data_element()");

  // (1) Get fetch properties

  // (1.1) Get URI
  vxistring uri;
  elem.GetAttribute(ATTRIBUTE_SRC, uri);
  if (uri.empty()) {
    elem.GetAttribute(ATTRIBUTE_SRCEXPR, uri);
    if (!uri.empty())
      exe->script.EvalScriptToString(uri, uri);
  }

  // (1.2) Get Submit-specific properties.
  VXIMapHolder submitData;
  if (submitData.GetValue() == NULL) throw VXIException::OutOfMemory();

  vxistring att;
  elem.GetAttribute(ATTRIBUTE_NAMELIST, att);
  if (!att.empty()) {
    TokenList names(att);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(submitData.GetValue(), (*i).c_str(), val);
    }
  }

  // (1.3) Get fetch properties.
  VXIMapHolder fetchobj;
  if (fetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();

  exe->properties.GetFetchobjBase(fetchobj);

  VXIMapHolder fetchAudioFetchobj(NULL);
  fetchAudioFetchobj = fetchobj;
  if (fetchAudioFetchobj.GetValue() == NULL) throw VXIException::OutOfMemory();

  exe->properties.GetFetchobjCacheAttrs(elem, PropertyList::Data, fetchobj);
  exe->properties.GetFetchobjCacheAttrs(elem, PropertyList::Audio, fetchAudioFetchobj);

  // (1.4) Add namelist values
  if (submitData.GetValue() != NULL) {
    if (!exe->properties.GetFetchobjSubmitAttributes(elem, submitData,
                                                     fetchobj))
    {
      // This should never occur.
      log->StartDiagnostic(0) << L"VXI::data_element - couldn't set "
                                 L"the submit attributes.";
      log->EndDiagnostic();
      throw VXIException::InterpreterEvent(EV_ERROR_BADURI);
    }
    submitData.Release(); // This map is now owned by fetchobj.
  }

  // (2) Start fetch audio.
  vxistring fetchaudio;
  if (!elem.GetAttribute(ATTRIBUTE_FETCHAUDIO, fetchaudio))
    fetchaudio = toString(exe->properties.GetProperty(L"fetchaudio"));

  if (!fetchaudio.empty()) {
    // Get All properties for fetching audio
    VXIMapHolder fillerProp(VXIMapClone(fetchAudioFetchobj.GetValue()));
    exe->properties.GetProperties(fillerProp);

    pm->PlayAll(); // Finish playing already queued prompts
    pm->PlayFiller(fetchaudio, fillerProp);
  }

  // (3) Get the data
  VXIMapHolder fetchStatus;
  DOMDocument *doc = 0;

  //  First, see if a varname was even used,
  //  If not no point in parsing the resulting XML
  vxistring varname;
  elem.GetAttribute(ATTRIBUTE_NAME, varname);
  bool parse_doc = (! varname.empty());

  int result = parser->FetchXML(
      uri.c_str(), fetchobj, fetchStatus,
      inet, *log, &doc, parse_doc);

  switch (result) {
  case -1: // Out of memory
    throw VXIException::OutOfMemory();
  case 0:  // Success
    break;
  case 1: // Invalid parameter
  case 2: // Unable to open URI
    log->LogError(203);
    // now look at the http status code to throw the 
    // compiliant error.badfetch.http.<response code> 
    if (fetchStatus.GetValue()) {
      // retrieve response code
      VXIint iv = 0;
      const VXIValue *v = 
        VXIMapGetProperty(fetchStatus.GetValue(), INET_INFO_HTTP_STATUS);
      if( v && VXIValueGetType(v) == VALUE_INTEGER &&
          ((iv = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v))) > 0)
         )
      {
        std::basic_stringstream<VXIchar> respStream;
        respStream << EV_ERROR_BADFETCH << L".http.";
        respStream << iv;
        
        throw VXIException::InterpreterEvent(respStream.str().c_str(), uri);    
      }
    }                      
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  case 3: // Unable to read from URI
    log->LogError(204);
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  case 4: // Unable to parse contents of URI
  { 
    log->LogError(205);
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  }
  default:
    log->LogError(206);
    throw VXIException::Fatal();
  }

  if (parse_doc)
  {
      // (4) Validate per VXML sandboxing
      const VXIValue * absurl = 
	  VXIMapGetProperty(fetchStatus.GetValue(), INET_INFO_ABSOLUTE_NAME);
      if (absurl == NULL || VXIValueGetType(absurl) != VALUE_STRING) {
	  throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
      }

      // (5) Check access
      AccessControl ac(defAccessControl, doc);
      if (!ac.canAccess(VXIStringCStr
			(reinterpret_cast<const VXIString*>(absurl))))
	  throw VXIException::InterpreterEvent(EV_ERROR_NOAUTHORIZ);

      // (6) Assign var
      exe->script.MakeVar(varname, doc);
  }
}

// This implementation returns the values as VXIObjects.
//
void VXI::exit_element(const VXMLElement & doc)
{
  log->LogDiagnostic(2, L"VXI::exit_element()");

  VXIMapHolder exprMap(NULL); 

  vxistring namelist;
  doc.GetAttribute(ATTRIBUTE_NAMELIST, namelist);
  if (!namelist.empty()) {
    exprMap.Acquire(VXIMapCreate());
    if (exprMap.GetValue() == NULL) throw VXIException::OutOfMemory();

    TokenList names(namelist);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      if (!exe->script.IsVarDeclared(*i))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(exprMap.GetValue(), (*i).c_str(), val);
    }
  }

  VXIValue * exprResult = NULL;

  vxistring expr;
  doc.GetAttribute(ATTRIBUTE_EXPR, expr);
  if (!expr.empty()) {
    // To evaluate expressions is a bit more ugly.  Because any object may be
    // returned by the expression, we create a new variable, evaluate it, and
    // return the result.
    vxistring variable;
    VXMLDocumentModel::CreateHiddenVariable(variable);
    exe->script.MakeVar(variable, expr);
    exprResult = exe->script.GetValue(variable);
  }

  // Now combine the two into one result (if necessary)
  if (exprMap.GetValue() != NULL && exprResult != NULL) {
    // We insert the exprResult into the exprMap.
    VXIMapSetProperty(exprMap.GetValue(), L"Attribute_expr_value", exprResult);
    exprResult = NULL;
  }
  if (exprResult == NULL)
    exprResult = reinterpret_cast<VXIValue *>(exprMap.Release());

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"exit_element got \"" << exprResult << L"\"";
    log->EndDiagnostic();
  }

  pm->PlayAll();

  throw VXIException::Exit(exprResult); // Stuff exprResult in here.
}


void VXI::disconnect_element(const VXMLElement & doc)
{
  log->LogDiagnostic(2, L"VXI::disconnect_element()");

  pm->PlayAll();

  VXIMapHolder namelistMap(NULL); 
  vxistring namelist;
  doc.GetAttribute(ATTRIBUTE_NAMELIST, namelist);
  if (!namelist.empty()) {
    namelistMap.Acquire(VXIMapCreate());
    if (namelistMap.GetValue() == NULL) throw VXIException::OutOfMemory();

    TokenList names(namelist);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      if (!exe->script.IsVarDeclared(*i))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(namelistMap.GetValue(), (*i).c_str(), val);
    }
  }

  if (tel->Disconnect(tel,namelistMap.GetValue()) != VXItel_RESULT_SUCCESS)
    log->LogError(213);

  throw VXIException::InterpreterEvent(EV_TELEPHONE_HANGUP);
}

void VXI::reprompt_element(const VXMLElement & doc, const VXMLElement & activeDialog)
{
  log->LogDiagnostic(2, L"VXI::reprompt_element()");

  // set a flag enabling prompting
  exe->playingPrompts = true;

  // set current active dialog
  if( activeDialog != 0 && activeDialog.GetName() == NODE_MENU)
    exe->currentDialog = activeDialog;
}


void VXI::throw_element(const VXMLElement & elem, const VXMLElement & activeDialog)
{
  vxistring event;
  elem.GetAttribute(ATTRIBUTE_EVENT, event);
  if (event.empty()) {
    elem.GetAttribute(ATTRIBUTE_EVENTEXPR, event);
    if (!event.empty())
      exe->script.EvalScriptToString(event, event);
  }
  if (event.empty()) {
    // This should be caught by the DTD, but just in case...
    log->LogDiagnostic(0, L"VXI::throw_element - failure, no event to throw");
    throw VXIException::InterpreterEvent(EV_ERROR_BAD_THROW);
  }

  vxistring message;
  elem.GetAttribute(ATTRIBUTE_MESSAGE, message);
  if (message.empty()) {
    elem.GetAttribute(ATTRIBUTE_MESSAGEEXPR, message);
    if (!message.empty())
      exe->script.EvalScriptToString(message, message);
  }

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::throw_element - throwing (" << event
                            << L", " << message << L")";
    log->EndDiagnostic();
  }
  throw VXIException::InterpreterEvent(event, message, activeDialog);
}


// The log element is handled in a slihtly odd manner.  The expressions in
// attributes are evaluated first because "expressions must be evaluated in
// document order".  And even if no output is being generated, the expressions
// must still be generated.
//
void VXI::log_element(const VXMLElement & doc)
{
  bool isLogging = log->IsLogging(1);
  vxistring diagLog;

  VXIVector * keys = VXIVectorCreate();
  if (keys == NULL) throw VXIException::OutOfMemory();

  VXIVector * vals = VXIVectorCreate();
  if (vals == NULL) {
    VXIVectorDestroy(&keys);
    throw VXIException::OutOfMemory();
  }

  std::basic_ostringstream<wchar_t> out;
  try {
    vxistring attr;

    // Process the label attribute
    if (doc.GetAttribute(ATTRIBUTE_LABEL, attr) && !attr.empty()) {
      VXIString * temp = VXIStringCreate(L"label");
      VXIVectorAddElement(keys, reinterpret_cast<VXIValue *>(temp));
      temp = VXIStringCreate(attr.c_str());
      VXIVectorAddElement(vals, reinterpret_cast<VXIValue *>(temp));

      if (isLogging) {
        diagLog += L"[label:";
        diagLog += attr.c_str();
        diagLog += L"] ";
      }
    }

    // Process the expr attribute
    VXIValue * expr = NULL;
    if (doc.GetAttribute(ATTRIBUTE_EXPR, attr) && !attr.empty()) {
      expr = exe->script.EvalScriptToValue(attr);

      out << expr;
      VXIValueDestroy(&expr);
      
      VXIString * temp = VXIStringCreate(L"expr");
      VXIVectorAddElement(keys, reinterpret_cast<VXIValue *>(temp));
      temp = VXIStringCreate(out.str().c_str());
      VXIVectorAddElement(vals, reinterpret_cast<VXIValue *>(temp));

      if (isLogging) {
        diagLog += L"[expr:";
        diagLog += out.str();
        diagLog += L"] ";
      }
    }

    // Process the CDATA
    bool first = true;
    const VXIchar * const SPACE = L" ";
    out.str(L"");

    for (VXMLNodeIterator it(doc); it; ++it) {
      VXMLNode child = *it;

      switch (child.GetType()) {
      case VXMLNode::Type_VXMLContent:
        if (first)
          first = false;
        else
          out << SPACE;
        out << reinterpret_cast<const VXMLContent &>(child).GetValue();
        break;
      case VXMLNode::Type_VXMLElement:
      {
        const VXMLElement & elem = reinterpret_cast<const VXMLElement&>(child);
        if (elem.GetName() == NODE_VALUE) {
          vxistring value;
          elem.GetAttribute(ATTRIBUTE_EXPR, value);
          if (value.empty()) break;

          VXIValue * val = exe->script.EvalScriptToValue(value);

          if (val != NULL) {
            if (first)
              first = false;
            else
              out << SPACE;
            out << val;
            VXIValueDestroy(&val);
          }
        }
        break;
      }
      default:
        break;
      }
    }

    // Log contents.
    if (!out.str().empty()) {
      VXIString * temp = VXIStringCreate(L"content");
      VXIVectorAddElement(keys, reinterpret_cast<VXIValue *>(temp));
      temp = VXIStringCreate(out.str().c_str());
      VXIVectorAddElement(vals, reinterpret_cast<VXIValue *>(temp));
    }

    log->LogEvent(VXIlog_EVENT_LOG_ELEMENT, keys, vals);

    if (isLogging) {
      diagLog += out.str();
      log->LogDiagnostic(1, diagLog.c_str());
    }

    VXIVectorDestroy(&keys);
    VXIVectorDestroy(&vals);
    return;
  }
  catch (...) {
    // Log as much as possible if an error occurred.
    if (!out.str().empty()) {
      VXIString * temp = VXIStringCreate(L"content");
      VXIVectorAddElement(keys, reinterpret_cast<VXIValue *>(temp));
      temp = VXIStringCreate(out.str().c_str());
      VXIVectorAddElement(vals, reinterpret_cast<VXIValue *>(temp));
    }

    log->LogEvent(VXIlog_EVENT_LOG_ELEMENT, keys, vals);

    if (isLogging) {
      diagLog += out.str();
      log->LogDiagnostic(1, diagLog.c_str());
    }

    VXIVectorDestroy(&keys);
    VXIVectorDestroy(&vals);
    throw;
  }
}


void VXI::if_element(const VXMLElement & doc)
{
  log->LogDiagnostic(2, L"VXI::if_element()");

  vxistring cond;
  doc.GetAttribute(ATTRIBUTE_COND, cond);
  bool executing = !cond.empty() && exe->script.TestCondition(cond);

  // Loop through children.
  // If cond == true, execute until <else> or <elseif>
  // If cond == false, continue until <else> or <elseif> 
  //   and test condition where <else> === <elseif cond="true">
  // Control is a little strange since intuitive body of <if> 
  //  and <else> are actually siblings of <else> and <elseif>, 
  //  not descendants

  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<VXMLElement &>(child);

    VXMLElementType nodeName = elem.GetName();
    if (nodeName == NODE_ELSE) { 
      if (executing) return;
      else executing = true;
    }
    else if (nodeName == NODE_ELSEIF) { 
      if (executing) return;
      vxistring cond;
      elem.GetAttribute(ATTRIBUTE_COND, cond);
      executing = !cond.empty() && exe->script.TestCondition(cond);
    }
    else if (executing)
      executable_element(elem);
  }
}



// <goto> is identical to submit except
//  - it can transition to internal items
//  - doesn't have a namelist
//
void VXI::goto_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::goto_element()");

  // Clear any information from unprocessed jumps.
  if (sdParams != NULL) {
    VXIMapDestroy(&sdParams);
    sdParams = NULL;
  }
  if (sdResult != NULL) {
    VXIMapDestroy(&sdResult);
    sdResult = NULL;
  }
  if (sdEvent != NULL) {
    delete sdEvent;
    sdEvent = NULL;
  }

  // According to the spec, the attributes follow the priority sequence:
  //   next, expr, nextitem, expritem

  vxistring uri;
  elem.GetAttribute(ATTRIBUTE_NEXT, uri);
  if (uri.empty()) {
    elem.GetAttribute(ATTRIBUTE_EXPR, uri);
    if (!uri.empty())
      exe->script.EvalScriptToString(uri, uri);
  }

  if (uri.empty()) {
    vxistring target;
    elem.GetAttribute(ATTRIBUTE_NEXTITEM, target);
    if (target.empty()) {
      elem.GetAttribute(ATTRIBUTE_EXPRITEM, target);
      if (!target.empty())
        exe->script.EvalScriptToString(target, target);
    }
    if (!target.empty()) {
      DoInnerJump(elem, target);
      throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uriCurrent);
    }
  }
  if (uri.empty())
    throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);

  PerformTransition(elem, uri);
}


void VXI::submit_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::submit_element()");

  // Clear any information from unprocessed jumps.
  if (sdParams != NULL) {
    VXIMapDestroy(&sdParams);
    sdParams = NULL;
  }
  if (sdResult != NULL) {
    VXIMapDestroy(&sdResult);
    sdResult = NULL;
  }
  if (sdEvent != NULL) {
    delete sdEvent;
    sdEvent = NULL;
  }

  // Get Submit-specific properties.
  VXIMapHolder submitData;
  if (submitData.GetValue() == NULL) throw VXIException::OutOfMemory();

  vxistring att;
  elem.GetAttribute(ATTRIBUTE_NAMELIST, att);
  if (att.empty()) {
    TokenList::const_iterator i;
    for (i = exe->formitems.begin(); i != exe->formitems.end(); ++i) {
      // Check for and ignore hidden variables.
      if (VXMLDocumentModel::IsHiddenVariable(*i)) continue;
      // Otherwise, add it.
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(submitData.GetValue(), (*i).c_str(), val);
    }
  }
  else {
    TokenList names(att);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(submitData.GetValue(), (*i).c_str(), val);
    }
  }

  vxistring uri;
  elem.GetAttribute(ATTRIBUTE_NEXT, uri);
  if (uri.empty()) {
    elem.GetAttribute(ATTRIBUTE_EXPR, uri);
    if (!uri.empty())
      exe->script.EvalScriptToString(uri, uri);
  }

  // PerformTransition now owns the submitData.
  PerformTransition(elem, uri, submitData.Release(), false, true);
}


void VXI::script_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::script_element()");

  // (1) Is the source specified?
  vxistring uri;
  elem.GetAttribute(ATTRIBUTE_SRC, uri);
  if (uri.empty()) {
    elem.GetAttribute(ATTRIBUTE_SRCEXPR, uri);
    if (!uri.empty())
      exe->script.EvalScriptToString(uri, uri);
  }

  // (2) If not, use the content of the script.
  if (uri.empty()) {
    for (VXMLNodeIterator it(elem); it; ++it) {
      VXMLNode child = *it;
      
      if (child.GetType() != VXMLNode::Type_VXMLContent) {
        log->LogDiagnostic(0, L"VXI::script_element - unexpected element");
        continue;
      }

      VXMLContent & content = reinterpret_cast<VXMLContent &>(child);
      vxistring str = content.GetValue();
      if (!str.empty()) exe->script.EvalScript(str);
      return;
    }
  }

  // (3) Otherwise, we must retrieve the document.

  // (3.1) Get the cache attributes and document base.
  VXIMapHolder fetchobj;
  exe->properties.GetFetchobjBase(fetchobj);
  exe->properties.GetFetchobjCacheAttrs(elem, PropertyList::Script, fetchobj);

  // (3.2) Get the character encoding.
  vxistring encoding;
  if (!elem.GetAttribute(ATTRIBUTE_CHARSET, encoding))
    encoding = L"UTF-8";

  // (3.3) Get the content.
  vxistring content;
  VXIMapHolder fetchInfo;
  switch (DocumentParser::FetchContent(uri.c_str(), fetchobj, fetchInfo, inet, *log,
                                       encoding, content))
  {
  case -1: // Out of memory
    throw VXIException::OutOfMemory();
  case 0:  // Success
    break;
  case 1: // Invalid parameter
  case 2: // Unable to open URI
    log->LogError(203, SimpleLogger::URI, uri.c_str());
    // now look at the http status code to throw the 
    // compiliant error.badfetch.http.<response code> 
    if (fetchInfo.GetValue()) {
      // retrieve response code
      VXIint iv = 0;
      const VXIValue *v = 
        VXIMapGetProperty(fetchInfo.GetValue(), INET_INFO_HTTP_STATUS);
      if( v && VXIValueGetType(v) == VALUE_INTEGER &&
          ((iv = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v))) > 0)
         )
      {
        std::basic_stringstream<VXIchar> respStream;
        respStream << EV_ERROR_BADFETCH << L".http.";
        respStream << iv;
        throw VXIException::InterpreterEvent(respStream.str().c_str(), uri);    
      }
    }                      
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  case 3: // Unable to read from URI
    log->LogError(204, SimpleLogger::URI, uri.c_str());
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  case 4: // Unsupported encoding
    log->LogError(208, SimpleLogger::URI, uri.c_str(),
                  SimpleLogger::ENCODING, encoding.c_str());
    throw VXIException::InterpreterEvent(EV_ERROR_BADFETCH, uri);
  default:
    log->LogError(206, SimpleLogger::URI, uri.c_str());
    throw VXIException::Fatal();
  }

  // (3.4) Execute the script.
  exe->script.EvalScript(content);
}

/***************************************************************
 * Collect Phase
 ***************************************************************/

void VXI::CollectPhase(const VXMLElement & form, const VXMLElement & item)
{
  if (item == 0) {
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"empty executable element in collecting phase");
    return;
  }

  VXMLElementType nodeName = item.GetName();

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::CollectPhase - " << nodeName;
    log->EndDiagnostic();
  }

  // Clear the event counts if we're collecting from a different form item
  vxistring itemname;
  form.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);
  exe->eventcounts.ClearIf(itemname, false);

  if (nodeName == NODE_BLOCK)
    block_element(item);
  else if (nodeName == NODE_OBJECT)
    object_element(item);
  else if (nodeName == NODE_TRANSFER)
    transfer_element(form, item);
  else if (nodeName == NODE_FIELD || nodeName == NODE_INITIAL)
    field_element(form, item);
  else if (nodeName == NODE_SUBDIALOG)
    subdialog_element(item);
  else if (nodeName == NODE_RECORD)
    record_element(form, item);
  else if (nodeName == NODE_MENU)
    menu_element(item);
  else if (nodeName == NODE_DATA)
    data_element(item);
  else
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"unexpected executable element in collecting phase");
}


void VXI::block_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::block_element()");

  exe->playingPrompts = true;

  // (1) Set this variable to prevent re-executing this block in the FIA.
  vxistring itemname;
  elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);
  exe->script.SetVar(itemname, L"true");

  // (2) Execute the content of the <block>
  execute_content(elem);
}


void VXI::subdialog_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::subdialog_element()");
  
  // (1) Set properties.
  exe->properties.SetProperties(elem, FIELD_PROP, VXIMapHolder());

  // (2) Disables any existing grammars and play entry prompts.
  exe->gm.DisableAllGrammars();
  queue_prompts(elem);

  // (3) Search for parameters defined within the subdialog tag.
  VXIMapHolder params(CollectParams(elem, false));

  // (4) Retrieve namelist properties.  This is similar to <submit> but differs
  //     in its default behavior.
  VXIMapHolder submitData;
  if (submitData.GetValue() == NULL) throw VXIException::OutOfMemory();

  vxistring att;
  elem.GetAttribute(ATTRIBUTE_NAMELIST, att);
  if (!att.empty()) {
    TokenList names(att);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(submitData.GetValue(), (*i).c_str(), val);
    }
  }

  vxistring uri;
  elem.GetAttribute(ATTRIBUTE_SRC, uri);
  if (uri.empty()) {
    elem.GetAttribute(ATTRIBUTE_SRCEXPR, uri);
    if (!uri.empty())
      exe->script.EvalScriptToString(uri, uri);
  }

  if (uri.empty())
    throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC,
                                         L"src or srcexpr must be specified");

  try {
    PerformTransition(elem, uri, submitData.Release(), true);
  }
  catch (JumpDoc & e) {  // If the jump works
    exe->lastItem = elem;
    sdParams = params.Release();
    throw e;
  }
}


void VXI::return_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::return_element()");

  // (1) Returning when subdialog was not invoked is a semantic error.
  if (stackDepth == 1)
    throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);

  // (2) Does this generate an event?

  vxistring event;

  elem.GetAttribute(ATTRIBUTE_EVENT, event);
  if (event.empty())
    elem.GetAttribute(ATTRIBUTE_EVENTEXPR, event);

  if (!event.empty()) {
    try {
      throw_element(elem);
    }
    catch (const VXIException::InterpreterEvent & evt) {
      sdEvent = new VXIException::InterpreterEvent(evt);
      if (sdEvent == NULL) throw VXIException::OutOfMemory();

      throw JumpReturn();
    }

    log->LogError(999, SimpleLogger::MESSAGE, L"return event throw failed");
  }

  // (3) No, go to namelist case.

  VXIMapHolder resultVars;
  vxistring namelist;
  elem.GetAttribute(ATTRIBUTE_NAMELIST, namelist);
  if (!namelist.empty()) {
    TokenList names(namelist);
    TokenList::const_iterator i;
    for (i = names.begin(); i != names.end(); ++i) {
      VXIValue * val = exe->script.GetValue(*i);
      if (val != NULL)
        VXIMapSetProperty(resultVars.GetValue(), (*i).c_str(), val);
    }
  }

  sdResult = resultVars.Release();
  throw JumpReturn();
}


/*
 * Record element: The spec for <record> is a little unclear. The difficulties
 *  arise because the spec allows simultaneous recognition and recording 
 *  If a recording is  made the result is put into a the field var 
 *  and can apparently be used in-line in prompts. However, it is unclear
 *  what should happen if both recording and recognition occur.
 * In the current implementation, we are making <record> a pure recording
 *  tag with no recognition or dtmf enabled. 
 */
void VXI::record_element(const VXMLElement & form, const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::record_element()");
  
  // (0) Is this modal?
  vxistring modal;
  bool isModal = !(elem.GetAttribute(ATTRIBUTE_MODAL, modal) &&
                   modal == L"false");

  // (1) Set properties.
  exe->properties.SetProperties(elem, FIELD_PROP, VXIMapHolder());

  // (2) Disables any existing grammars and play entry prompts.
  exe->gm.DisableAllGrammars();
  queue_prompts(elem);

  // (3) Create a map containing the properties for the record.

  // (3.1) Get the other attributes.
  vxistring maxtime;  elem.GetAttribute(ATTRIBUTE_MAXTIME,      maxtime);
  vxistring silence;  elem.GetAttribute(ATTRIBUTE_FINALSILENCE, silence);
  vxistring dtmfterm; elem.GetAttribute(ATTRIBUTE_DTMFTERM,     dtmfterm);
  vxistring type;     elem.GetAttribute(ATTRIBUTE_TYPE,         type);

  // (3.2) Add them to the map.
  if (!maxtime.empty())
    exe->properties.SetProperty(GrammarManager::MaxTime,
                                maxtime.c_str(), FIELD_PROP);
  if (!silence.empty())
    exe->properties.SetProperty(GrammarManager::FinalSilence,
                                silence.c_str(), FIELD_PROP);
  if (!dtmfterm.empty())
    exe->properties.SetProperty(GrammarManager::DTMFTerm,
                                dtmfterm.c_str(), FIELD_PROP);
  if (!type.empty())
    exe->properties.SetProperty(GrammarManager::RecordingType,
                                type.c_str(), FIELD_PROP);

  // (3.3) Flatten the properties.
  VXIMapHolder recProps(exe->gm.GetRecordProperties(exe->properties,
                                                 pm->GetMillisecondTimeout()));
  if (recProps.GetValue() == NULL) throw VXIException::OutOfMemory();
  
  // (3.4) Activate grammars

  // Get form name.
  vxistring formName;
  form.GetAttribute(ATTRIBUTE__ITEMNAME, formName);

  // Get dialog name
  vxistring itemname;
  elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);

  exe->gm.EnableGrammars(exe->documentName, formName, itemname,
                         recProps, isModal, exe->script, exe->properties);

  // (3.5) Possibly append a beep.
  vxistring beep;
  if (elem.GetAttribute(ATTRIBUTE_BEEP, beep) && beep == L"true") {
      VXIMapSetProperty( recProps.GetValue(), REC_BEEP, reinterpret_cast<VXIValue*>(VXIStringCreate( beep.c_str() )) );
  }
  else{
      VXIMapSetProperty( recProps.GetValue(), REC_BEEP, reinterpret_cast<VXIValue*>(VXIStringCreate( L"false" )));
  }

  // (4) Issue record call and handle the return values.
  VXIrecRecordResult * answer = NULL;
  VXIInteger * duration = NULL;
  VXIString * termchar = NULL;
  VXIInteger * size = NULL;

  try {
    bool playedBargeinDisabledPrompt;
    pm->Play(&playedBargeinDisabledPrompt);

    GrammarManager::RecResult recResult = exe->gm.Record(recProps, playedBargeinDisabledPrompt,
                                   answer);

    switch (recResult) {
    case GrammarManager::Success:     // Record produced a recording
      break;
    case GrammarManager::InternalError: // Invalid result from Record
      throw VXIException::Fatal();
    default:
      log->LogError(999, SimpleLogger::MESSAGE,
                    L"unexpected value in recording result");
      throw VXIException::Fatal();
    }

    // (5) Trigger filled or hangup event as appropriate.

    // Set up shadow variables
    vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + itemname + L"$";

    // (5.1) The NLSML result is optional for record.  If not supplied
    // we need to handle the mark stuff here, otherwise, normal
    // NLSML processing will handle it.
    if (answer->xmlresult == NULL) {
      if (answer->markname != NULL) {
        exe->script.SetValue(shadowbase + L".markname", reinterpret_cast<const VXIValue*>(answer->markname));

        VXIInteger *mtime = VXIIntegerCreate(answer->marktime);
        exe->script.SetValue(shadowbase + L".marktime", reinterpret_cast<const VXIValue*>(mtime));
        VXIIntegerDestroy(&mtime);
      }
      else {
        exe->script.EvalScript(shadowbase + L".markname = undefined;");
        exe->script.EvalScript(shadowbase + L".marktime = undefined;");
      }
    }
    else {
      try {
        ProcessRecognitionResult(answer, exe->properties);
      }
      catch (VXIException::InterpreterEvent & e) {
                
        // Rethrow anything which is not nomatch
        if (e.GetValue() != EV_NOMATCH)
          throw e;

        // Record implementation is broken.
        log->LogError(441, SimpleLogger::MESSAGE, L"unexpected return value");
        throw VXIException::InterpreterEvent(EV_ERROR_RECORD);
      }
      catch (AnswerInformation & info) {
        // If the grammar is external, we must rethrow.
        if (info.element != elem) {
          CheckLineStatus();
          throw info;
        }

        // Had a successful recognition.
        exe->script.EvalScript(shadowbase +
                       L".utterance = application.lastresult$[0].utterance");
        exe->script.EvalScript(shadowbase +
                       L".confidence = application.lastresult$[0].confidence");
      }
    }

    // (6) Store the waveform and create shadow variables.
    
    if (answer->waveform == NULL) {
      // nothing was recorded.  according to spec, a noinput is not thrown, rather
      // the FIA continues as normal
      CheckLineStatus();
    }
    else {
      // (6.1) put waveform into 'itemname'
      exe->script.SetValue(itemname,
                         reinterpret_cast<const VXIValue*>(answer->waveform));

      // (6.2) Set up shadow variables

      // (6.2.1) duration
      duration = VXIIntegerCreate(answer->duration);
      if (duration == NULL) VXIException::OutOfMemory();
      vxistring shadow = shadowbase + L".duration";
      exe->script.SetValue(shadow, reinterpret_cast<const VXIValue*>(duration));
      VXIIntegerDestroy(&duration); duration = NULL;

      // (6.2.2) termchar
      if (answer->termchar != 0) {
        VXIchar tempChar = static_cast<VXIchar>(answer->termchar);
        termchar = VXIStringCreateN(&tempChar, 1);
        if (termchar == NULL) VXIException::OutOfMemory();
        shadow = shadowbase + L".termchar";
        exe->script.SetValue(shadow,reinterpret_cast<const VXIValue*>(termchar));
        VXIStringDestroy(&termchar); termchar = NULL;
      }
      else {
        shadow = shadowbase + L".termchar = null";
        exe->script.EvalScript(shadow);
      }

      // (6.2.3) maxtime
      shadow = shadowbase + L".maxtime = ";
      shadow += (answer->maxtime ? L"true" : L"false");
      exe->script.EvalScript(shadow);
    
      // (6.2.4) size
      // we are not interested in the actual data at this point, so we pass null for the content
      const VXIchar * contentType;
      const VXIbyte * content;
      VXIulong contentSize = 0; 
      if (VXIContentValue(answer->waveform, &contentType, &content, &contentSize)
          != VXIvalue_RESULT_SUCCESS)
        throw VXIException::Fatal();

      size = VXIIntegerCreate(contentSize);
      if (size == NULL) VXIException::OutOfMemory();
      shadow = shadowbase + L".size";
      exe->script.SetValue(shadow, reinterpret_cast<const VXIValue*>(size));
      VXIIntegerDestroy(&size); size = NULL;

      // (7) Maybe check line status
      CheckLineStatus();

      // (8) Trigger filled or hangup event as appropriate.
      ProcessFilledElements(itemname, form);
    }

    // (9) Call destroy on the structure.
    if (answer != NULL) {
      answer->Destroy(&answer);
      answer = NULL;
    }
  }

  catch (...) {
    if (answer != NULL)   answer->Destroy(&answer);
    if (duration != NULL) VXIIntegerDestroy(&duration);
    if (termchar != NULL) VXIStringDestroy(&termchar);
    if (size != NULL)     VXIIntegerDestroy(&size);
    throw;
  }
}


/*
 * Object element: this is essentially a platform dependent field 
 *  with odd return values. In order to preserve generality, 
 *  we will setup field item processing here (prompts and properties).
 *  We will collect the rest of the attributes and params into separate
 *  objects (to avoid name collisions) and pass them down to the 
 *  object handler. The object handler must copy out any data it 
 *  wants as we gc the args and params.
 */
void VXI::object_element(const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::object_element()");

  // (1) Set properties.
  exe->properties.SetProperties(elem, FIELD_PROP, VXIMapHolder());

  // (2) Disable any existing grammars and play entry prompts.
  exe->gm.DisableAllGrammars();
  queue_prompts(elem);

  if (object == NULL)
    throw VXIException::InterpreterEvent(EV_UNSUPPORT_OBJECT);

  // (3) Get the basic parameters.
  vxistring itemname;
  elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);

  // (3.1) Create a new property map.
  VXIMapHolder properties;
  if (properties.GetValue() == NULL) throw VXIException::OutOfMemory();
  exe->properties.GetProperties(properties);
  
  // (3.2) Add the attributes as properties.
  vxistring arg;
  elem.GetAttribute(ATTRIBUTE_CLASSID, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_CLASS_ID, arg);
  elem.GetAttribute(ATTRIBUTE_CODEBASE, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_CODE_BASE, arg);
  elem.GetAttribute(ATTRIBUTE_CODETYPE, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_CODE_TYPE, arg);
  elem.GetAttribute(ATTRIBUTE_DATA, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_DATA, arg);
  elem.GetAttribute(ATTRIBUTE_TYPE, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_TYPE, arg);
  elem.GetAttribute(ATTRIBUTE_ARCHIVE, arg);
  if (!arg.empty()) AddParamValue(properties, OBJECT_ARCHIVE, arg);

  // (4) Search for parameters defined within the object tag.
  VXIMapHolder parameters(CollectParams(elem, true));

  // (5) Play any queued prompts.
  bool playedBargeinDisabledPrompt;
  pm->Play(&playedBargeinDisabledPrompt);

  // (6) Call the platform dependent implementation.
  VXIValue * resultObj = NULL;
  VXIobjResult rc = object->Execute(object, properties.GetValue(),
                                    parameters.GetValue(), &resultObj);

  // (7) Process result code.

  // (7.1) Handle error results.
  switch (rc) {
  case VXIobj_RESULT_SUCCESS:
    break;
  case VXIobj_RESULT_OUT_OF_MEMORY:
    throw VXIException::OutOfMemory();
  case VXIobj_RESULT_UNSUPPORTED:
    throw VXIException::InterpreterEvent(EV_UNSUPPORT_OBJECT);
  default:
    log->LogError(460, SimpleLogger::MESSAGE, L"unexpected return value");
    throw VXIException::InterpreterEvent(EV_ERROR_OBJECT);
  }

  // (8) Process returned object.
  exe->script.SetValue(itemname, resultObj);
  VXIValueDestroy(& resultObj);

  // (9) Done.
  log->LogDiagnostic(2, L"VXI::object_element - done");

  ProcessFilledElements(itemname, elem.GetParent());
}


// Field or Initial item
//
void VXI::field_element(const VXMLElement & form, const VXMLElement & itemNode)
{
  log->LogDiagnostic(2, L"VXI::field_element()");

  exe->properties.SetProperties(itemNode, FIELD_PROP, VXIMapHolder());

  // (1) Disable any existing grammars and play entry prompts.
  queue_prompts(itemNode);      // calls platform
  exe->gm.DisableAllGrammars();

  // (2.1) Get form name.
  vxistring formName;
  form.GetAttribute(ATTRIBUTE__ITEMNAME, formName);

  // (2.2) Get dialog name
  vxistring itemName;
  itemNode.GetAttribute(ATTRIBUTE__ITEMNAME, itemName);

  // (2.3) Is this dialog modal?
  vxistring modal;
  itemNode.GetAttribute(ATTRIBUTE_MODAL, modal);
  bool isModal = false;
  if (modal.length() != 0) isModal = (modal == L"true");

  // (2.4) Finally, do the recognition.

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::field_element - activating grammars for "
      L"form = '" << formName << L"' formitem = '" << itemName << L"'";
    log->EndDiagnostic();
  }

  DoRecognition(itemNode,exe->documentName, formName, itemName,
                exe->properties, pm->GetMillisecondTimeout(), isModal);

  log->LogError(999, SimpleLogger::MESSAGE, L"unexpected exit from field");
  throw VXIException::Fatal();
}


/* 
 * The description of <transfer> a little unclear on result handling.
 *  The are 2 simultaeous mechanisms for handling error conditions
 *  return values in item var, and event throwing (with potential races between
 *  them) but apparently no mechanism to describe success condition.
 * We will do the following:
 *  If attempt to transfer was unsuccessful, set field var to error code
 *  If transfer was successful and non-bridging, set field var to 'transferred'
 *   and THEN throw disconnent.transfer event
 *  If transfer was successful and bridging:
 *    If far_end_disconnect set field var and continue in FIA
 *    If our user hung up or was otherwise disconnected, throw hangup event
 *  We also need return policy for maxtime. Here we will throw a 
 *    telephone.bridge.timeout event. Which will probably log the event and
 *    exit, cleaning up the session. (Ideally the user could be tranferred
 *    without tied up local resources and then transfered back into old session
 *    with return values. However, this will probably require a server-side
 *    implementation).
 */
void DummyTransferResultDestroyFunc(VXIrecTransferResult **result)
{ return; }

void VXI::transfer_element(const VXMLElement & form, const VXMLElement & elem)
{
  log->LogDiagnostic(2, L"VXI::transfer_element()");

  // (1) Set properties for this form item.
  exe->properties.SetProperties(elem, FIELD_PROP, VXIMapHolder());

  // (2) Disables any existing grammars and queue entry prompts.
  exe->gm.DisableAllGrammars();
  queue_prompts(elem);

  // (3) Get the basic parameters.

  // (3.1) Get destination
  vxistring dest;
  elem.GetAttribute(ATTRIBUTE_DEST, dest);
  if (dest.empty()) {
    elem.GetAttribute(ATTRIBUTE_DESTEXPR, dest);
    if (!dest.empty())
      exe->script.EvalScriptToString(dest, dest);
  }

  // (3.2) Get transfer type
  vxistring type;
  elem.GetAttribute(ATTRIBUTE_BRIDGE, type);
  if (!type.empty()){
    if (type == L"true")
      type = ATTRVAL_TRANSFERTYPE_BRIDGE;
	else
      type = ATTRVAL_TRANSFERTYPE_BLIND;
  }
  else {
    elem.GetAttribute(ATTRIBUTE_TYPE, type);
    if (type.empty()){
      type = ATTRVAL_TRANSFERTYPE_BLIND;
    }
  }

  bool isBlind = type == ATTRVAL_TRANSFERTYPE_BLIND;

  // (3.3) transferaudio
  vxistring transferaudio;
  elem.GetAttribute(ATTRIBUTE_TRANSFERAUDIO, transferaudio);

  // (4) Get transfer properties
  VXIMapHolder properties(exe->gm.GetRecProperties(exe->properties, 0));
  if (properties.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (4.1) Get general properties
  exe->properties.GetProperties(properties);

  // (4.2) Get timeout info
  if (type != ATTRVAL_TRANSFERTYPE_BLIND) {
    int time;

    // (4.2.1) Maximum call duration
    vxistring maxtime;
    elem.GetAttribute(ATTRIBUTE_MAXTIME, maxtime);
    if (!maxtime.empty() ) {
      if(!exe->properties.ConvertTimeToMilliseconds(*log, maxtime, time))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
      AddParamValue(properties, TEL_MAX_CALL_TIME, time);
    }

    // (4.2.2) Timeout on connection attempts
    vxistring connecttime;
    elem.GetAttribute(ATTRIBUTE_CONNECTTIME, connecttime);
    if (!connecttime.empty() ) {
      if(!exe->properties.ConvertTimeToMilliseconds(*log, connecttime, time))
        throw VXIException::InterpreterEvent(EV_ERROR_SEMANTIC);
      AddParamValue(properties, TEL_CONNECTTIMEOUT, time);
    }
  }

  // (4.3) aai information
  vxistring aai;
  elem.GetAttribute(ATTRIBUTE_AAI, aai);
  if (aai.empty()) {
    elem.GetAttribute(ATTRIBUTE_AAIEXPR, aai);
    if (!aai.empty())
      exe->script.EvalScriptToString(aai, aai);
  }
  if( !aai.empty() )
    AddParamValue(properties, TEL_TRANSFER_DATA, aai);

  // (4.4) Transfer "type"
  AddParamValue(properties, TEL_TRANSFER_TYPE, type);

  // (6) Activate grammars (modal)
  vxistring formName;
  form.GetAttribute(ATTRIBUTE__ITEMNAME, formName);
  vxistring itemname;
  elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);

  bool grammarsEnabled = exe->gm.EnableGrammars(exe->documentName, formName, 
                                                itemname, properties, true, exe->script, exe->properties);

  // (7) Is this line still active?
  CheckLineStatus();

  // (8) Set up the shadow variables.
  vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + itemname + L"$";
  exe->script.EvalScript(shadowbase + L" = new Object();");
  exe->script.EvalScript(shadowbase + L".duration = 0");

  // At this point, all properties are loaded, prompts queued, and 
  // grammars activated.

  // (9) Do the transfer
  VXItelResult telResult;
  VXIunsigned transferDuration = 0;
  VXItelTransferStatus transferStatus = VXItel_TRANSFER_UNKNOWN;

  // (9.1) Do a HotwordTransfer if the platform supports it.
  if (!isBlind && rec->SupportsHotwordTransfer(rec, properties.GetValue(), dest.c_str())){
    log->LogDiagnostic(2, L"VXI::transfer_element - performing HotwordTransfer");

    // (9.1.1) Play the queued prompts
    bool playedBargeinDisabledPrompt;
    pm->Play(&playedBargeinDisabledPrompt);

	// (9.1.2) queue/play fetchaudio
    if (!transferaudio.empty()) {
      // Get All properties for fetching audio
      VXIMapHolder fillerProp;
      exe->properties.GetFetchobjBase(fillerProp);
      exe->properties.GetProperties(fillerProp);
      pm->PlayFiller(transferaudio, fillerProp);
    }

    // (9.1.3) Is this line still active?
    CheckLineStatus();

	// (9.1.4) Perform the hotword transfer
    VXIrecTransferResult * xferData = NULL;
    VXIrecResult result = rec->HotwordTransfer(rec, tel, properties.GetValue(), 
                                             dest.c_str(), &xferData);

    switch (result) {
    case VXIrec_RESULT_SUCCESS:
      if (xferData == NULL) {
        log->LogError(440, SimpleLogger::MESSAGE,L"transfer_element: no result structure returned");
        throw VXIException::InterpreterEvent(EV_ERROR_TRANSFER);
      }

      transferDuration = xferData->duration;
      transferStatus = xferData->status;

      try {
        if (xferData->xmlresult != NULL)
          ProcessRecognitionResult(xferData, exe->properties);
      }
      catch (const AnswerInformation &) {
        transferStatus = VXItel_TRANSFER_NEAR_END_DISCONNECT;
		SetFieldRecordUtterance(exe->currentFormItem);
        exe->script.EvalScript(shadowbase + 
                       L".utterance = application.lastresult$[0].utterance");
        exe->script.EvalScript(shadowbase + 
                       L".inputmode = application.lastresult$[0].inputmode");
      }
      catch (VXIException::InterpreterEvent & e) {
        // ignore noinput and nomatch cases (shouldn't happen with hotword,
        // but just in case).
        if ((e.GetValue() != EV_NOINPUT) && (e.GetValue() != EV_NOMATCH)) {    
          if( xferData ) {
            (xferData->Destroy)(&xferData);
            xferData = NULL;
          }
          log->LogError(440, SimpleLogger::MESSAGE, L"unexpected return value");
          throw VXIException::InterpreterEvent(EV_ERROR_TRANSFER);
        }
      }
      catch (...) {
        if( xferData ) {
          (xferData->Destroy)(&xferData);
          xferData = NULL;
        }
        throw;
      }

      if( xferData ) {
        (xferData->Destroy)(&xferData);
        xferData = NULL;
      }
      telResult = VXItel_RESULT_SUCCESS;
      break;

    default:
      // throw a specific event for the error
      if (result == VXIrec_RESULT_UNSUPPORTED){
        if( type == ATTRVAL_TRANSFERTYPE_BRIDGE )
          throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_BRIDGE);
        else if( type == ATTRVAL_TRANSFERTYPE_CONSULTATION )
          throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_CONSULTATION);
      }

      GrammarManager::ThrowSpecificEventError(result, GrammarManager::TRANSFER);
      // throw an internal error if specific event cannot be thrown
      log->LogError(441, SimpleLogger::MESSAGE, L"unexpected return value");
      throw VXIException::InterpreterEvent(EV_ERROR_TRANSFER);
    }
  }
  // (9.2) If the platform doesn't support HotwordTransfer, we have to 
  // handle caller interruption ourselves.  There is a tradeoff for not
  // supporting HotwordTransfer though... Recognition will be handled 
  // during prompt playback, but is stopped just prior starting any 
  // fetchaudio, which happens immediately prior to starting the transfer.  
  // This means the caller will be able to interrupt during prompt playback, 
  // but not during connection setup, or anytime during the call (in the 
  // case of bridged transfers).
  else {
    log->LogDiagnostic(2, L"VXI::transfer_element - starting transfer");
    
    bool doTransfer = true;

    try {
      // (9.2.1) If there are active grammars, we'll listen for input while
      // the prompts are playing.  Otherwise, we'll just wait for the prompts 
      // to complete.
      if (grammarsEnabled)
        do_recognition(properties, exe->properties);
      else
        pm->WaitAndCheckError();
    }
    catch (const AnswerInformation &) {
      doTransfer = false;
	  SetFieldRecordUtterance(exe->currentFormItem);
      transferStatus = VXItel_TRANSFER_NEAR_END_DISCONNECT;
      exe->script.EvalScript(shadowbase + 
                       L".utterance = application.lastresult$[0].utterance");
      exe->script.EvalScript(shadowbase + 
                       L".inputmode = application.lastresult$[0].inputmode");
    }
    catch (VXIException::InterpreterEvent & e) {
      // ignore noinput and nomatch cases.  In fact, since our timeout (above)
      // is set to zero, we would expect a noinput if the caller didn't interrupt
      // the prompts.
      if ((e.GetValue() != EV_NOINPUT) && (e.GetValue() != EV_NOMATCH)) {
        throw;
      }
    }
    catch (...) {
      throw VXIException::InterpreterEvent(EV_ERROR_TRANSFER);
    }

    // (9.2.2) Perform the transfer if the caller didn't cancel it by
    // matching a grammar.
    if (doTransfer) {
      // (9.2.2.1) Start fetchaudio
      if (!isBlind && !transferaudio.empty()) {
        VXIMapHolder fillerProp;
        exe->properties.GetFetchobjBase(fillerProp);
        exe->properties.GetProperties(fillerProp);
        pm->PlayFiller(transferaudio, fillerProp);
      }

      // (9.2.2.2) Perform the transfer
      VXIMap *rawXferInfo = NULL;
      if (type == ATTRVAL_TRANSFERTYPE_BRIDGE) {
        log->LogDiagnostic(2, L"VXI::transfer_element - performing bridge transfer");
        telResult = tel->TransferBridge(tel, properties.GetValue(),
                                        dest.c_str(), &rawXferInfo);
      }
      else if (type == ATTRVAL_TRANSFERTYPE_CONSULTATION) {
        log->LogDiagnostic(2, L"VXI::transfer_element - performing consultation transfer");
        telResult = tel->TransferConsultation(tel, properties.GetValue(),
                                              dest.c_str(), &rawXferInfo);
      }
      else {
        log->LogDiagnostic(2, L"VXI::transfer_element - performing blind transfer");
        telResult = tel->TransferBlind(tel, properties.GetValue(),
                                       dest.c_str(), &rawXferInfo);
      }

      VXIMapHolder xferInfo(rawXferInfo);

      // (9.2.2.3) Throw any transfer error events
      ThrowTransferEvent(telResult, type);

      // (9.2.2.4) translate the VXIMap return values
      const VXIValue * v = VXIMapGetProperty(xferInfo.GetValue(), TEL_TRANSFER_STATUS);
      if (v != NULL) {
        if (VXIValueGetType(v) == VALUE_INTEGER) {
          long temp = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v));
          transferStatus = VXItelTransferStatus(temp);

          // if it's a blind transfer, force the value to either unknown or
          // caller hangup.
          if (isBlind && (transferStatus != VXItel_TRANSFER_CALLER_HANGUP))
            transferStatus = VXItel_TRANSFER_UNKNOWN;
	    }
        else {
          vxistring message(TEL_TRANSFER_STATUS);
          message += L" must be defined with type integer";
          log->LogError(440,SimpleLogger::MESSAGE, message.c_str());
        }
      }

	  if (!isBlind) {
        v = VXIMapGetProperty(xferInfo.GetValue(), TEL_TRANSFER_DURATION);
        if (v != NULL){
          if (VXIValueGetType(v) == VALUE_INTEGER)
            transferDuration=VXIIntegerValue(reinterpret_cast<const VXIInteger*>(v));
          else {
            vxistring message(TEL_TRANSFER_DURATION);
            message += L" must be defined with type integer";
            log->LogError(440,SimpleLogger::MESSAGE, message.c_str());
          }
        }
	  }
    }
  } 

  // (10) Process results

  // (10.1) Set duration shadow var.
  if (transferDuration > 0) {
    // Convert the duration from milliseconds to seconds and add it.
    VXIFloat * seconds = VXIFloatCreate(VXIflt32(transferDuration / 1000.0));
    if (seconds == NULL) throw VXIException::OutOfMemory();

    try {
      vxistring shadow = shadowbase + L".duration";
      exe->script.SetValue(shadow, reinterpret_cast<const VXIValue *>(seconds));
    }
    catch (...) {
      VXIFloatDestroy(&seconds);
      throw;
    }
    VXIFloatDestroy(&seconds);
  }

  // (10.2) Handle transfer status result.
  switch (transferStatus) {
  case VXItel_TRANSFER_CALLER_HANGUP:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_HANGUP);
  case VXItel_TRANSFER_CONNECTED:
    if (type == ATTRVAL_TRANSFERTYPE_CONSULTATION)
      throw VXIException::InterpreterEvent(EV_TELEPHONE_TRANSFER);
    exe->script.SetString(itemname, L"unknown");
    break;
  case VXItel_TRANSFER_BUSY:
    exe->script.SetString(itemname, L"busy");
    break;
  case VXItel_TRANSFER_NOANSWER:
    exe->script.SetString(itemname, L"noanswer");
    break;
  case VXItel_TRANSFER_NETWORK_BUSY:
    exe->script.SetString(itemname, L"network_busy");
    break;
  case VXItel_TRANSFER_NEAR_END_DISCONNECT:
    exe->script.SetString(itemname, L"near_end_disconnect");
    break;
  case VXItel_TRANSFER_FAR_END_DISCONNECT:
    exe->script.SetString(itemname, L"far_end_disconnect");
    break;
  case VXItel_TRANSFER_NETWORK_DISCONNECT: 
    exe->script.SetString(itemname, L"network_disconnect");
    break;
  case VXItel_TRANSFER_MAXTIME_DISCONNECT:
    exe->script.SetString(itemname, L"maxtime_disconnect");
    break;
  case VXItel_TRANSFER_UNKNOWN:
  default:
    if (type == ATTRVAL_TRANSFERTYPE_BLIND)
      throw VXIException::InterpreterEvent(EV_TELEPHONE_TRANSFER);
    exe->script.SetString(itemname, L"unknown");
    break;
  }

  // (11) Done.
  log->LogDiagnostic(2, L"VXI::transfer_element - transfer done");
  ProcessFilledElements(itemname, form);
}

void VXI::ThrowTransferEvent(VXItelResult telResult, const vxistring &type)
{
  // throw a specific event for the error
  switch (telResult) {
  case VXItel_RESULT_SUCCESS: break;
  case VXItel_RESULT_OUT_OF_MEMORY:
    throw VXIException::OutOfMemory();
  case VXItel_RESULT_CONNECTION_NO_AUTHORIZATION:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_NOAUTHORIZ);
  case VXItel_RESULT_CONNECTION_BAD_DESTINATION:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_BAD_DEST);
  case VXItel_RESULT_CONNECTION_NO_ROUTE:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_NOROUTE);
  case VXItel_RESULT_CONNECTION_NO_RESOURCE:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_NORESOURCE);
  case VXItel_RESULT_UNSUPPORTED_URI:
    throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_URI);
  case VXItel_RESULT_UNSUPPORTED:
    if( type == ATTRVAL_TRANSFERTYPE_BRIDGE )
        throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_BRIDGE);
    else if( type == ATTRVAL_TRANSFERTYPE_CONSULTATION )
        throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_CONSULTATION);
    throw VXIException::InterpreterEvent(EV_TELEPHONE_UNSUPPORT_BLIND);
  default:
    log->LogError(440, SimpleLogger::MESSAGE,L"unexpected return value");
    throw VXIException::InterpreterEvent(EV_ERROR_TRANSFER);
  }        
}


// Menu element as field
//
void VXI::menu_element(const VXMLElement & menuNode)
{
  log->LogDiagnostic(2, L"VXI::menu_element()");

  exe->properties.SetProperties(menuNode, FIELD_PROP, VXIMapHolder());

  // (1) Disable any existing grammars and play entry prompts.
  queue_prompts(menuNode);
  exe->gm.DisableAllGrammars();

  // (2) Do the recognition.
  vxistring menuName;
  menuNode.GetAttribute(ATTRIBUTE__ITEMNAME, menuName);

  if (log->IsLogging(2)) {
    log->StartDiagnostic(2) << L"VXI::menu_element - activating grammars for "
      L"menu = '" << menuName << L"'";
    log->EndDiagnostic();
  }

  DoRecognition(menuNode,exe->documentName, menuName, menuName,
                exe->properties, pm->GetMillisecondTimeout(), false);

  log->LogError(999, SimpleLogger::MESSAGE, L"unexpected exit from menu");
  throw VXIException::Fatal();
}


// Collect <param> elements into a VXIMap.  This is used by both the
// <subdialog> and <object> elements.
//
VXIMap * VXI::CollectParams(const VXMLElement & doc, bool isObject)
{
  VXIMapHolder obj;
  if (obj.GetValue() == NULL) throw VXIException::OutOfMemory();

  // (1) Walk through the children on this node.
  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    // (1.1) Ignore anything which isn't a <param>
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    if (elem.GetName() != NODE_PARAM) continue;

    // (1.2) <param> elements must have names.  This is enforced by the DTD.
    vxistring name;
    if (!elem.GetAttribute(ATTRIBUTE_NAME, name) || name.empty()) continue;

    // (1.3) For <object>, check if the value type is "ref" or "data".
    // If "ref", the param properties must be embedded in another map.
    VXIbool embedMap = false;
    VXIMap * properties = NULL;
    if (isObject) {
      vxistring vtype;
      elem.GetAttribute(ATTRIBUTE_VALUETYPE, vtype);
      if (vtype == L"ref") {
        embedMap = true;

        // Create the new properties map and set the VALUETYPE 
        // property (for now, always "ref").
        properties = VXIMapCreate();
        if (properties == NULL) continue;
        VXIString * val = VXIStringCreate(vtype.c_str());
        if (val == NULL) throw VXIException::OutOfMemory();
        VXIMapSetProperty(properties, OBJECT_VALUETYPE, 
                          reinterpret_cast<VXIValue *>(val));
      }
    }

    vxistring value;
    elem.GetAttribute(ATTRIBUTE_VALUE, value);
    if (!value.empty()) {
      VXIString * val = VXIStringCreate(value.c_str());
      if (val == NULL) throw VXIException::OutOfMemory();
      if (embedMap)
        VXIMapSetProperty(properties, OBJECT_VALUE,
                          reinterpret_cast<VXIValue *>(val));
      else
        VXIMapSetProperty(obj.GetValue(), name.c_str(),
                          reinterpret_cast<VXIValue *>(val));
    }
    else {
      elem.GetAttribute(ATTRIBUTE_EXPR, value);
      if (value.empty()) continue;
      VXIValue * val = exe->script.GetValue(value);
      if (val == NULL) continue;
      if (embedMap)
        VXIMapSetProperty(properties, OBJECT_VALUE, val);
      else
        VXIMapSetProperty(obj.GetValue(), name.c_str(), val);
    }

    if (embedMap) {
      vxistring mime;
      elem.GetAttribute(ATTRIBUTE_TYPE, mime);
      if (!mime.empty()) {
        VXIString * val = VXIStringCreate(mime.c_str());
        if (val == NULL) throw VXIException::OutOfMemory();
        VXIMapSetProperty(properties, OBJECT_TYPE, 
                          reinterpret_cast<VXIValue *>(val));
      }

      // Finally, embed the properties map into the original map
      VXIMapSetProperty(obj.GetValue(), name.c_str(), 
                        reinterpret_cast<VXIValue *>(properties));
    }
  }

  return obj.Release();
}


/*
// This is the where recognition occurs and results are processed.  The outline
// looks something like this:
//
// 1) Call VXIrecInterface::Recognize and handle errors.
// 2) Handle <link> & <choice> (where the actual result doesn't matter).
// 3) Extract the recognition results and set shadow variables.
// 4) Handle <filled>.
//
// The grammars must be activated before calling this function.
*/
void VXI::DoRecognition(const VXMLElement & item,
                        const vxistring & documentID,
                        const vxistring & dialogName,
                        const vxistring & fieldName,
                        const PropertyList & propertyList,
                        int timeout,
                        bool isModal)
{
  log->LogDiagnostic(2, L"VXI::DoRecognition()");

  // (1) Handle grammars.

  VXIMapHolder properties(exe->gm.GetRecProperties(propertyList, timeout));
  if (properties.GetValue() == NULL) throw VXIException::OutOfMemory();

  if (!exe->gm.EnableGrammars(documentID, dialogName, fieldName,
                              properties, isModal, exe->script, exe->properties))
    throw VXIException::InterpreterEvent(EV_ERROR_NO_GRAMMARS);

  do_recognition(properties, propertyList);
}

void VXI::do_recognition(VXIMapHolder &properties, const PropertyList & propertyList)
{
  // (2) Is the line still active?
  CheckLineStatus();

  // (3) Play prompts.
  bool playedBargeinDisabledPrompt;
  pm->Play(&playedBargeinDisabledPrompt);

  VXIrecRecognitionResult *answer = NULL;
  try {
    // (4) Do a recognition.
    GrammarManager::RecResult recResult = exe->gm.Recognize(properties, answer);
  
    // (4.1) Is the line still active?
    CheckLineStatus();
  
    // (4.2) Throw an event if an error occurred.
    switch (recResult) {
    case GrammarManager::Success:
      break;
    case GrammarManager::InternalError: // Invalid result from Recognize
      throw VXIException::Fatal();
    default:
      log->LogError(999, SimpleLogger::MESSAGE,
                    L"unexpected value from GrammarManager::Recognize");
      throw VXIException::Fatal();
    }

    // (5) Analyze the result
    ProcessRecognitionResult(answer, propertyList);
    
    // (6) Clean up memory
    if (answer != NULL)
      answer->Destroy(&answer);
  }
  catch (...) { 
    // (7) Clean up memory if necessary
    if (answer != NULL)
      answer->Destroy(&answer);
    throw;
  }
}


void VXI::ProcessRecognitionResult(VXIrecRecognitionResult *recResult, const PropertyList & propertyList)
{
   ProcessRecognitionResult(recResult->xmlresult, recResult->utterance, recResult->utteranceDuration,
      recResult->markname, recResult->marktime, propertyList);
}

void VXI::ProcessRecognitionResult(VXIrecRecordResult *recResult, const PropertyList & propertyList)
{
   ProcessRecognitionResult(recResult->xmlresult, NULL, 0,
      recResult->markname, recResult->marktime, propertyList);
}

void VXI::ProcessRecognitionResult(VXIrecTransferResult *recResult, const PropertyList & propertyList)
{
   ProcessRecognitionResult(recResult->xmlresult, recResult->utterance, recResult->utteranceDuration,
      recResult->markname, recResult->marktime, propertyList);
}

void VXI::ProcessRecognitionResult(
    VXIContent  * xmlresult,
    VXIContent  * utterance,
    VXIunsigned   utteranceDuration,
    VXIString   * markname,
    VXIunsigned   marktime,
    const PropertyList & propertyList)
{
  log->LogDiagnostic(2, L"VXI::ProcessRecognitionResult");

  VXIAnswerTranslator translator(exe->script);
  AnswerParser answerParser;
  vxistring grammarID;
  int maxnbest = 1;

  // (1) maxnbest property	
  const VXIchar* j = propertyList.GetProperty(PROP_MAXNBEST);
  if (j != NULL) {
    std::basic_stringstream<VXIchar> nbestStream(j);
    nbestStream >> maxnbest;
    if (maxnbest <= 0 ){
      log->LogError(224, L"maxnbest", j);
      maxnbest = 1;	  	
    }
  }

  // (2) Set application.lastresult$ array.
  VXMLElement answerElement;  // hold best matched answer
  AnswerParser::ParseResult parseRes = answerParser.Parse(*log, translator, exe->gm, maxnbest, xmlresult, answerElement);
  switch (parseRes) {

  case AnswerParser::ParseError:
    log->LogError(432);
    throw VXIException::InterpreterEvent(EV_ERROR_RECOGNITION);

  case AnswerParser::UnsupportedContent:
    log->LogError(430);
    throw VXIException::InterpreterEvent(EV_ERROR_RECOGNITION);

  case AnswerParser::InvalidParameter:
    log->LogError(431);
    throw VXIException::InterpreterEvent(EV_ERROR_RECOGNITION);

  case  AnswerParser::Success: // Success - string contains the grammar id of the best match
  case  AnswerParser::NoMatch:
    break;

  case  AnswerParser::NoInput:
    throw VXIException::InterpreterEvent(EV_NOINPUT);

  default:
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"unexpected result from AnswerParser::Parse");
    throw VXIException::Fatal();
  }

  // (3) Set record utterance vars
  if (utterance != NULL) {
    log->LogDiagnostic(2, L"VXI::ProcessRecognitionResult - set recordutterance");

    exe->script.SetValue(L"application.lastresult$.recording",
                         reinterpret_cast<const VXIValue*>(utterance));

    VXIInteger *size = VXIIntegerCreate(VXIContentGetContentSizeBytes(utterance));
    exe->script.SetValue(L"application.lastresult$.recordingsize", 
      reinterpret_cast<const VXIValue*>(size));
    VXIIntegerDestroy(&size);

    VXIInteger *duration = VXIIntegerCreate(utteranceDuration);
    exe->script.SetValue(L"application.lastresult$.recordingduration", 
      reinterpret_cast<const VXIValue*>(duration));
    VXIIntegerDestroy(&duration);
  }
  else {
    exe->script.EvalScript(L"application.lastresult$.recording = undefined;");
    exe->script.EvalScript(L"application.lastresult$.recordingsize = undefined;");
    exe->script.EvalScript(L"application.lastresult$.recordingduration = undefined;");
  }

  // (4) Set mark vars
  SetMarkVars(markname, marktime);

  // throw the "nomatch" event now (if needed)
  if (parseRes == AnswerParser::NoMatch) {
    SetFieldRecordUtterance(exe->currentFormItem);
	SetFieldMark(exe->currentFormItem);
    throw VXIException::InterpreterEvent(EV_NOMATCH);
  }

  // (5) Copy the 0th element to application.lastresult$

  exe->script.EvalScript(L"application.lastresult$.utterance = "
                         L"application.lastresult$[0].utterance;");
  exe->script.EvalScript(L"application.lastresult$.confidence = "
                         L"application.lastresult$[0].confidence;");
  exe->script.EvalScript(L"application.lastresult$.inputmode = "
                         L"application.lastresult$[0].inputmode;");
  exe->script.EvalScript(L"application.lastresult$.interpretation = "
                         L"application.lastresult$[0].interpretation;");

  // (6) Enforce confidence threshold.

  const VXIchar * threshold = propertyList.GetProperty(PROP_CONFIDENCE);
  if (threshold != NULL &&
      exe->script.TestCondition(vxistring(L"application.lastresult$[0]."
                                          L"confidence < ") + threshold))
    throw VXIException::InterpreterEvent(EV_NOMATCH);

  // (7) Get the associated element.
  if (answerElement == 0)
  {
    // No match found!
    log->LogError(433);
    throw VXIException::InterpreterEvent(EV_ERROR_RECOGNITION);
  }

  // (8) At this point we have success.  Find the action associated with this
  // element.

  log->LogDiagnostic(2, L"VXI::ProcessRecognitionResult - have an answer");

  // (8.1) Find the element in which the grammar lives.

  VXMLElementType name = answerElement.GetName();
  if (name == NODE_GRAMMAR) {
    answerElement = answerElement.GetParent();
    if (answerElement == 0) {
      log->LogError(999, SimpleLogger::MESSAGE,
                    L"could not locate parent of grammar");
      throw VXIException::Fatal();
    }
    name = answerElement.GetName();
  }

  // (8.2) Handle the two goto possibilities (choice & link).

  if (name == NODE_CHOICE || name == NODE_LINK) {
    // Decide whether to treat this as an event or a goto.
    vxistring next;
    answerElement.GetAttribute(ATTRIBUTE_NEXT, next);
    if (next.empty())
      answerElement.GetAttribute(ATTRIBUTE_EXPR, next);
    if (next.empty()) {
      // If this generates an event, 'event' or 'eventexpr' must be specified.
      answerElement.GetAttribute(ATTRIBUTE_EVENT, next);
      if (next.empty())
        answerElement.GetAttribute(ATTRIBUTE_EVENTEXPR, next);

      if (!next.empty()) {
        // At this point, the semantics are identical to throw.  
        const VXMLElement & currentActiveDiag = answerElement.GetParent();
                              
        throw_element(answerElement, currentActiveDiag);
      }
    }

    // Otherwise, the semantics are identical to goto.
    goto_element(answerElement);
  }

  // (8.3) Find the form and item associated with this response.

  // (8.3.1) Find the enclosing form.
  VXMLElement answerDialog = answerElement;
  name = answerDialog.GetName();
  if (name != NODE_FORM) {
    answerDialog = answerDialog.GetParent();
    if (answerDialog == 0) {
      log->LogError(999, SimpleLogger::MESSAGE,
                    L"could not locate form associated with answer");
      throw VXIException::Fatal();
    }
    name = answerDialog.GetName();
  };

  // (8.3.2) Perform a final sanity check to verify that the results match our
  // expection.
  if (name != NODE_FORM ||
      (!IsFormItemNode(answerElement) && answerElement != answerDialog))
  {
    log->LogError(999, SimpleLogger::MESSAGE,
                  L"bad grammar location from recognition");
    throw VXIException::Fatal();
  }

  AnswerInformation temp(answerElement, answerDialog);
  throw temp;
}

void VXI::SetMarkVars(VXIString *markname, VXIunsigned marktime)
{
  if (markname != NULL) {
    exe->script.SetValue(L"application.lastresult$.markname", 
      reinterpret_cast<const VXIValue*>(markname));

    VXIInteger *mtime = VXIIntegerCreate(marktime);
    exe->script.SetValue(L"application.lastresult$.marktime", 
      reinterpret_cast<const VXIValue*>(mtime));
    VXIIntegerDestroy(&mtime);
  }
  else {
    exe->script.EvalScript(L"application.lastresult$.markname = undefined;");
    exe->script.EvalScript(L"application.lastresult$.marktime = undefined;");
  }
}

void VXI::SetFieldRecordUtterance(const VXMLElement & answerElement)
{
  if(answerElement == 0) return;

  VXMLElementType nodeName = answerElement.GetName();
  if (nodeName == NODE_FIELD     || nodeName == NODE_RECORD ||
      nodeName == NODE_TRANSFER  || nodeName == NODE_INITIAL ) 
  {
    vxistring fieldName;
    answerElement.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);
    SetFieldRecordUtterance(fieldName);
  }
}

void VXI::SetFieldRecordUtterance(vxistring & fieldName)
{
  log->StartDiagnostic(2) << L"VXI::SetFieldRecordUtterance: " << fieldName.c_str(); 
  log->EndDiagnostic();

  vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + fieldName + L"$";

  if (!exe->script.IsVarDeclared(shadowbase))
    exe->script.EvalScript(shadowbase + L" = new Object();");

  exe->script.EvalScript(shadowbase + L".recording = "
                         L"application.lastresult$.recording;");
  exe->script.EvalScript(shadowbase + L".recordingsize = "
                         L"application.lastresult$.recordingsize;");
  exe->script.EvalScript(shadowbase + L".recordingduration = "
                         L"application.lastresult$.recordingduration;");
}

void VXI::SetFieldMark(const VXMLElement & answerElement)
{
  if(answerElement == 0) return;

  VXMLElementType nodeName = answerElement.GetName();
  if (nodeName == NODE_FIELD     || nodeName == NODE_RECORD ||
      nodeName == NODE_TRANSFER  || nodeName == NODE_INITIAL ) 
  {
    vxistring fieldName;
    answerElement.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);
    SetFieldMark(fieldName);
  }
}

void VXI::SetFieldMark(vxistring & fieldName)
{
  log->StartDiagnostic(2) << L"VXI::SetFieldMark: " << fieldName.c_str(); 
  log->EndDiagnostic();

  vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + fieldName + L"$";

  vxistring test = shadowbase + L" == undefined";
  if (exe->script.TestCondition(test))
    exe->script.EvalScript(shadowbase + L" = new Object();");

  exe->script.EvalScript(shadowbase + L".markname = "
                         L"application.lastresult$.markname;");
  exe->script.EvalScript(shadowbase + L".marktime = "
                         L"application.lastresult$.marktime;");
}

void VXI::HandleRemoteMatch(const VXMLElement & answerForm,
                            const VXMLElement & answerElement)
{
  log->LogDiagnostic(2, L"VXI::HandleRemoteMatch()");

  // (1) Determine the field name & whether or not this is mixed initiative.
  bool mixedInitiative = (answerForm == answerElement ||
                          answerElement.GetName() == NODE_INITIAL);

  // (2) Handle field level grammars.
  TokenList filledSlots;

  if (!mixedInitiative) {
    vxistring slot;
    vxistring fieldName;
    answerElement.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);
    answerElement.GetAttribute(ATTRIBUTE_SLOT, slot);
    if (slot.empty()) slot = fieldName;

    if (slot.empty()) {
      // This shouldn't be possible.  The itemname should always be defined.
      log->LogError(999, SimpleLogger::MESSAGE,
                    L"unable to obtain slot name in rec result processing");
      throw VXIException::Fatal();
    }

    if (log->IsLogging(2)) {
      log->StartDiagnostic(2) << L"VXI::HandleRemoteMatch - non-mixed initiative: "
                              << L"Processing (" << slot << L")";
      log->EndDiagnostic();
    }

    vxistring expr(L"typeof application.lastresult$[0].interpretation.");
    expr += slot;      
    expr += L" != 'undefined'";
    bool slotmatch = exe->script.TestCondition(expr);
    
    vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + fieldName + L"$";
    exe->script.EvalScript(shadowbase + L" = new Object();");

    exe->script.EvalScript(shadowbase + L".confidence = "
                           L"application.lastresult$[0].confidence;");
    exe->script.EvalScript(shadowbase + L".utterance = "
                           L"application.lastresult$[0].utterance;");
    exe->script.EvalScript(shadowbase + L".inputmode = "
                           L"application.lastresult$[0].inputmode;");

    if (!slotmatch) {
        exe->script.EvalScript(shadowbase + L".interpretation = "
                            L"application.lastresult$[0].interpretation;");
        exe->script.EvalScript(fieldName +
                            L" = application.lastresult$[0].interpretation;");
    }
    else {
      exe->script.EvalScript(shadowbase + L".interpretation = "
                             L"application.lastresult$[0].interpretation." +
                             slot + L";");
      exe->script.EvalScript(fieldName +
                             L" = application.lastresult$[0].interpretation." +
                             slot + L";");
    }

	SetFieldRecordUtterance(fieldName);
	SetFieldMark(fieldName);

    filledSlots.push_back(fieldName);
  }

  // (3) Handle form level grammars.
  if (mixedInitiative) {
    // Find all <field> elements for this form.
    for (VXMLNodeIterator it(answerForm); it; ++it) {
      VXMLNode child = *it;
      if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
      if (!IsInputItemNode(elem)) continue;

      // (3.1) Find the corresponding field variable and slot names
      vxistring slot;
      vxistring fieldvar;
      elem.GetAttribute(ATTRIBUTE__ITEMNAME, fieldvar);
      elem.GetAttribute(ATTRIBUTE_SLOT, slot);
      if (slot.empty()) slot = fieldvar;

      if (log->IsLogging(2)) {
        log->StartDiagnostic(2) << L"VXI::HandleRemoteMatch - mixed initiative: "
                                << L"Processing (" << slot << L")";
        log->EndDiagnostic();
      }

      // (3.2) Does the answer contain data for this slot?     
      vxistring test(L"typeof application.lastresult$[0].interpretation.");
      test += slot;
      test += L" == 'undefined'";

      if (exe->script.TestCondition(test)) continue;
      slot += L";";

      // (3.3) Assign ECMAScript variables.
      exe->script.EvalScript(fieldvar +
                             L" = application.lastresult$[0].interpretation."
                             + slot);

      vxistring shadowbase = vxistring(SCOPE_Dialog) + L"." + fieldvar + L"$";
      exe->script.EvalScript(shadowbase + L" = new Object();");

      exe->script.EvalScript(shadowbase + L".confidence = "
                             L"application.lastresult$[0].confidence");
      exe->script.EvalScript(shadowbase + L".utterance = "
                             L"application.lastresult$[0].utterance;");
      exe->script.EvalScript(shadowbase + L".inputmode = "
                             L"application.lastresult$[0].inputmode;");
      exe->script.EvalScript(shadowbase + L".interpretation = " + fieldvar);

	  SetFieldRecordUtterance(fieldvar);
      SetFieldMark(fieldvar);

      filledSlots.push_back(fieldvar);
    }
  }

  // (4) In nothing was 'filled', we treat this as a nomatch event.
  if (filledSlots.empty()) {
    log->LogDiagnostic(2, L"VXI::HandleRemoteMatch - no matches");
    return;
  }

  // (5) Do the filled processing.
  ProcessFilledElements(filledSlots, answerForm);
}


void VXI::ProcessFilledElements(const vxistring & filled,
                                const VXMLElement & element)
{
  TokenList name;
  name.push_back(filled);
  ProcessFilledElements(name, element);
}


void VXI::ProcessFilledElements(TokenList & filledSlots,
                                const VXMLElement & answerForm)
{
  log->LogDiagnostic(2, L"VXI::ProcessFilledElements - start");

  // (1) Sort the filledSlots.  We'll need this later.
  if (filledSlots.size() != 1) {
    std::sort(filledSlots.begin(), filledSlots.end());
    if (std::unique(filledSlots.begin(), filledSlots.end()) 
        != filledSlots.end())
    {
      log->LogError(416);
      throw VXIException::Fatal();
    }
  }

  // (2) Clear the current form item.  This will cause all events to be
  // generated at the dialog level.
  exe->currentFormItem = VXMLElement();

  // (3) After a successful recognition, the <initial> elements are all
  //     disabled.  This must happen BEFORE executing <filled> elements.

  for (VXMLNodeIterator it2(answerForm); it2; ++it2) {
    VXMLNode child2 = *it2;

    if (child2.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child2);
    if (elem.GetName() == NODE_INITIAL) {
      vxistring itemName;
      elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemName);
      exe->script.SetVar(itemName, L"true");
    }
  }

  // (4) Process <filled> elements

  vxistring ALLNAMES;

  // (4.1) Get all names
  for (VXMLNodeIterator iter(answerForm); iter; ++iter) {
    VXMLNode child = *iter;
    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    if (elem.GetName() == NODE_INITIAL || elem.GetName() == NODE_BLOCK) continue;
    vxistring id;
    if (!elem.GetAttribute(ATTRIBUTE__ITEMNAME, id) || id.empty()) continue;
    if (!ALLNAMES.empty()) ALLNAMES += L' ';
    ALLNAMES += id;
  }

  for (VXMLNodeIterator it(answerForm); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);

    // (4.2) Handle filled elements at the form level.

    if (elem.GetName() == NODE_FILLED) {
      // Reset position to form level.
      if (exe->currentFormItem != 0)
        exe->currentFormItem = VXMLElement();

      vxistring mode;
      vxistring namelist;
      elem.GetAttribute(ATTRIBUTE_MODE, mode);
      elem.GetAttribute(ATTRIBUTE_NAMELIST, namelist);

      if (namelist.empty())
        namelist = ALLNAMES;

      // Find the intersection between the slots & the namelist.
      TokenList names(namelist), intersection;
      std::set_intersection(filledSlots.begin(), filledSlots.end(),
                            names.begin(), names.end(),
                            std::back_inserter(intersection));
      if (intersection.empty()) continue;

      // Test the mode requirements.
      if (mode == L"any") {
        execute_content(elem);
        continue;
      }

      bool doit = true;
      for (TokenList::const_iterator i = names.begin(); i != names.end();++i)
      {
        doit &= exe->script.IsVarDefined(*i);
        if (!doit) break;
      }

      if (doit) {
        execute_content(elem);
        continue;
      }
    } // end <filled> at dialog level

    // (4.3) Locate form items corresponding to filled slots.

    if (!IsFormItemNode(elem)) continue;

    // All these events must be thrown inside the input item.
    exe->currentFormItem = elem;

    TokenList::const_iterator i;
    vxistring itemName;
    elem.GetAttribute(ATTRIBUTE__ITEMNAME, itemName);
    for (i = filledSlots.begin(); i != filledSlots.end(); ++i)
      if ((*i) == itemName) break;
    if (i == filledSlots.end()) continue;

    // (4.4) Look inside for filled elements.

    for (VXMLNodeIterator it(elem); it; ++it) {
      VXMLNode itemChild = *it;

      if (itemChild.GetType() != VXMLNode::Type_VXMLElement) continue;
      const VXMLElement & itemElem
        = reinterpret_cast<const VXMLElement &>(itemChild);
      if (itemElem.GetName() == NODE_FILLED)
        execute_content(itemElem);
    }
  }

  log->LogDiagnostic(2, L"VXI::ProcessFilledElements - done");
}


//#############################################################################
// Prompt related
//#############################################################################

void VXI::queue_prompts(const VXMLElement & doc)
{
  // If an event has occurred, prompt playing may be disabled.  If so, we
  // must re-enable it for the next recognition.
  if (!exe->playingPrompts) {
    log->LogDiagnostic(2, L"VXI::queue_prompts - disabled");
    exe->playingPrompts = true;
    return;
  }

  log->LogDiagnostic(2, L"VXI::queue_prompts()");

  int count = 1;
  int correctcount = 1;

  vxistring itemname;
  doc.GetAttribute(ATTRIBUTE__ITEMNAME, itemname);
  if (itemname.empty())
    log->LogError(999, SimpleLogger::MESSAGE, L"unnamed internal node found");
  else
    count = exe->promptcounts.Increment(itemname);

  typedef std::deque<bool> PROMPTFLAGS;
  PROMPTFLAGS flags;

  // Walk through nodes.  Determine closest match to the count - curiously,
  // only <prompt> elements have an associated count.  By using the PROMPTFLAGS
  // container, we guarentee that we evaluate the conditions exactly once as
  // required by the spec.

  for (VXMLNodeIterator it(doc); it; ++it) {
    VXMLNode child = *it;

    if (child.GetType() != VXMLNode::Type_VXMLElement) continue;
    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
    if (elem.GetName() != NODE_PROMPT) continue;

    // Evaluate the condition for each prompt that is encountered.
    vxistring cond;
    elem.GetAttribute(ATTRIBUTE_COND, cond);
    if (cond.empty())
      flags.push_back(true);
    else
      flags.push_back(exe->script.TestCondition(cond));
    if (!flags.back()) continue;
    
    // Retrieve the associated count.
    vxistring countvar;
    elem.GetAttribute(ATTRIBUTE_COUNT, countvar);
    if (countvar.empty()) continue;

    // Update the best match (if necessary)
    int icount = exe->script.EvalScriptToInt(countvar);
    if (icount <= count && icount > correctcount) correctcount = icount;
  }

  // And walk through once again, playing all matching prompts.  Here we do
  // evaluate the count a second time, but this should be harmless.

  for (VXMLNodeIterator it2(doc); it2; ++it2) {
    VXMLNode child2 = *it2;
    if (child2.GetType() != VXMLNode::Type_VXMLElement) continue;

    const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child2);

    if (elem.GetName() == NODE_PROMPT) {
      bool cond = flags.front();
      flags.pop_front();
      if (!cond) continue;

      vxistring countvar;
      elem.GetAttribute(ATTRIBUTE_COUNT, countvar);
      int icount;
      if (countvar.length() == 0) icount = 1;
      else icount = exe->script.EvalScriptToInt(countvar);

      if (icount == correctcount) {
        // Create new translator for ECMA script & queue it.
        log->LogDiagnostic(2, L"VXI::queue_prompts - playing");
        VXIPromptTranslator translator(exe->script);
        pm->Queue(elem, doc, exe->properties, translator);
      }
    }
  }
}


void VXI::executable_prompt(const VXMLElement & child)
{
  // The 'cond' attribute is still active on prompts in executable content.
  // Handle this special case.

  const VXMLElement & elem = reinterpret_cast<const VXMLElement &>(child);
  if (elem.GetName() == NODE_PROMPT) {
    vxistring cond;
    elem.GetAttribute(ATTRIBUTE_COND, cond);
    if (!cond.empty() && !exe->script.TestCondition(cond))
      return;
  }

  // Create new translator for ECMA script & queue this prompt.
  log->LogDiagnostic(2, L"VXI::executable_prompt - playing");
  VXIPromptTranslator translator(exe->script);
  pm->Queue(child,
            (exe->eventSource==0) ? child.GetParent() : exe->eventSource,
            exe->properties, translator);
}
