
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

#include "DocumentModel.hpp"

class VXMLDocumentRep {
public:
  const VXMLNodeRef * GetRoot() const;

  void StartElement(VXMLElementType n, DocumentLevel dlevel);
  void PruneWhitespace();
  void EndElement();

  void AddContent(const VXIchar * c, unsigned int len);
  void MarkSSMLHeader(VXIint len = -1);
  void AddAttribute(VXMLAttributeType name, const vxistring & attr);

  bool GetAttribute(VXMLAttributeType key, vxistring & attr) const;

  VXMLDocumentRep();

  static bool AddRefAtomic(VXMLDocumentRep * t);
  static bool AddRef(VXMLDocumentRep * t);
  static bool ReleaseAtomic(VXMLDocumentRep * & t);
  static bool Release(VXMLDocumentRep * & t);
  static bool IsValidDocRep(VXMLDocumentRep * t);
  static void IfValidRemoveDocRep(VXMLDocumentRep * t);

  VXMLElementType GetParentType() const;

private:
  void AddChild(VXMLNodeRef *);

private:
  ~VXMLDocumentRep();                                   // only used by Release
  VXMLDocumentRep(const VXMLDocumentRep &);             // not implemented
  VXMLDocumentRep & operator=(const VXMLDocumentRep &); // not implemented

  const VXMLNodeRef * root;
  VXMLNodeRef * pos;
  VXMLNode::VXMLNodeType posType;
  int count;
public:
  vxistring rootBaseURL;
  vxistring defaultLang;
};
