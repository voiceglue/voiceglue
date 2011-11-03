#ifndef _VXML_H
#define _VXML_H

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

#include "VXItypes.h"

enum VXMLElementType {
  NODE_ASSIGN,
  NODE_AUDIO,
  NODE_BLOCK,
  NODE_CANCEL,
  NODE_CATCH,
  NODE_CHOICE,        // 5
  NODE_CLEAR,
  NODE_DATA,
  NODE_DISCONNECT,
  NODE_ELSE,
  NODE_ELSEIF,        // 10
  NODE_ENUMERATE,
  NODE_EXIT,
  NODE_FIELD,
  NODE_FILLED,
  NODE_FOREACH,       // 15
  NODE_FORM,
  NODE_GOTO,
  NODE_GRAMMAR,
  NODE_IF,
  NODE_INITIAL,       // 20
  NODE_LINK,
  NODE_LOG,
  NODE_MENU,
  NODE_MARK,
  NODE_META,          // 25
  NODE_OBJECT,
  NODE_OPTION,
  NODE_PARAM,
  NODE_PROMPT,
  NODE_PROPERTY,      // 30
  NODE_RECORD,
  NODE_RETURN,
  NODE_REPROMPT,
  NODE_SCRIPT,
  NODE_SUBDIALOG,     // 35
  NODE_SUBMIT,
  NODE_THROW,
  NODE_TRANSFER,
  NODE_VALUE,
  NODE_VAR,           // 40
  NODE_VXML,

  DEFAULTS_ROOT,
  DEFAULTS_LANGUAGE
};


enum VXMLAttributeType {
  ATTRIBUTE__ITEMNAME,
  ATTRIBUTE_AAI,
  ATTRIBUTE_AAIEXPR,
  ATTRIBUTE_ACCEPT,
  ATTRIBUTE_APPLICATION,
  ATTRIBUTE_ARCHIVE,
  ATTRIBUTE_ARRAY,
  ATTRIBUTE_AUDIOBASE,
  ATTRIBUTE_BARGEIN,
  ATTRIBUTE_BARGEINTYPE,
  ATTRIBUTE_BASE,
  ATTRIBUTE_BEEP,
  ATTRIBUTE_BRIDGE,
  ATTRIBUTE_CHARSET,
  ATTRIBUTE_CLASSID,
  ATTRIBUTE_CODEBASE,
  ATTRIBUTE_CODETYPE,
  ATTRIBUTE_COND,
  ATTRIBUTE_CONNECTTIME,
  ATTRIBUTE_CONTENT,
  ATTRIBUTE_COUNT,
  ATTRIBUTE_DATA,
  ATTRIBUTE_DEST,
  ATTRIBUTE_DESTEXPR,
  ATTRIBUTE_DTMF,
  ATTRIBUTE_DTMFTERM,
  ATTRIBUTE_ENCTYPE,
  ATTRIBUTE_EVENT,
  ATTRIBUTE_EVENTEXPR,
  ATTRIBUTE_EXPR,
  ATTRIBUTE_EXPRITEM,
  ATTRIBUTE_FETCHAUDIO,
  ATTRIBUTE_FETCHHINT,
  ATTRIBUTE_FETCHTIMEOUT,
  ATTRIBUTE_FINALSILENCE,
  ATTRIBUTE_HTTP_EQUIV,
  ATTRIBUTE_ID,
  ATTRIBUTE_ITEM,
  ATTRIBUTE_LABEL,
  ATTRIBUTE_MAXAGE,
  ATTRIBUTE_MAXSTALE,
  ATTRIBUTE_MAXTIME,
  ATTRIBUTE_MESSAGE,
  ATTRIBUTE_MESSAGEEXPR,
  ATTRIBUTE_METHOD,
  ATTRIBUTE_MODAL,
  ATTRIBUTE_MODE,
  ATTRIBUTE_NAME,
  ATTRIBUTE_NAMEEXPR,
  ATTRIBUTE_NAMELIST,
  ATTRIBUTE_NEXT,
  ATTRIBUTE_NEXTITEM,
  ATTRIBUTE_ROOT,
  ATTRIBUTE_SCOPE,
  ATTRIBUTE_SLOT,
  ATTRIBUTE_SRC,
  ATTRIBUTE_SRCEXPR,
  ATTRIBUTE_TAGFORMAT,
  ATTRIBUTE_TIMEOUT,
  ATTRIBUTE_TRANSFERAUDIO,
  ATTRIBUTE_TYPE,
  ATTRIBUTE_VALUE,
  ATTRIBUTE_VALUETYPE,
  ATTRIBUTE_VERSION,
  ATTRIBUTE_WEIGHT,
  ATTRIBUTE_XMLLANG,
  ATTRIBUTE_XMLNS
};


typedef const VXIchar * const VXML_SYMBOL;

// Attribute values

static VXML_SYMBOL ATTRVAL_TRANSFERTYPE_BRIDGE   = L"bridge";
static VXML_SYMBOL ATTRVAL_TRANSFERTYPE_BLIND    = L"blind";
static VXML_SYMBOL ATTRVAL_TRANSFERTYPE_CONSULTATION = L"consultation";

// Standard Event definitions

static VXML_SYMBOL EV_CANCEL             = L"cancel";
static VXML_SYMBOL EV_TELEPHONE_HANGUP   = L"connection.disconnect.hangup";
static VXML_SYMBOL EV_TELEPHONE_TRANSFER = L"connection.disconnect.transfer";
static VXML_SYMBOL EV_EXIT               = L"exit";
static VXML_SYMBOL EV_HELP               = L"help";
static VXML_SYMBOL EV_NOINPUT            = L"noinput";
static VXML_SYMBOL EV_NOMATCH            = L"nomatch";
static VXML_SYMBOL EV_MAXSPEECH          = L"maxspeechtimeout";

