#ifndef _VXML_Document_HPP
#define _VXML_Document_HPP

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
 * Definition of a VXML document and its serialization interface.
 *
 ***********************************************************************/

#include "DocumentModel.hpp"

class SerializerOutput {
public:
  virtual void Write(const VXIbyte * data, VXIulong size) = 0;

  SerializerOutput()  { }
  virtual ~SerializerOutput()  { }
};


class SerializerInput {
public:
  virtual VXIulong Read(VXIbyte * buffer, VXIulong bufSize) = 0;

  SerializerInput()  { }
  virtual ~SerializerInput()  { }
};


class VXMLDocument {
public:
  VXMLDocument(VXMLDocumentRep * = NULL);
  VXMLDocument(const VXMLDocument &);
  VXMLDocument & operator=(const VXMLDocument &);
  ~VXMLDocument();

  VXMLElement GetRoot() const;
  void GetBaseURL(vxistring & baseuri) const;
  void SetBaseURL(const vxistring & baseuri);
  void GetDefaultLang(vxistring & defaultLanguage) const;
  void SetDefaultLang(const vxistring & defaultLanguage);
  
  // Begin Note: The following functions have been obsoleted
  // Do not attempt to use them!!!
  void WriteDocument(SerializerOutput &) const;
  void ReadDocument(SerializerInput &);
  // End Note
  
  class SerializationError { };
  // This exception may be generated during serialization attempts.

private:
  VXMLDocumentRep * internals;
};

#endif // #define _VXML_Document_HPP
