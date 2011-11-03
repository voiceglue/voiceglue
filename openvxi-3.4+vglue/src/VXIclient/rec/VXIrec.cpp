
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#define VXIREC_EXPORTS
#include "VXIrecAPI.h"
#include "VXIrec_utils.h"
#include "LogBlock.hpp"

#include "SWIutfconversions.h"
#include <VXItrd.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>
#include <vglue_rec.h>

#include <cstdio>
#include <cstring>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

// ------*---------*---------*---------*---------*---------*---------*---------

static void ResultContentDestroy(VXIbyte **content, void *userData)
{
  if (content && *content)
    delete [] *content;
  *content = NULL;
}

class VXIVectorHolder {
public:
  VXIVectorHolder() : _vector(NULL)  { _vector = VXIVectorCreate(); }
  VXIVectorHolder(VXIVector * m) : _vector(m) { }
  ~VXIVectorHolder()        { if (_vector != NULL) VXIVectorDestroy(&_vector);}

  VXIVectorHolder & operator=(const VXIVectorHolder & x)
  { if (this != &x) {
      if (_vector != NULL) VXIVectorDestroy(&_vector);
      _vector = VXIVectorClone(x._vector); }
    return *this; }

  // GetValue returns the internal vector.  The standard vector manipulation
  // functions may then be used.

  VXIVector * GetValue() const       { return _vector; }

  // These functions allow the holder to take ownership of an existing vector
  // or to release the internal one.

  VXIVector * Release()  
  { VXIVector * m = _vector; _vector = NULL; return m; }

  void Acquire(VXIVector * m)
  { if (_vector != NULL) VXIVectorDestroy(&_vector); else _vector = m; }

private:
  VXIVectorHolder(const VXIVectorHolder &);   // intentionally not defined.

  VXIVector * _vector;
};

// ------*---------*---------*---------*---------*---------*---------*---------

// Global for the base diagnostic tag ID, see osbrec_utils.h for tags
//
static VXIunsigned gblDiagLogBase = 0;


// VXIrec implementation of the VXIrec interface
//
extern "C" {
  struct VXIrecImpl {
    // Base interface, must be first
    VXIrecInterface intf;

    // Class for managing grammars
    VXIrecData *recData;
  };
}


// A few conversion functions...

static inline VXIrecGrammar * ToVXIrecGrammar(VXIrecGrammar * i)
{ return reinterpret_cast<VXIrecGrammar *>(i); }

static inline VXIrecGrammar * FromVXIrecGrammar(VXIrecGrammar * i)
{ return reinterpret_cast<VXIrecGrammar *>(i); }

static inline VXIrecData * GetRecData(VXIrecInterface * i)
{
  if (i == NULL) return NULL;
  return reinterpret_cast<VXIrecImpl *>(i)->recData;
}

/*******************************************************
 * Method routines for VXIrecInterface structure
 *******************************************************/ 

// Get the VXIrec interface version supported
//
static VXIint32 VXIrecGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}


// Get the implementation name
//
static const VXIchar* VXIrecGetImplementationName(void)
{
  static const VXIchar IMPLEMENTATION_NAME[] = COMPANY_DOMAIN L".VXIrec";
  return IMPLEMENTATION_NAME;
}

static VXIrecResult VXIrecBeginSession(VXIrecInterface * pThis, VXIMap *)
{
  const wchar_t* fnname = L"VXIrecBeginSession";
  VXIrecData* tp = GetRecData(pThis);
  
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);
  
  return VXIrec_RESULT_SUCCESS;
}


static VXIrecResult VXIrecEndSession(VXIrecInterface *pThis, VXIMap *)
{
  const wchar_t* fnname = L"VXIrecEndSession";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  tp->Clear();
  return VXIrec_RESULT_SUCCESS;
}


