
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

#ifndef __ANSWER_PARSER_HPP__
#define __ANSWER_PARSER_HPP__

#include "VXItypes.h"
#include <parsers/XercesDOMParser.hpp>

extern "C" struct VXIContent;
extern "C" struct VXIValue;
class SimpleLogger;
class AnswerHolder;

class AnswerTranslator {
public:
  virtual void EvaluateExpression(const vxistring & expression) = 0;
  virtual void SetString(const vxistring & var, const vxistring & val) = 0;

  AnswerTranslator() { }
  virtual ~AnswerTranslator() { }
};


class AnswerParser {
public:
  enum ParseResult {
    Success,
	NoInput,
	NoMatch,
	ParseError,
	UnsupportedContent,
	InvalidParameter
  };

  /**
   * Parses a NLSML recognition result.
   */
  ParseResult Parse(SimpleLogger & log, AnswerTranslator &, GrammarManager &, int, VXIContent *, VXMLElement &);

  AnswerParser();
  ~AnswerParser();

private:
  // Returns: -1 Parse error
  //           0 Success - application.lastresult$ written
  //           1 Success - noinput event
  //           2 Success - nomatch event
  int ProcessNLSML(SimpleLogger & log, AnswerTranslator &, GrammarManager &, int, VXMLElement &);

  bool IsAllWhiteSpace(const XMLCh* str);

private:
  xercesc::XercesDOMParser * nlsmlParser;
};

#endif
