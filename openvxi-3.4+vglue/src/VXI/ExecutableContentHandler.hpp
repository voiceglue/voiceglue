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

#ifndef _EXECUTABLECONTENTHANDLER_H_
#define _EXECUTABLECONTENTHANDLER_H_

#include "VXIvalue.h"
#include "DocumentModel.hpp"

class ExecutableContentHandler
{
public:
  virtual void executable_element(const VXMLElement & elem, const VXMLElement & activeDialog = 0) = 0;

  virtual void assign_element(const VXMLElement& elem) = 0;
  virtual void clear_element(const VXMLElement& elem) = 0;
  virtual void data_element(const VXMLElement & elem) = 0;
  virtual void disconnect_element(const VXMLElement& elem) = 0;
  virtual void goto_element(const VXMLElement& elem) = 0;
  virtual void exit_element(const VXMLElement& elem) = 0;
  virtual void if_element(const VXMLElement& elem) = 0;
  virtual void log_element(const VXMLElement& elem) = 0;
  virtual void reprompt_element(const VXMLElement& elem, const VXMLElement & activeDialog = 0) = 0;
  virtual void return_element(const VXMLElement& elem) = 0;
  virtual void script_element(const VXMLElement& elem) = 0;
  virtual void submit_element(const VXMLElement& elem) = 0;
  virtual void throw_element(const VXMLElement& elem, const VXMLElement & activeDialog = 0) = 0;
  virtual void var_element(const VXMLElement & elem) = 0;
};

#endif