static VXIrecResult VXIrecLoadGrammarFromString(VXIrecInterface *pThis,
                                                const VXIMap *prop,
                                                const VXIchar *type,
                                                const VXIchar *str,
                                                VXIrecGrammar **gram)
{
  const wchar_t* fnname = L"VXIrecLoadGrammarFromString";
  const VXIchar *gram_type;
  VXIrecData* tp = GetRecData(pThis);

  //  DEBUG
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIrecLoadGrammarFromString called with type "
		<< VXIchar_to_Std_String (type)
		<< ", grammar "
		<< VXIchar_to_Std_String (str)
		<< ", properties "
		<< VXIValue_to_Std_String ((const VXIValue *) prop);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Check the arguments
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;

  if (str == NULL) {
    return VXIrec_RESULT_INVALID_ARGUMENT;
  }
  if (type == NULL) {
      // return VXIrec_RESULT_UNSUPPORTED;
      gram_type = L"";
  }
  else
  {
      gram_type = type;
  };

  //  Create a dummy grammar object
  //  (grammar is really parsed externally)
  vxistring srgsGram (str);
  VXIrecGrammar * gramPtr = tp->ParseSRGSGrammar(srgsGram, prop);
  tp->AddGrammar(gramPtr);
  *gram = ToVXIrecGrammar(gramPtr);

  //  Get the gram_id
  char gram_id[24];
  sprintf (gram_id, "%p", gramPtr);

  //  Do the external parse
  return voiceglue_load_grammar (str, gram_type, prop, gram_id);
}

static VXIrecResult VXIrecLoadGrammarFromURI(struct VXIrecInterface *pThis,
                                             const VXIMap *properties,
                                             const VXIchar *type, 
                                             const VXIchar *uri,
                                             const VXIMap *uriArgs,
                                             VXIrecGrammar **gram)
{
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  const wchar_t* fnname = L"VXIrecLoadGrammarFromURI";
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);
  tp->ShowPropertyValue(properties);

  //  DEBUG
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIrecLoadGrammarFromURI called with type "
		<< VXIchar_to_Std_String (type)
		<< ", uri "
		<< VXIchar_to_Std_String (uri)
		<< ", properties "
		<< VXIValue_to_Std_String ((const VXIValue *) properties)
		<< ", uriArgs "
		<< VXIValue_to_Std_String ((const VXIValue *) uriArgs);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (wcsncmp(uri, L"builtin:", 8) == 0 )
  {
      //  DEBUG
      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "VXIrecLoadGrammarFromURI detected builting grammar "
		    << VXIchar_to_Std_String (uri);
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };

      //  Let voiceglue translate it
      return VXIrecLoadGrammarFromString (pThis, properties, type, uri, gram);
  }

  VXIbyte *buffer = NULL;
  VXIulong read = 0;
  if (!tp->FetchContent(uri, uriArgs, &buffer, read))
    return VXIrec_RESULT_FAILURE;

  // NOTE:  this code assumes the retieved buffer is
  // composed of single byte chars.  in the real world,
  // that can't be assumed.
  buffer[read] = 0;
  VXIchar *charbuff = new VXIchar[read+1];
  mbstowcs(charbuff, (char*)buffer, read+1);

  VXIrecResult rc = VXIrecLoadGrammarFromString(pThis, properties,
    type, charbuff, gram);

  delete[] buffer;
  delete[] charbuff;

  return rc;
}


