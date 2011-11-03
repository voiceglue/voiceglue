
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

#ifndef _VXI_H
#define _VXI_H

#include "VXIvalue.h"
#include "CommonExceptions.hpp"
#include "VXMLDocument.hpp"
#include "InternalMutex.hpp"
#include "TokenList.hpp"
#include "VXItel.h"
#include "ExecutableContentHandler.hpp"

class AnswerInformation;
class DocumentParser;
class ExecutionContext;
class JumpDoc;
class PromptManager;
class PropertyList;
class RecognitionAnswer;
class SimpleLogger;
extern "C" struct VXIcacheInterface;
extern "C" struct VXIinetInterface;
extern "C" struct VXIjsiInterface;
extern "C" struct VXIpromptInterface;
extern "C" struct VXIrecInterface;
extern "C" struct VXItelInterface;
extern "C" struct VXIobjectInterface;
extern "C" struct VXIrecRecognitionResult;
extern "C" struct VXIrecRecordResult;
extern "C" struct VXIrecTransferResult;

class VXI : public ExecutableContentHandler {
public:
  VXI();  // May throw VXIException::OutOfMemory
  ~VXI();

  // Returns: -2 Fatal error
  //          -1 Out of memory
  //           0 Success
  //           1 Infinite loop suspected.
  //           2 Javascript error
  //           3 Invalid startup documents
  //           4 Stopped
  int Run(const VXIchar * initialDocument, const VXIchar * sessionScript,
          SimpleLogger * log, VXIinetInterface * inet,
          VXIcacheInterface * cache, VXIjsiInterface * jsi,
          VXIrecInterface * rec, VXIpromptInterface * prompt,
          VXItelInterface * tel, VXIobjectInterface * object,
          VXIValue ** result);

  // This sets a flag which may be used to abort Run.
  void DeclareStopRequest(bool doStop);

  bool DeclareExternalEvent(const VXIchar * event, const VXIchar * message);
  bool ClearExternalEventQueue(void);

  enum PropertyID {
    BeepURI,             /// URI to beep audio
    PlatDefaultsURI,     /// URI to platform defaults script
	DefaultAccessControl /// non-zero to allow access when ?access-control? is missing, otherwise 0 to deny
  };

  // Returns: true  - Property set
  //          false - Invalid parameter value
  bool SetRuntimeProperty(PropertyID, const VXIValue * value);

private:
  // these getters expect the proper ID/type.  i.e. Passing 
  // an ID that returns a string to the int overload will 
  // result in the returned value being untouched.
  void GetRuntimeProperty(PropertyID, vxistring &) const;
  void GetRuntimeProperty(PropertyID, int &) const;

  ////////////////////////////////////////////////////////////////////////////
  // Document level functions
  ////////////////////////////////////////////////////////////////////////////

  int RunDocumentLoop(const vxistring & initialDocument,
                      const vxistring & sessionScript,
                      VXIValue ** result);

  void PerformTransition(const VXMLElement & doc, const vxistring & url,
                         VXIMap * submitData = NULL, bool isSubdialog = false, 
                         bool isSubmitElement = false);

  // Finds the named dialog in the document.  If the name is empty, the first
  // item is returned.
  VXMLElement FindDialog(const VXMLElement & doc, const vxistring & name);

  ////////////////////////////////////////////////////////////////////////////
  // Document level support functions
  ////////////////////////////////////////////////////////////////////////////

  // This is responsible for adding a next execution context for the initial
  // document and for <subdialog> elements.  It also initializes the session
  // and language-neutral platform defaults.
  //
  // Returns: true - success
  //          false - failure (stack depth exceeded?)
  bool PushExecutionContext(const vxistring & sessionScript);

  // This undoes PushExecutionContext()
  void PopExecutionContext();

  void InstallDocument(JumpDoc &);

  void ProcessRootScripts(VXMLElement & doc);

  void AttemptDocumentLoad(const vxistring & rawURL,
			   const VXIMapHolder & urlProperties,
			   VXMLDocument & doc,
			   VXIMapHolder & docProperties,
			   bool isDefaults = false,
			   bool isRootApp = false);

  // Recursively walks the document tree and assigns internal names as needed.
  void PrepareDocumentTree(VXMLElement & doc);

private:
  ////////////////////////////////////////////////////////////////////////////
  // Dialog level functions
  ////////////////////////////////////////////////////////////////////////////
  void RunInnerLoop();

  // Either throws an event containing the next form item to execute or
  // simply returns if none is found.
  void DoInnerJump(const VXMLElement & elem, const vxistring & item);

  ////////////////////////////////////////////////////////////////////////////
  // Dialog level support functions
  ////////////////////////////////////////////////////////////////////////////

  // Perform initialization associated with property tags and form level
  // variables.  Reset the event and prompts counts.
  void FormInit(const VXMLElement & form, VXIMapHolder & params);

  // Returns true iff this element is a 'form item'.
  bool IsFormItemNode(const VXMLElement& doc);

  // Returns true iff this element is an 'input item'.
  bool IsInputItemNode(const VXMLElement& doc);
 
private:
  ////////////////////////////////////////////////////////////////////////////
  // Collect Phase and element related.
  ////////////////////////////////////////////////////////////////////////////