static VXML_SYMBOL EV_ERROR_BADFETCH     = L"error.badfetch";
static VXML_SYMBOL EV_ERROR_BADURI       = L"error.badfetch.baduri";
static VXML_SYMBOL EV_ERROR_BADDIALOG    = L"error.badfetch.baddialog";
static VXML_SYMBOL EV_ERROR_APP_BADURI   = L"error.badfetch.applicationuri";

static VXML_SYMBOL EV_ERROR_SEMANTIC     = L"error.semantic";
static VXML_SYMBOL EV_ERROR_ECMASCRIPT   = L"error.semantic.ecmascript";
static VXML_SYMBOL EV_ERROR_RECORD_PARAM = L"error.semantic.recordparameter";
static VXML_SYMBOL EV_ERROR_BAD_THROW    = L"error.semantic.no_event_in_throw";
static VXML_SYMBOL EV_ERROR_NO_GRAMMARS  = L"error.semantic.nogrammars";

static VXML_SYMBOL EV_ERROR_NOAUTHORIZ   = L"error.noauthorization";
static VXML_SYMBOL EV_ERROR_NORESOURCE   = L"error.noresource";

static VXML_SYMBOL EV_UNSUPPORT_FORMAT   = L"error.unsupported.format";
static VXML_SYMBOL EV_UNSUPPORT_OBJECT   = L"error.unsupported.objectname";
static VXML_SYMBOL EV_UNSUPPORT_LANGUAGE = L"error.unsupported.language";
static VXML_SYMBOL EV_UNSUPPORT_TRANSFER = L"error.unsupported.transfer";
static VXML_SYMBOL EV_UNSUPPORT_RECORD_M = L"error.unsupported.record.modal";
static VXML_SYMBOL EV_UNSUPPORT_BUILTIN  = L"error.unsupported.builtin";

// Transfer events
static VXML_SYMBOL EV_TELEPHONE_NOAUTHORIZ = L"error.connection.noauthorization";
static VXML_SYMBOL EV_TELEPHONE_BAD_DEST = L"error.connection.baddestination";
static VXML_SYMBOL EV_TELEPHONE_NOROUTE  = L"error.connection.noroute";
static VXML_SYMBOL EV_TELEPHONE_NORESOURCE = L"error.connection.noresource";
static VXML_SYMBOL EV_TELEPHONE_UNSUPPORT_BLIND = L"error.unsupported.transfer.blind";
static VXML_SYMBOL EV_TELEPHONE_UNSUPPORT_BRIDGE = L"error.unsupported.transfer.bridge";
static VXML_SYMBOL EV_TELEPHONE_UNSUPPORT_CONSULTATION = L"error.unsupported.transfer.consultation";
static VXML_SYMBOL EV_TELEPHONE_UNSUPPORT_URI = L"error.unsupported.uri";
 
// Outside VXML specification

static VXML_SYMBOL EV_ERROR_BAD_GRAMMAR  = L"error.grammar";
static VXML_SYMBOL EV_ERROR_BAD_INLINE   = L"error.grammar.inlined";
static VXML_SYMBOL EV_ERROR_BAD_CHOICE   = L"error.grammar.choice";
static VXML_SYMBOL EV_ERROR_BAD_OPTION   = L"error.grammar.option";
static VXML_SYMBOL EV_ERROR_RECOGNITION  = L"error.recognition";
static VXML_SYMBOL EV_ERROR_RECORD       = L"error.record";
static VXML_SYMBOL EV_ERROR_TRANSFER     = L"error.transfer";
static VXML_SYMBOL EV_ERROR_OBJECT       = L"error.object";

static VXML_SYMBOL EV_ERROR_STACK_OVERFLOW = L"error.interpreter.stack_overflow";
static VXML_SYMBOL EV_ERROR_LOOP_COUNT     = L"error.interpreter.max_loop_count_exceeded";

// Property names

static const VXIchar * const PROP_BARGEIN             = L"bargein";
static const VXIchar * const PROP_BARGEINTYPE         = L"bargeintype";
static const VXIchar * const PROP_COMPLETETIME        = L"completetimeout";
static const VXIchar * const PROP_CONFIDENCE          = L"confidencelevel";
static const VXIchar * const PROP_INCOMPLETETIME      = L"incompletetimeout";
static const VXIchar * const PROP_INPUTMODES          = L"inputmodes";
static const VXIchar * const PROP_INTERDIGITTIME      = L"interdigittimeout";
static const VXIchar * const PROP_MAXNBEST            = L"maxnbest";
static const VXIchar * const PROP_SENSITIVITY         = L"sensitivity";
static const VXIchar * const PROP_SPEEDVSACC          = L"speedvsaccuracy";
static const VXIchar * const PROP_TERMCHAR            = L"termchar";
static const VXIchar * const PROP_TERMTIME            = L"termtimeout";
static const VXIchar * const PROP_TIMEOUT             = L"timeout";
static const VXIchar * const PROP_UNIVERSALS          = L"universals";
static const VXIchar * const PROP_MAXSPEECHTIME       = L"maxspeechtimeout";
static const VXIchar * const PROP_RECORDUTTERANCE     = L"recordutterance";
static const VXIchar * const PROP_RECORDUTTERANCETYPE = L"recordutterancetype";

#endif