static VXIrecResult VXIrecLoadGrammarOption(VXIrecInterface * pThis,
                                      const VXIMap    * properties,
                                      const VXIVector * gramChoice,
                                      const VXIVector * gramValue,
                                      const VXIVector * gramAcceptance,
                                      const VXIbool     isDTMF,
                                      VXIrecGrammar  ** gram)
{
  const wchar_t* fnname = L"VXIrecLoadGrammarOption";
  std::ostringstream srgs_version;
  vxistring srgs_wide_version;
  VXIunsigned num_choices;
  VXIvalueType value_type;
  const VXIValue *utterance_value;
  // Check the arguments
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);
    
  if (gram == NULL) {
    logger = VXIrec_RESULT_INVALID_ARGUMENT;
    return VXIrec_RESULT_INVALID_ARGUMENT;
  }

  //  DEBUG
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIrecLoadGrammarOption called with type "
		<< (isDTMF ? "DTMF" : "speech")
		<< ", choices "
		<< VXIValue_to_Std_String ((VXIValue *) gramChoice)
		<< ", values "
		<< VXIValue_to_Std_String ((VXIValue *) gramValue)
		<< ", properties "
		<< VXIValue_to_Std_String ((const VXIValue *) properties);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  tp->ShowPropertyValue(properties);

  //  Convert to SRGS grammar
  num_choices = VXIVectorLength (gramChoice);
  srgs_version << "<rule id=\"choice\">\n  <one-of>\n";
  for (VXIunsigned choice_num = 0; choice_num < num_choices; ++choice_num)
  {
      srgs_version << "    <item> ";
      utterance_value = VXIVectorGetElement (gramChoice, choice_num);
      value_type = VXIValueGetType (utterance_value);
      if (value_type == VALUE_INTEGER)
      {
	  srgs_version << VXIIntegerValue ((const VXIInteger*) utterance_value);
      }
      else if (value_type == VALUE_LONG)
      {
	  srgs_version << VXILongValue ((const VXILong*) utterance_value);
      }
      else if (value_type == VALUE_ULONG)
      {
	  srgs_version << VXIULongValue ((const VXIULong*) utterance_value);
      }
      else if (isDTMF)
      {
	  srgs_version << VXIchar_to_Std_String (
	      VXIStringCStr ((VXIString *) utterance_value));
      }
      else
      {
	  srgs_version << (choice_num + 1);
      };
      srgs_version << " <tag>"
		   << VXIchar_to_Std_String (
		       VXIStringCStr (
			   (VXIString *) VXIVectorGetElement
			   (gramValue, choice_num)))
		   << "</tag> </item>\n";
  };
  srgs_version << "  </one-of>\n</rule>\n";
  srgs_wide_version = Std_String_to_vxistring (srgs_version.str());
  return VXIrecLoadGrammarFromString
      (pThis,
       properties,
       L"text/x-grammar-choice-dtmf",
       (const VXIchar *) srgs_wide_version.c_str(),
       gram);
  
  VXIrecGrammar * gp = NULL;
  
  vxistring srgsGram;
  if( !tp->OptionToSRGS(properties, gramChoice, gramValue, 
                       gramAcceptance, isDTMF, srgsGram) )
    return VXIrec_RESULT_FAILURE;

  // Parsing SRGS grammar.
  // NOTES: The parsing is very simple, therefore it may not work
  // for complex grammar.  As you know, this is a simulator!!!
  
  VXIrecGrammar * gramPtr = tp->ParseSRGSGrammar(srgsGram, properties, isDTMF);
  if( gramPtr == NULL ) return VXIrec_RESULT_FAILURE;
  tp->AddGrammar(gramPtr);
  *gram = ToVXIrecGrammar(gramPtr);

  return VXIrec_RESULT_SUCCESS;
}

static VXIrecResult VXIrecFreeGrammar(VXIrecInterface *pThis,
                                      VXIrecGrammar **gram)
{
  const wchar_t* fnname = L"VXIrecFreeGrammar";
  // Check the arguments
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  // If the grammar pointer is null, there is nothing to free.
  if (gram == NULL || *gram == NULL) VXIrec_RESULT_SUCCESS;

  //  Get the gram_id
  char gram_id[24];
  sprintf (gram_id, "%p", (void *) *gram);
  voiceglue_free_grammar(gram_id);

  tp->FreeGrammar(FromVXIrecGrammar(*gram));
  *gram = NULL;
  return VXIrec_RESULT_SUCCESS;
}


static VXIrecResult VXIrecActivateGrammar(VXIrecInterface *pThis,
                                          const VXIMap *properties,
                                          VXIrecGrammar *gram)
{
    const wchar_t* fnname = L"VXIrecActivateGrammar";
    VXIrecData* tp = GetRecData(pThis);
    if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
    LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

    if (gram == NULL) {
	logger = VXIrec_RESULT_INVALID_ARGUMENT;
	return VXIrec_RESULT_INVALID_ARGUMENT;
    }

    tp->ActivateGrammar(FromVXIrecGrammar(gram));

    //  Get the gram_id
    char gram_id[24];
    sprintf (gram_id, "%p", (void *) gram);

    //  Do external activation
    return (voiceglue_activate_grammar (properties, gram_id));
}


