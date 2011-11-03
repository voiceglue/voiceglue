
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

#ifndef __GRAMMAR_MANAGER_HPP__
#define __GRAMMAR_MANAGER_HPP__

#include "VXIrec.h"
#include "VXIvalue.h"
#include "Scripter.hpp" 
#include <vector>

extern "C" struct VXIrecGrammar;
extern "C" struct VXIrecInterface;
extern "C" struct VXIrecRecordResult;
class GrammarInfo;
class GrammarInfoUniv;
class PropertyList;
class SimpleLogger;
class VXMLElement;


class GrammarManager {
public:
  enum OpType {
    GRAMMAR,            
    RECOGNIZE,
    RECORD,
    TRANSFER
  };

  static void ThrowSpecificEventError(VXIrecResult err, OpType opType);
  
  static const VXIchar * const DTMFTerm;
  static const VXIchar * const FinalSilence;
  static const VXIchar * const MaxTime;
  static const VXIchar * const RecordingType;

  // This function walks through the document, creating grammars as necessary.
  // The id of the document (passed to Enable) is returned.
  //
  // may throw: VXIException::InterpreterError & VXIException::OutOfMemory
  void LoadGrammars(const VXMLElement& doc, vxistring & id, PropertyList &,
    bool isDefaults = false);

  // Deactivates all current grammars.
  void DisableAllGrammars();

  // Activates (for recognition) grammars matching the given dialog & field
  // name.  The documentID is returned by LoadGrammars.
  //
  // Returns: false - no grammars were enabled
  //          true  - at least one grammar is ready
  //
  // may throw VXIException::InterpreterEvent
  bool EnableGrammars(const vxistring & documentID,
                      const vxistring & dialogName,
                      const vxistring & fieldName,
                      const VXIMapHolder & properties,
                      bool isModal,
					  Scripter &script,
					  PropertyList & propList);

  /** Recognition result */
  enum RecResult {
    Success,       /// Recognition returned a hypothesis
    OutOfMemory,   //  should be gone ??? // Insufficient memory available
    BadMimeType,   //  should be gone ??? // The requested type is not supported
    InternalError  /// VXIrecInterface::Recognize failed gave invalid results
  };

  /**
   * Performs ASR recognition.  The result argument will contain the
   * recognition result, and must be destroyed by the caller by invoking
   * the Destroy function.
   */
  RecResult Recognize(const VXIMapHolder & properties,
                VXIrecRecognitionResult * & result);

  bool FindMatchedElement(const vxistring & id, VXMLElement & match) const;

  bool FindMatchedElement(const vxistring & id, VXMLElement & match,
                          unsigned long & ginfo) const;

  // Compare the precedence of info1 against info2
  // Returns: 1 if info1 has higher precedence than info2
  //          0 if both has the same precedence
  //         -1 if info2 has higher precedence than info1
  int CompareGrammar(unsigned long info1, unsigned long info2);

  RecResult Record(const VXIMapHolder & properties,
             bool flushBeforeRecord,
             VXIrecRecordResult * & answer);

  // Deletes all current grammars.
  void ReleaseGrammars();

  VXIMap * GetRecProperties(const PropertyList &, int timeout = -1) const;
  VXIMap * GetRecordProperties(const PropertyList &, int timeout = -1) const;

  // may throw: VXIException::OutOfMemory()
  GrammarManager(VXIrecInterface * r, const SimpleLogger & l);

  ~GrammarManager();

private:
  void SetGrammarLoadProperties(const VXMLElement & element, const PropertyList&,
                                VXIMapHolder & props) const;

  // Invoked by LoadGrammars and called recursively to build grammars.
  //
  // may throw: VXIException::InterpreterError & VXIException::OutOfMemory
  void BuildGrammars(const VXMLElement& doc, const vxistring & id,
                     PropertyList &, VXIMapHolder & flattenedProperties,
                     int menuAcceptLevel = 0);

  // Invoked by LoadGrammars to commit each grammar to the collection.
  //
  // may throw: VXIException::OutOfMemory()
  void AddGrammar(VXIrecGrammar * gr, const vxistring & docID,
                  const VXMLElement & elem, const VXIchar *expr = NULL, const VXIchar *type = NULL);

  void BuildUniversals(const VXMLElement& doc, PropertyList &);

  void AddUniversal(VXIrecGrammar * gr, const VXMLElement & elem,
                    const vxistring & langID, const vxistring & name);

  VXIrecGrammar * BuildInlineGrammar(const VXMLElement & element,
                                     const VXIMapHolder & localProps);

  void BuildOptionGrammars(const vxistring & docID, const VXMLElement& doc,
                           const VXIMapHolder & props);
  
  static bool GetEnclosedText(const SimpleLogger & log,
                              const VXMLElement & doc, vxistring & str);
  static VXIrecGrammar * CreateGrammarFromString(VXIrecInterface * vxirec,
                                                 const SimpleLogger & log,
                                                 const vxistring & source,
                                                 const VXIchar * type,
                                                 const VXIMapHolder & props);
  static VXIrecGrammar * CreateGrammarFromURI(VXIrecInterface * vxirec,
                                              const SimpleLogger & log,
                                              const vxistring & source,
                                              const VXIchar * type,
                                              const VXIMap * fetchProps,
                                              const VXIMapHolder & props);

  unsigned long GetGrammarSequence(void) { return ++grammarSequence; }
  
  void SetInputMode(const VXIchar *mode, VXIMapHolder &map) const;
                                    
private:
  typedef std::vector<GrammarInfo *> GRAMMARS;
  GRAMMARS grammars;

  typedef std::vector<GrammarInfoUniv *> UNIVERSALS;
  UNIVERSALS universals;

  const SimpleLogger & log;
  VXIrecInterface * vxirec;
  unsigned int grammarSequence;
};

#endif
