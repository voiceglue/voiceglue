#ifndef _CommonExceptions_H
#define _CommonExceptions_H

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
 * Common exceptions used by components in the VXI.
 *
 ***********************************************************************/

#include "VXIvalue.h"
#include "DocumentModel.hpp"

class VXIException {
public:
  class OutOfMemory { };

  class JavaScriptError { };

  class StopRequest { };

  class InterpreterEvent {
  public:
    InterpreterEvent(const vxistring & v, const vxistring & m = vxistring(), 
                     const VXMLElement & adialog = 0)
      : val(v), message(m), activeDialog(adialog)  { }
    ~InterpreterEvent()                             { }

    const vxistring & GetValue() const              { return val; }
    const vxistring & GetMessage() const            { return message; }
    const VXMLElement & GetActiveDialog() const     { return activeDialog; }
    
  private:
    vxistring val;
    vxistring message;
    VXMLElement activeDialog;
  };

  class Fatal { };

  class Exit {
  public:
    Exit(VXIValue * v) : exprResult(v) { }

    VXIValue * exprResult;
  };

private: // unimplemented
  VXIException();
};

#endif