static VXIrecResult VXIrecDeactivateGrammar(VXIrecInterface *pThis,
                                            VXIrecGrammar *gram)
{
  const wchar_t* fnname = L"VXIrecDeactivateGrammar";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  if (gram == NULL) 
  {
    logger = VXIrec_RESULT_INVALID_ARGUMENT;
    return VXIrec_RESULT_INVALID_ARGUMENT;
  }


  tp->DeactivateGrammar(FromVXIrecGrammar(gram));

    //  Get the gram_id
    char gram_id[24];
    sprintf (gram_id, "%p", (void *) gram);

    //  Do external deactivation
    return (voiceglue_deactivate_grammar (gram_id));
}

/*******************************************************
 * Recognize related
 *******************************************************/ 

static void RecognitionResultDestroy(VXIrecRecognitionResult **Result)
{
  if (Result == NULL || *Result == NULL) return;

  VXIrecRecognitionResult * result = *Result;

  if (result->utterance != NULL)
    VXIContentDestroy(&result->utterance);

  if (result->xmlresult != NULL)
    VXIContentDestroy(&result->xmlresult);

  if (result->markname != NULL)
    VXIStringDestroy(&result->markname);

  delete result;
  *Result = NULL;
}

static void TransferResultDestroy(VXIrecTransferResult **Result)
{
  if (Result == NULL || *Result == NULL) return;

  VXIrecTransferResult * result = *Result;

  if (result->utterance != NULL)
    VXIContentDestroy(&result->utterance);

  if (result->xmlresult != NULL)
    VXIContentDestroy(&result->xmlresult);

  if (result->markname != NULL)
    VXIStringDestroy(&result->markname);

  delete result;
  *Result = NULL;
}

static const unsigned int NLSML_NOINPUT_SIZE = 109;
static const VXIchar NLSML_NOINPUT[NLSML_NOINPUT_SIZE] =
  L"<?xml version='1.0'?>"
  L"<result>"
    L"<interpretation>"
      L"<instance/>"
      L"<input><noinput/></input>"
    L"</interpretation>"
  L"</result>\0";

static const unsigned int NLSML_NOMATCH_SIZE = 109;
static const VXIchar NLSML_NOMATCH[NLSML_NOMATCH_SIZE] =
  L"<?xml version='1.0'?>"
  L"<result>"
    L"<interpretation>"
      L"<instance/>"
      L"<input><nomatch/></input>"
    L"</interpretation>"
  L"</result>\0";

void DestroyNLSMLBuffer(VXIbyte ** buffer, void * ok)
{
  // Return immediate if we know the buffer is static memory, 
  // i.e: noinput, nomatch as defined above
  if (ok == NULL || buffer == NULL) return;
  // otherwise reclaim the memory
  delete[] (*buffer);
  *buffer = NULL;
}


