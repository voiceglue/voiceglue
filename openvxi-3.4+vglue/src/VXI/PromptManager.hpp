
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

#ifndef _PROMPTMANAGER_H_
#define _PROMPTMANAGER_H_

#include "VXIvalue.h"
#include "VXIprompt.h"
#include "ExecutableContentHandler.hpp"

extern "C" struct VXIpromptInterface;
class PromptTranslator;
class PropertyList;
class SimpleLogger;
class VXMLElement;
class VXMLNode;

/**
 * Interface for accessing ECMA Script functions.
 */
class PromptTranslator {
public:
  /**
   * Evaluates an expression which arises during prompt construction.  The
   * returned value will be freed by the PromptManager.
   */
  virtual VXIValue * EvaluateExpression(const vxistring & expression) = 0;

  /**
   * Sets a local variable in the current scope.
   */
  virtual void SetVariable(const vxistring & name, const vxistring & value) =0;

  /** 
   * Evaluates an expression as a boolean.
   */
  virtual bool TestCondition(const vxistring & script) = 0;

  /** 
   * Evaluates an ECMAScript script.
   */
  virtual void EvalScript(const vxistring & script) = 0;

  PromptTranslator() { }
  virtual ~PromptTranslator() { }
};


/**
 * Handles prompts.
 */
class PromptManager {
public:
  /**
   * This function walks through the document, preloading prompts as necessary.
   *
   * @throw VXIException::OutOfMemory, VXIException::InterpreterEvent
   */
  void PreloadPrompts(const VXMLElement& doc, PropertyList &, PromptTranslator &);

  /**
   * Queues a <prompt> element and process it's child nodes (<audio>,
   * <enumerate>, etc...).
   *
   * @throw VXIException::InterpreterEvent
   */
  void Queue(const VXMLElement & child, const VXMLElement & reference,
             const PropertyList &, PromptTranslator &);

  /**
   * Queues a segment from a known URI.
   * @throw VXIException::InterpreterEvent
   */
  //void Queue(const vxistring & uri);

  /**
   * Waits until the prompts currenly marked for playing are done.  Then starts
   * playing everything remaining in the queue.  This is generally followed by
   * a recognition / record attempt.
   *
   * @throw VXIException::InterpreterEvent
   */
  void Play(bool *playedBarginDisabledPrompt);

  /**
   * Play everything currently in the queue.  The user is unable to barge in.
   */
  void PlayAll();

  /**
   * Waits for playback to complete.
   * @throw VXIException::InterpreterEvent
   */
  void WaitAndCheckError(void);

  /** 
   * Returns: the recognition timeout, in milliseconds, or -1 if none was
   * specified.
   */
  int GetMillisecondTimeout() const;

  /**
   * Start playing fetchaudio.
   *
   * @param src        SSML document
   * @param properties Current VXI & VXML properties
   *
   * @throw VXIException::InterpreterEvent
   */
  void PlayFiller(const vxistring & src, const VXIMapHolder & properties);

public:
  bool ConnectResources(SimpleLogger *, VXIpromptInterface *, ExecutableContentHandler *);
  PromptManager() : log(NULL), prompt(NULL), contentHandler(NULL) { }
  ~PromptManager() { }

private:
  enum BargeIn {
    UNSPECIFIED,
    ENABLED,
    DISABLED
  };

  enum SegmentType {
    SEGMENT_AUDIO,
    SEGMENT_SSML
  };

  void DoPreloadPrompts(const VXMLElement& doc,
                        PropertyList & properties,
                        VXIMapHolder & levelProperties,
                        PromptTranslator & translator);

  bool ProcessPrefetchPrompts(const VXMLNode & node, const VXMLElement& item, 
                      const PropertyList & propertyList,
                      PromptTranslator & translator, 
                      vxistring & sofar);

  void ProcessSegments(const VXMLNode & node,
                       const VXMLElement & item,
                       const PropertyList & propertyList,
                       PromptTranslator & translator,
                       BargeIn,
                       VXIMapHolder & props,
                       vxistring & sofar,
                       vxistring & ssmlHeader,
                       bool & hasPromptData,
                       bool replaceXmlBase);

  // Returns: true - segment successfully queued
  //          false - segment addition failed
  bool AddSegment(SegmentType, const vxistring & data,
                  const VXIMapHolder & properties, BargeIn,
                  bool throwExceptions = true);

  // NOTE: This takes ownership of the VXIValue.
  void AddContent(VXIMapHolder & props,
                  vxistring & sofar,
                  VXIValue * value);

  void ThrowEventIfError(VXIpromptResult rc);
  
private:
  SimpleLogger       * log;
  VXIpromptInterface * prompt;
  ExecutableContentHandler *contentHandler;
  bool enabledSegmentInQueue;
  bool playedBargeinDisabledPrompt;
  int timeout;
};

#endif
