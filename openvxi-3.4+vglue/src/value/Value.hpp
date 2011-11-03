
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

#ifndef _VALUE_HPP
#define _VALUE_HPP

// Forward declarations, required because we mask out the struct definitions
// in VXIvalue.h with the define below
class VXIValue;
class VXIBoolean;
class VXIInteger;
class VXIFloat;
class VXIPtr;
class VXIString;
class VXIContent;
class VXIMap;
class VXIVector;
class VXIMapIterator;
class VXIContentData;
class VXIDouble;
class VXIULong;
class VXILong;

#define VXIVALUE_REAL_STRUCTS
#include "VXIvalue.h"

#ifndef WIN32
extern "C" struct VXItrdMutex;
#endif


/**
 * VXIValue base class
 */
class VXIValue {
 public:
  // Constructor and destructor
  VXIValue (VXIvalueType t) : type(t) { }
  virtual ~VXIValue( ) { }

  // Get the value type
  VXIvalueType GetType( ) const { return type; }

 private:
  VXIvalueType type;
};

/**
 * Basic VXIValue based data types
 */
class VXIBoolean : public VXIValue {
 public:
  // Constructor and destructor
  VXIBoolean (VXIbool v) : VXIValue (VALUE_BOOLEAN), value(v) { }
  virtual ~VXIBoolean( ) { }

  // Get the value
  VXIbool GetValue( ) const { return value; }

 private:
  VXIbool  value;
};

class VXIInteger : public VXIValue {
 public:
  // Constructor and destructor
  VXIInteger (VXIint32 v) : VXIValue (VALUE_INTEGER), value(v) { }
  virtual ~VXIInteger( ) { }

  // Get the value
  VXIint32 GetValue( ) const { return value; }

 private:
  VXIint32  value;
};

class VXIULong : public VXIValue {
 public:
  // Constructor and destructor
  VXIULong (VXIulong v) : VXIValue (VALUE_ULONG), value(v) { }
  virtual ~VXIULong( ) { }

  // Get the value
  VXIulong GetValue( ) const { return value; }

 private:
  VXIulong  value;
};

class VXILong : public VXIValue {
 public:
  // Constructor and destructor
  VXILong (VXIlong v) : VXIValue (VALUE_LONG), value(v) { }
  virtual ~VXILong( ) { }

  // Get the value
  VXIlong GetValue( ) const { return value; }

 private:
  VXIlong  value;
};

class VXIFloat : public VXIValue {
 public:
  // Constructor and destructor
  VXIFloat (VXIflt32 v) : VXIValue (VALUE_FLOAT), value(v) { }
  virtual ~VXIFloat( ) { }

  // Get the value
  VXIflt32 GetValue( ) const { return value; }

 private:
  VXIflt32  value;
};

class VXIDouble : public VXIValue {
 public:
  // Constructor and destructor
  VXIDouble (VXIflt64 v) : VXIValue (VALUE_DOUBLE), value(v) { }
  virtual ~VXIDouble( ) { }

  // Get the value
  VXIflt64 GetValue( ) const { return value; }

 private:
  VXIflt64  value;
};

class VXIPtr : public VXIValue {
 public:
  // Constructor and destructor
  VXIPtr (VXIptr v) : VXIValue (VALUE_PTR), value(v) { }
  virtual ~VXIPtr( ) { }

  // Get the value
  VXIptr GetValue( ) const { return value; }

 private:
  VXIptr        value;
};

// Helper class for VXIContent, non-public
class VXIContentData {
 public:
  // Constructors and destructor
  VXIContentData (const VXIchar  *ct,
		  VXIbyte        *c,
		  VXIulong        csb,
		  void          (*Destroy)(VXIbyte **content, void *userData),
          void          (*GetValue)(void *userData, const VXIbyte *currcontent, const VXIbyte **realcontent, VXIulong* realcontentSizeBytes),
		  void           *ud);
  virtual ~VXIContentData( );

  // Add and remove references
  static void AddRef  (VXIContentData *data);
  static void Release (VXIContentData **data);

 void RetrieveContent( const VXIchar **type, const VXIbyte **content, VXIulong* contentsize ) const;
  // Get the data

  void* GetUserData() const { return userData; }
  const VXIulong  GetContentSizeBytes( ) const { return contentSizeBytes; }

  void SetTransferEncoding(const VXIchar* e);
  const VXIchar* GetTransferEncoding() const { return contentTransfer; }

private:
  const VXIchar  *GetContentType( ) const { return contentType; }
  const VXIbyte  *GetContent( ) const { return content; }
  const VXIchar  *GetContentTransferEncoding( ) const { return contentTransfer; }

 private:
  // Disabled copy constructor and assignment operator
  VXIContentData (const VXIContent &c);
  VXIContent &operator= (const VXIContent &c);

 private:
#ifdef WIN32
  long         refCount;
#else
  VXItrdMutex *mutex;
  VXIulong     refCount;
#endif

  VXIchar     *contentType;
  VXIchar     *contentTransfer;
  VXIbyte     *content;
  VXIulong     contentSizeBytes;
  void       (*Destroy)(VXIbyte **content, void *userData);
  void       (*GetValue)(void *userData, const VXIbyte *currcontent, const VXIbyte **realcontent, VXIulong* realcontentSizeBytes);
  void        *userData;
};

class VXIContent : public VXIValue {
 public:
  // Constructors and destructor
  VXIContent (const VXIchar  *contentType,
	      VXIbyte        *content,
	      VXIulong        contentSizeBytes,
	      void          (*Destroy)(VXIbyte **content, void *userData),
          void          (*GetValue)(void *userData, const VXIbyte *currcontent, const VXIbyte **realcontent, VXIulong* realcontentSizeBytes),
	      void           *userData) : 
    VXIValue(VALUE_CONTENT), details(NULL) {
    details = new VXIContentData (contentType, content, contentSizeBytes,
				  Destroy, GetValue, userData); }
  VXIContent (const VXIContent &c) : 
    VXIValue(VALUE_CONTENT), details(c.details) { 
    VXIContentData::AddRef (details); }
  virtual ~VXIContent( ) { VXIContentData::Release (&details); }

  void RetrieveContent( const VXIchar **type, const VXIbyte **content, VXIulong* contentsize ) const{
    if( details )
      details->RetrieveContent(type,content,contentsize);
  }

  VXIbool IsValid() {
    return (details != NULL );
  }

  void* GetUserData() const{
    if( details )
      return details->GetUserData();
    return NULL;
  }

  void SetTransferEncoding( const VXIchar* e ) const{
    if( details )
      details->SetTransferEncoding(e);
  }
  const VXIchar* GetTransferEncoding() const{
    if( details )
      return details->GetTransferEncoding();
    return NULL;
  }

  VXIulong GetContentSizeBytes( ) const {
    if( details )
      return details->GetContentSizeBytes();
    return 0;
  }

  // Assignment operator
  VXIContent &operator= (const VXIContent &c) {
    if ( &c != this ) {
      VXIContentData::Release (&details);
      details = c.details;
      VXIContentData::AddRef (details);
    }
    return *this;
  }

 private:
  VXIContentData  *details;
};

#endif  /* include guard */