static VXIrecResult VXIrecRecognize(VXIrecInterface *pThis,
                                    const VXIMap *properties,
                                    VXIrecRecognitionResult **recogResult)
{
  const wchar_t* fnname = L"VXIrecRecognize";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  VXIchar* input = NULL;
  VXIMap*  res  = NULL;
  VXIchar* str  = NULL;
  VXIchar console[512];
  bool recordUtterance = false;
  bool haveUtterance = true;

  //  DEBUG
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIrecRecognize called with properties "
		<< VXIValue_to_Std_String ((const VXIValue *) properties);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (properties != NULL) {
    const VXIValue * ru = VXIMapGetProperty(properties, REC_RECORDUTTERANCE);
    if (ru)
      recordUtterance = wcscmp(VXIStringCStr((const VXIString*)ru), L"true") == 0;
    /*  Ignore force-feed input option
    const VXIValue * val = VXIMapGetProperty(properties, L"SpeechInput");
    if( val ) {
       logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%s", L"SpeechInput: ",
                      VXIStringCStr((const VXIString*)val));
    }
    if (val == NULL || VXIValueGetType(val) != VALUE_STRING) {
      val = VXIMapGetProperty(properties, L"DTMFInput");
      if( val ) {
        logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%s", L"DTMFInput: ",
                       VXIStringCStr((const VXIString*)val));      
      }
    }
    VXIString* vect = (VXIString*) val;
    if (vect != NULL) input = (VXIchar*) VXIStringCStr(vect);
    */

    /*  Ignore console input option
    if (input && wcscmp(input, L"-") == 0) {
      unsigned int i;
      VXIchar* cp = console;
      char lbuf[512];
      printf("Console: ");
      fgets(lbuf, 511, stdin);
      // copy to VXIchar
      for(i = 0; i < strlen(lbuf); ++i) {
        if (lbuf[i] == '\r' || lbuf[i] == '\n') continue;
        *cp++ = lbuf[i] & 0x7f;
      }
      *cp++ = 0;
      input = console;
      logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%s", L"Input: ", 
                     (input ? input : L"NULL"));
    }
    */
  }

  //  Get NLSML recognition result from perl in nlsmlresult
  vxistring nlsmlresult;
  VXIrecResult rec_res;
  rec_res = voiceglue_recognize (properties, nlsmlresult);
  if (rec_res != VXIrec_RESULT_SUCCESS)
  {
      return (rec_res);
  };

  // Create a new results structure.
  const unsigned int CHARSIZE = sizeof(VXIchar) / sizeof(VXIbyte);
  VXIContent * xmlresult = NULL;

    logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%s", L"NLSML_RESULT: ", 
                   nlsmlresult.c_str());
    
    unsigned int BUFFERSIZE = (nlsmlresult.length() + 1) * CHARSIZE;
    VXIbyte * buffer = new VXIbyte[BUFFERSIZE];
    if (buffer == NULL) return VXIrec_RESULT_OUT_OF_MEMORY;
    memcpy(buffer, nlsmlresult.c_str(), BUFFERSIZE);

    xmlresult = VXIContentCreate(VXIREC_MIMETYPE_XMLRESULT,
                                 buffer, BUFFERSIZE,
                                 DestroyNLSMLBuffer, buffer);

    if (xmlresult == NULL) {
      delete [] buffer;
      return VXIrec_RESULT_OUT_OF_MEMORY;
    }

  VXIrecRecognitionResult * result = new VXIrecRecognitionResult();
  if (result == NULL) {
    VXIContentDestroy(&xmlresult);
    return VXIrec_RESULT_OUT_OF_MEMORY;
  }

  result->Destroy = RecognitionResultDestroy;
  result->markname = NULL;
  result->marktime = 0;
  result->xmlresult = xmlresult;

  if (!haveUtterance ||!recordUtterance) {
    result->utterance = NULL;
    result->utteranceDuration = 0;
  }
  else {
    result->utteranceDuration = 5000; // 5sec
    unsigned int waveformSizeBytes = (result->utteranceDuration / 1000 ) * 8000 * sizeof(unsigned char);
    unsigned char * c_waveform = new unsigned char[waveformSizeBytes];
    if (c_waveform == NULL) {
      result->utterance = NULL;
      result->utteranceDuration = 0;
    }
    else {
      for (unsigned int i = 0; i < waveformSizeBytes; ++i)
        c_waveform[i] = i & 0x00ff;

      result->utterance = VXIContentCreate(VXIREC_MIMETYPE_ULAW,
                                           c_waveform, waveformSizeBytes,
                                           ResultContentDestroy, NULL);  
    }
  }

  *recogResult = result;

  return VXIrec_RESULT_SUCCESS;
}


static VXIrecResult VXIrecGetMatchedGrammar(VXIrecInterface *pThis,
                                            const VXIchar *grammarID,
                                            const VXIrecGrammar **gram)
{
  const wchar_t* fnname = L"VXIrecGetMatchedGrammar";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  if (grammarID == NULL || gram == NULL)
    return VXIrec_RESULT_INVALID_ARGUMENT;

  VXIrecGrammar * mgram = NULL;
  SWIswscanf(grammarID, L"%p", &mgram);

  *gram = mgram;
  logger.logDiag(DIAG_TAG_RECOGNITION, L"%s%p", L"Got Matched grammar: ", mgram); 
  return VXIrec_RESULT_SUCCESS;
}

static void RecordResultDestroy(VXIrecRecordResult **Result)
{
  if (Result == NULL || *Result == NULL) return;

  VXIrecRecordResult * result = *Result;

  if (result->waveform != NULL)
    VXIContentDestroy(&result->waveform);

  if (result->xmlresult != NULL)
    VXIContentDestroy(&result->xmlresult);

  delete result;
  *Result = NULL;
}