  void CollectPhase(const VXMLElement& form, const VXMLElement& item);

  void ProcessReturn(const VXMLElement& form, const VXMLElement& item,
                     VXIMapHolder & result);

  void DoEvent(const VXMLElement & item,
               const VXIException::InterpreterEvent & event);

  // Returns: true - event handled successfully.
  //          false - no handler available.
  bool do_event(const VXMLElement & item,
                const VXIException::InterpreterEvent & event);

  void DoRecognition(const VXMLElement & item,
	                 const vxistring & documentID,
                     const vxistring & dialogName,
                     const vxistring & fieldName,
                     const PropertyList & properties,
                     int timeout,
                     bool isModal);
  void do_recognition(VXIMapHolder &properties, const PropertyList & propertyList);

  void ProcessRecognitionResult(VXIrecRecognitionResult *recResult, const PropertyList & propertyList);
  void ProcessRecognitionResult(VXIrecRecordResult *recResult, const PropertyList & propertyList);
  void ProcessRecognitionResult(VXIrecTransferResult *recResult, const PropertyList & propertyList);
  void ProcessRecognitionResult(VXIContent * result, VXIContent * utterance, VXIunsigned utteranceDuration, 
      VXIString *markname, VXIunsigned marktime,
      const PropertyList & propertyList);

  void HandleRemoteMatch(const VXMLElement & dialog,
                         const VXMLElement & element);

  void SetMarkVars(VXIString *markname, VXIunsigned marktime);
  void SetFieldRecordUtterance(const VXMLElement & answerElement);
  void SetFieldRecordUtterance(vxistring & fieldName);

  void SetFieldMark(const VXMLElement & answerElement);
  void SetFieldMark(vxistring & fieldName);

  void ProcessFilledElements(const vxistring & filled,
                             const VXMLElement & element);

  void ProcessFilledElements(TokenList & filled,
                             const VXMLElement & element);

private:
  void execute_content(const VXMLElement & doc,
                       const VXIMapHolder & vars = VXIMapHolder(NULL),
                       const VXMLElement & activeDialog = 0);
  void executable_prompt(const VXMLElement& child);

// executable content handler
private:
  void executable_element(const VXMLElement& child, const VXMLElement & activeDialog = 0);

  void assign_element(const VXMLElement& elem);
  void clear_element(const VXMLElement& elem);
  void data_element(const VXMLElement & elem);
  void disconnect_element(const VXMLElement& elem);
  void goto_element(const VXMLElement& elem);
  void exit_element(const VXMLElement& elem);
  void if_element(const VXMLElement& elem);
  void log_element(const VXMLElement& elem);
  void reprompt_element(const VXMLElement& doc, const VXMLElement & activeDialog = 0);
  void return_element(const VXMLElement& elem);
  void script_element(const VXMLElement& elem);
  void submit_element(const VXMLElement& elem);
  void throw_element(const VXMLElement& doc, const VXMLElement& activeDialog = 0);
  void var_element(const VXMLElement & elem);

private:
  // form item handlers
  void block_element(const VXMLElement& doc);
  void field_element(const VXMLElement& form, const VXMLElement& field);
  void menu_element(const VXMLElement& doc);
  void object_element(const VXMLElement& doc);
  void record_element(const VXMLElement& form, const VXMLElement& doc);
  void subdialog_element(const VXMLElement& doc);
  void transfer_element(const VXMLElement & form, const VXMLElement& doc);

  // misc handlers
  void meta_element(const VXMLElement & doc);

  // helpers
  VXIMap * CollectParams(const VXMLElement & doc, bool isObject);
  void hangup_log(void);
  void CheckLineStatus(void);
  void ThrowTransferEvent(VXItelResult telResult, const vxistring &type);

private: // All prompt related.
  void queue_prompts(const VXMLElement& doc);

private:
  DocumentParser     * parser;  // owned
  SimpleLogger       * log;
  VXIcacheInterface  * cache;
  VXIinetInterface   * inet;
  VXIrecInterface    * rec;
  VXIjsiInterface    * jsi;
  VXItelInterface    * tel;
  VXIobjectInterface * object;
  PromptManager      * pm;      // owned

  VXIMap             * sdParams;
  VXIMap             * sdResult;
  VXIException::InterpreterEvent * sdEvent;

  bool updateDefaultDoc;
  VXMLDocument domDefaultDoc;

  // Used by Get/Set Property
  InternalMutex mutex;
  vxistring uriPlatDefaults;
  vxistring uriBeep;
  bool      defAccessControl;  // for <data> missing ?access-control?

  // Keep track of current URI
  vxistring uriCurrent;

  int stackDepth;
  ExecutionContext * exe;

  // Event related
  bool lineHungUp;              // Tel. status to track hang-up event
  bool stopRequested;           // Should the interpreter stop ASAP?
  bool haveExternalEvents;      // Are there unprocessed external events?
  TokenList externalEvents;     // Unprocessed external events
  TokenList externalMessages;   // Unprocessed external events (messages)
};

#endif
