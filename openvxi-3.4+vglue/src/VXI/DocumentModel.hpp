#ifndef _DOCUMENT_MODEL__HPP_
#define _DOCUMENT_MODEL__HPP_

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

#include "VXIvalue.h"
#include "VXML.h"

class VXMLElement;
class VXMLNodeRef;
class VXMLNodeIterator;
class VXMLNodeIteratorData;
class VXMLDocumentRep;

enum DocumentLevel {
  DOCUMENT,         // Current active document
  APPLICATION,      // Root Application
  DEFAULTS          // Default application
};

class VXMLDocumentModel {
public:
  static bool Initialize();
  static void Deinitialize();

  static void CreateHiddenVariable(vxistring &);
  static bool IsHiddenVariable(const vxistring &);

  class OutOfMemory { };
  class InternalError { };
};


class VXMLNode {
  friend class VXMLNodeIterator;

public:
  enum VXMLNodeType {
    Type_VXMLContent,              // This node represents a leaf.
    Type_VXMLElement,              // This node represents a branch.
    Type_VXMLNode                  // This node is not initialized.
  };

  VXMLNode(const VXMLNodeRef * i = NULL) : internals(i) { }
  virtual ~VXMLNode()          { }

  VXMLNode(const VXMLNode & x);
  VXMLNode & operator=(const VXMLNode &);

public:
  VXMLNodeType GetType() const;
  VXMLElement GetParent() const;

  bool operator!=(const VXMLNode &x) const  { return internals != x.internals;}
  bool operator==(const VXMLNode &x) const  { return internals == x.internals;}

  bool operator==(int a) const               { if (a != 0) return false;
                                               return (internals == NULL); }

  bool operator!=(int a) const               { if (a != 0) return false;
                                               return (internals != NULL); }

protected:
  const VXMLNodeRef * internals;
};


class VXMLNodeIterator {
public: // Creators
  VXMLNodeIterator(const VXMLNode &);
  ~VXMLNodeIterator();

public: // Manipulators
  void operator++();                 // Increments iterator to next child
  VXMLNode operator*() const;        // Returns corresponding node
  operator const void *() const;     // Returns 0 if iteration is invalid
  void reset();                      // Resets iterator to first child

private: // not implemented
  VXMLNodeIterator(const VXMLNodeIterator &);
  VXMLNodeIterator & operator=(const VXMLNodeIterator &);

  VXMLNodeIteratorData * data;
};


class VXMLContent : public VXMLNode {
public:
  virtual ~VXMLContent() { }
  VXMLContent(const VXMLNodeRef * ref = NULL) : VXMLNode(ref) { }

public:
  const vxistring & GetValue() const;
  VXIulong GetSSMLHeaderLen() const;

private: // Not implemented
  VXMLContent(const VXMLContent & x);
  VXMLContent & operator=(const VXMLNode &);
  VXMLContent(const VXMLElement * p);
};


class VXMLElement : public VXMLNode {
public: // Creation & comparison functions
  VXMLElement(const VXMLNodeRef * ref = NULL);
  VXMLElement(const VXMLElement & x);
  virtual ~VXMLElement() { }

  // Navigation functions (moving up & down the tree)
  bool hasChildren() const;

  // Information retrieval
  VXMLElementType GetName() const;
  bool GetAttribute(VXMLAttributeType key, vxistring & attr) const;
  DocumentLevel GetDocumentLevel() const;

private:
  VXMLElement & operator=(const VXMLNode &);
};

#endif // #ifndef _DOCUMENT_MODEL__HPP_