static VXIrecResult VXIrecRecord(VXIrecInterface *pThis,
                                 const VXIMap *props,
                                 VXIrecRecordResult **recordResult)
{
  const wchar_t* fnname = L"VXIrecRecord";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);

  if (recordResult == NULL) {
    logger = VXIrec_RESULT_INVALID_ARGUMENT;
    return VXIrec_RESULT_INVALID_ARGUMENT;
  }

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXIrecRecord called with properties "
		<< VXIValue_to_Std_String ((const VXIValue *) props);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // (1) Create record structure
  VXIrecRecordResult *result = new VXIrecRecordResult();
  if (result == NULL) {
    logger = VXIrec_RESULT_OUT_OF_MEMORY;
    return VXIrec_RESULT_OUT_OF_MEMORY;
  }
  result->Destroy = RecordResultDestroy;
  result->waveform = NULL;
  result->xmlresult = NULL;
  result->duration = 0;
  result->termchar = 0;
  result->maxtime = FALSE;
  result->markname = NULL;
  result->marktime = 0;
  *recordResult = result;

  unsigned char * c_waveform;
  unsigned int waveformSizeBytes;
  int rec_maxtime;
  unsigned int vg_duration;
  VXIbyte digit;

  //  voiceglue code puts waveform into c_waveform[0..<end>]
  VXIrecResult r =  voiceglue_record (props,
				      &c_waveform,
				      &waveformSizeBytes,
				      &vg_duration,
				      &rec_maxtime,
				      &digit);
  if (r != VXIrec_RESULT_SUCCESS)
  {
      (*recordResult)->Destroy(recordResult);    
      return r;
  };
  if (c_waveform == NULL)
  {
      //  Got NOINPUT
      (*recordResult)->xmlresult =
	  VXIContentCreate(VXIREC_MIMETYPE_XMLRESULT,
			   (VXIbyte *) NLSML_NOINPUT, NLSML_NOINPUT_SIZE*4,
			   DestroyNLSMLBuffer, NULL);
  }
  else
  {
      //  Got a waveform
      (*recordResult)->waveform =
	  VXIContentCreate(VXIREC_MIMETYPE_ULAW,
			   c_waveform, waveformSizeBytes,
			   ResultContentDestroy, NULL);
      if( (*recordResult)->waveform == NULL ) {
	  (*recordResult)->Destroy(recordResult);    
	  tp->LogError(400, L"%s%s", L"Error", L"out of memory");
	  return VXIrec_RESULT_OUT_OF_MEMORY;    
      }
      
      (*recordResult)->duration = (VXIlong) vg_duration;
      if(rec_maxtime == 1) (*recordResult)->maxtime = TRUE;
      (*recordResult)->termchar = digit;
  };

  return VXIrec_RESULT_SUCCESS;
}

static VXIbool VXIrecSupportsHotwordTransfer(
  struct VXIrecInterface * pThis,
  const VXIMap  * properties,
  const VXIchar * transferDest)
{
  VXIbool result = FALSE;

  // this is really just to demonstrate how the platform could
  // support hotword for some type of transfers, but not others.
  // this could just as easily been based on other properties of
  // the transfer. i.e., destination type, required line type, etc...
  const VXIValue* dval = VXIMapGetProperty(properties, TEL_TRANSFER_TYPE);
  if( dval && VXIValueGetType(dval) == VALUE_STRING ){
    const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
	if (wcscmp(hid, L"consultation") == 0)
      return FALSE;
  }

  dval = VXIMapGetProperty(properties, L"SupportsHotwordTransfer");
  if( dval && VXIValueGetType(dval) == VALUE_STRING ){
    const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
	result = (wcscmp(hid, L"true") == 0) ? TRUE : FALSE;
  }

  return result;
}

static VXIrecResult VXIrecHotwordTransfer
(
  struct VXIrecInterface * pThis,
  struct VXItelInterface * tel,
  const VXIMap *properties,
  const VXIchar* transferDest,
  VXIrecTransferResult  ** transferResult
)
{
  const wchar_t* fnname = L"VXIrecHotwordTransfer";
  VXIrecData* tp = GetRecData(pThis);
  if (tp == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);
  
  VXIrecResult res = VXIrec_RESULT_SUCCESS;

  // this is probably not how it would be done in real life...but for this
  // sample app it will suffice:
  // Do the transfer
  int activeGrammars = tp->GetActiveCount();

  VXIMap *xferResp = NULL;
  VXItelResult xferResult = tel->TransferBridge(tel, properties, transferDest, &xferResp);
  if (xferResult != VXItel_RESULT_SUCCESS) {
    res = VXIrec_RESULT_FAILURE;
  }
  else if (activeGrammars == 0) {
    *transferResult = new VXIrecTransferResult();
    (*transferResult)->Destroy = TransferResultDestroy;
    (*transferResult)->utterance = NULL;
    (*transferResult)->markname = NULL;
    (*transferResult)->marktime = 0;
    (*transferResult)->xmlresult = NULL;

    const VXIValue * v = VXIMapGetProperty(xferResp, TEL_TRANSFER_DURATION);
    if (v != NULL && VXIValueGetType(v) == VALUE_INTEGER)
      (*transferResult)->duration = VXIIntegerValue(reinterpret_cast<const VXIInteger*>(v));

    v = VXIMapGetProperty(xferResp, TEL_TRANSFER_STATUS);
    if (v != NULL && VXIValueGetType(v) == VALUE_INTEGER) {
      long temp = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v));
      (*transferResult)->status = VXItelTransferStatus(temp);
    }

	VXIMapDestroy(&xferResp);
  }
  else {
    // get input from the user.
    VXIrecRecognitionResult *recResult = NULL;
    VXIrecResult localRes = pThis->Recognize(pThis, properties, &recResult);
	if (localRes == VXIrec_RESULT_SUCCESS){
      *transferResult = new VXIrecTransferResult();
	  if ((*transferResult) == NULL) {
        recResult->Destroy(&recResult);
        VXIMapDestroy(&xferResp);
        return VXIrec_RESULT_OUT_OF_MEMORY;
	  }

      // setup the transfer result by first copying the 
	  // RecResult.
      (*transferResult)->Destroy = TransferResultDestroy;
      (*transferResult)->utterance = recResult->utterance;
      (*transferResult)->markname = recResult->markname;
	  (*transferResult)->marktime = recResult->marktime;
      (*transferResult)->xmlresult = recResult->xmlresult;
	  (*transferResult)->duration = 0;
	  (*transferResult)->status = VXItel_TRANSFER_FAR_END_DISCONNECT;

      // we no long need the rec result (but we don't want the VXIContent
	  // fields destroyed).
	  recResult->utterance = NULL;
	  recResult->xmlresult = NULL;
	  recResult->Destroy(&recResult);

      // translate the status and duration to the transfer result fields.
	  // since this code is simply so that Hotword transfers work, any
	  // valid recognition (ignoring noinput and nomatch) will set the
	  // transfer to "near_end_disconnect".  Otherwise, we pretend that
	  // the other end terminated the transfer.
	  if((*transferResult)->xmlresult){
	    // we're going to ignore noinput and nomatch results.
        const VXIchar * contentType;
        const VXIbyte * content;
        VXIulong contentSize = 0; 
        VXIContentValue((*transferResult)->xmlresult, &contentType, &content, &contentSize);

	    // we're going to ignore noinput and nomatch results.
        if (   !wcsstr((VXIchar*)content, L"<noinput/>")
			&& !wcsstr((VXIchar*)content, L"<nomatch/>")){
          (*transferResult)->status = VXItel_TRANSFER_NEAR_END_DISCONNECT;
        }
      }

	  const VXIValue * v = VXIMapGetProperty(xferResp, TEL_TRANSFER_DURATION);
	  if (v != NULL && VXIValueGetType(v) == VALUE_INTEGER)
	    (*transferResult)->duration = VXIIntegerValue(reinterpret_cast<const VXIInteger*>(v));

	  VXIMapDestroy(&xferResp);
	}
  }

  return res; 
}

/*******************************************************
 * Init and factory routines
 *******************************************************/ 

static inline VXIrecImpl * ToVXIrecImpl(VXIrecInterface * i)
{ return reinterpret_cast<VXIrecImpl *>(i); }

// Global init - Don't need to do much here
//
VXIREC_API VXIrecResult VXIrecInit(VXIlogInterface *log,
				   VXIunsigned      diagLogBase,
				   VXIMap           *args)
{
  if (!log) return VXIrec_RESULT_INVALID_ARGUMENT;
  gblDiagLogBase = diagLogBase;
  const wchar_t* fnname = L"VXIrecInit";
  LogBlock logger(log, gblDiagLogBase, fnname, VXIREC_MODULE);

  logger = VXIrecData::Initialize(log, diagLogBase);
  return VXIrec_RESULT_SUCCESS;
}

// Global shutdown
//
VXIREC_API VXIrecResult VXIrecShutDown(VXIlogInterface *log)
{
  if (!log) return VXIrec_RESULT_INVALID_ARGUMENT;
  const wchar_t* fnname = L"VXIrecShutDown";
  LogBlock logger(log, gblDiagLogBase, fnname, VXIREC_MODULE);
  logger = VXIrecData::ShutDown();
  return VXIrec_RESULT_SUCCESS;
}

// Create an VXIrecInterface object and return.
//
VXIREC_API VXIrecResult VXIrecCreateResource(VXIlogInterface   *log,
					     VXIinetInterface  *inet,
  					   VXIcacheInterface *cache,
	  				   VXIpromptInterface *prompt,
		  			   VXItelInterface *tel,
					     VXIrecInterface  **rec)
{
  if (!log) return VXIrec_RESULT_INVALID_ARGUMENT;
  const wchar_t* fnname = L"VXIrecCreateResource";
  LogBlock logger(log, gblDiagLogBase, fnname, VXIREC_MODULE);

  VXIrecImpl* pp = new VXIrecImpl();
  if (pp == NULL) {
    logger = VXIrec_RESULT_OUT_OF_MEMORY;
    return VXIrec_RESULT_OUT_OF_MEMORY;
  }

  VXIrecData* tp = new VXIrecData(log, inet);
  if (tp == NULL) {
    logger = VXIrec_RESULT_OUT_OF_MEMORY;
    return VXIrec_RESULT_OUT_OF_MEMORY;
  }
  
  pp->recData = tp;
  pp->intf.GetVersion = VXIrecGetVersion;
  pp->intf.GetImplementationName = VXIrecGetImplementationName;
  pp->intf.BeginSession = VXIrecBeginSession;
  pp->intf.EndSession = VXIrecEndSession;
  pp->intf.LoadGrammarURI = VXIrecLoadGrammarFromURI;  
  pp->intf.LoadGrammarString = VXIrecLoadGrammarFromString;
  pp->intf.LoadGrammarOption = VXIrecLoadGrammarOption;
  pp->intf.ActivateGrammar = VXIrecActivateGrammar;
  pp->intf.DeactivateGrammar = VXIrecDeactivateGrammar;
  pp->intf.FreeGrammar = VXIrecFreeGrammar;
  pp->intf.Recognize = VXIrecRecognize;
  pp->intf.Record = VXIrecRecord;
  pp->intf.HotwordTransfer = VXIrecHotwordTransfer;
  pp->intf.SupportsHotwordTransfer = VXIrecSupportsHotwordTransfer;
  pp->intf.GetMatchedGrammar = VXIrecGetMatchedGrammar;

  *rec = &pp->intf;
  return VXIrec_RESULT_SUCCESS;
}


// Free VXIrec structure allocated in VXIrecCreateResource.
//
VXIREC_API VXIrecResult VXIrecDestroyResource(VXIrecInterface **rec)
{
  if (rec == NULL || *rec == NULL) return VXIrec_RESULT_INVALID_ARGUMENT;
  VXIrecImpl* recImpl = reinterpret_cast<VXIrecImpl*>(*rec);

  VXIrecData * tp = recImpl->recData;
  if ( tp ) {
    const wchar_t* fnname = L"VXIrecDestroyResource";
    LogBlock logger(tp->GetLog(), gblDiagLogBase, fnname, VXIREC_MODULE);
    delete tp;
  }
  
  delete recImpl;
  *rec = NULL;

  return VXIrec_RESULT_SUCCESS;
}
