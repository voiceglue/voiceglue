
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

#ifndef _VXIVALUE_H
#define _VXIVALUE_H

#include "VXItypes.h"                  /* For VXIchar, VXIint32, etc. */

#include "VXIheaderPrefix.h"
#ifdef VXIVALUE_EXPORTS
#define VXIVALUE_API SYMBOL_EXPORT_DECL
#else
#define VXIVALUE_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * \defgroup VXIvalue Type library
 *
 * Abstract run-time types for VXI interferences and optionally
 * implementation. These types mirror ECMAScript types (and could be
 * implemented as such). They could also be implemented using C++ and
 * STL, or using C. Using these abstract types rather than directly
 * using an externally defined class library increases portability,
 * and allows the implementers of each interface to independantly
 * select external class libraries (or none at all) for their own
 * implementation.<p>
 *
 * Each type is implemented as a handle that is obtained from a
 * constructor and supports run-time typing. The owner (creator) of
 * each type is responsible for freeing it by calling the appropriate
 * destructor.<p>
 *
 * Note: When errors occur, constructors return a NULL object. 
 * Typically this is due to running out of memory, or a type
 * mismatch for copy constructors.<p>
 *
 * The value types are as follows, note that the naming convention is
 * VXI followed by the type name starting with an uppercase letter, while
 * the simple VXI types in VXIvalue.h use VXI followed by the type name
 * starting with a lowercase letter:<p>
 *
 * <table border=0>
 * <tr>
 * <td valign=top> <b>VXIValue</b></td>
 * <td valign=top> Abstract base type for all the rest, can cast any type to
 *      this type and still determine the original type. Used
 *      to provide input/return values where the actual underlying
 *      type can vary. </td>
 * </tr>
 * <tr>
 * <td valign=top><b>VXIBoolean</b></td><td>   Container for a boolean (VXIbool) </td>
 * </tr>
 * <tr>
 * <td valign=top><b>VXIInteger</b></td><td>   Container for a 32-bit integer (VXIint32) </td>
 * </tr>
 * <tr>
 * <td valign=top><b>VXILong</b></td><td>      Container for a 32-bit long (VXIlong) </td>
 * </tr>
 * <tr>
 * <td valign=top><b>VXIULong</b></td><td>     Container for a 32-bit unsigned int (VXIulong) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIFloat</b></td><td>     Container for a 32-bit float type (VXIflt32) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIDouble</b></td><td>    Container for a 32-bit double type (VXIflt64) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIString</b></td><td>    Container for a string (VXIchar) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIPtr</b></td><td>       Container for a untyped pointer (VXIptr) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIContent</b></td><td>   Container for MIME content typed data (VXIptr) </td>
 * </tr>
 * <tr>
 * <td valign=top><b> VXIMap</b></td>
 * <td>    Simple key/value container where the keys are of VXIchar 
 *         type and the values are any of the abstract types defined 
 *        here.</td>
 * </tr>
 * <tr>
 * <td valign=top><b>  VXIVector</b></td>
 *  <td>    Simple indexed vector that supports appending elements 
 *          at the end, and getting and setting elements by index.
 *          There is no support for removing elements or insertions. </td>
 * </tr>
 * </table>
 */

/*@{*/

/* 
 * 
 * If real structures aren't already declared 
 */
#ifndef VXIVALUE_REAL_STRUCTS  

/*
 * Define each type as an opaque structure to get full information hiding
 * while ensuring strong compiler type checks. The actual implementation
 * defines these structures however it wants.
 */
#ifdef __cplusplus
struct VXIValue;
struct VXIBoolean;
struct VXIInteger;
struct VXIFloat;
struct VXIString;
struct VXIPtr;
struct VXIContent;
struct VXIMap;
struct VXIVector;
struct VXIDouble;
struct VXIULong;
struct VXILong;
  
struct VXIMapIterator;
#else

typedef struct VXIValue   { void * dummy; } VXIValue;
typedef struct VXIBoolean { void * dummy; } VXIBoolean;
typedef struct VXIInteger { void * dummy; } VXIInteger;
typedef struct VXIFloat   { void * dummy; } VXIFloat;
typedef struct VXIString  { void * dummy; } VXIString;
typedef struct VXIPtr     { void * dummy; } VXIPtr;
typedef struct VXIContent { void * dummy; } VXIContent;
typedef struct VXIMap     { void * dummy; } VXIMap;
typedef struct VXIVector  { void * dummy; } VXIVector;
typedef struct VXIDouble  { void * dummy; } VXIDouble;
typedef struct VXIULong   { void * dummy; } VXIULong;
typedef struct VXILong    { void * dummy; } VXILong;

typedef struct VXIMapIterator  { void * dummy; } VXIMapIterator;
#endif

#define VXIArgs VXIMap /* For backward compatibility use only */

#endif /* VXIVALUE_REAL_STRUCTS */


/*
 * VXI Value types:
 *  0x0000 - 0x00FF           reserved for definition in this header
 *  0x0100 - 0xFFFF           reserved for implementation specific definition
 */
typedef VXIint32 VXIvalueType;
enum {
  VALUE_INTEGER    = 0,
  VALUE_FLOAT      = 1,
  VALUE_STRING     = 2,
  VALUE_PTR        = 3,
  VALUE_MAP        = 4,
  VALUE_VECTOR     = 5,
  VALUE_CONTENT    = 6,
  VALUE_BOOLEAN    = 7,
  VALUE_DOUBLE     = 8,
  VALUE_LONG       = 9,
  VALUE_ULONG      = 10
};


/**
 * Result codes for function methods
 * 
 * Result codes less than zero are severe errors (likely to be
 * platform faults), those greater than zero are warnings (likely to
 * be application issues) 
 */
typedef enum VXIvalueResult {
  /* Fatal error, terminate call    */
  VXIvalue_RESULT_FATAL_ERROR       =  -100, 
  /* I/O error                      */
  VXIvalue_RESULT_IO_ERROR           =   -8,
  /* Out of memory                  */
  VXIvalue_RESULT_OUT_OF_MEMORY      =   -7, 
  /* System error, out of service   */
  VXIvalue_RESULT_SYSTEM_ERROR       =   -6, 
  /* Errors from platform services  */
  VXIvalue_RESULT_PLATFORM_ERROR     =   -5, 
  /* Return buffer too small        */
  VXIvalue_RESULT_BUFFER_TOO_SMALL   =   -4, 
  /* Property name is not valid    */
  VXIvalue_RESULT_INVALID_PROP_NAME  =   -3, 
  /* Property value is not valid   */
  VXIvalue_RESULT_INVALID_PROP_VALUE =   -2, 
  /* Invalid function argument      */
  VXIvalue_RESULT_INVALID_ARGUMENT   =   -1, 
  /* Success                        */
  VXIvalue_RESULT_SUCCESS            =    0,
  /* Normal failure, nothing logged */
  VXIvalue_RESULT_FAILURE            =    1,
  /* Non-fatal non-specific error   */
  VXIvalue_RESULT_NON_FATAL_ERROR    =    2, 
  /* Operation is not supported     */
  VXIvalue_RESULT_UNSUPPORTED        =  100
} VXIvalueResult;


/**
 * Get the type of a Value
 *
 * @param   v   Value to check
 * @return      Type of value
 */
VXIVALUE_API VXIvalueType VXIValueGetType(const VXIValue *v);

/**
 * Generic Value destructor
 *
 * This automatically invokes the appropriate type specific
 * destructor.
 *
 * @param   v   Value to destroy
 */
VXIVALUE_API void VXIValueDestroy(VXIValue **v);

/**
 * Generic Value clone
 *
 * This automatically invokes the appropriate type specific clone
 * operation.
 *
 * @param   v   Value to clone
 * @return      Clone of v, NULL on error
 */
VXIVALUE_API VXIValue *VXIValueClone(const VXIValue *v);

/**
 * Create a Boolean from a VXIbool
 *
 * @param   n   VXIbool value, either TRUE or FALSE
 * @return      Boolean with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXIBoolean *VXIBooleanCreate(VXIbool n);

/**
 * Boolean destructor
 *
 * @param   i   Boolean to destroy
 */
VXIVALUE_API void VXIBooleanDestroy(VXIBoolean **i);

/**
 * Get the value of a Boolean
 *
 * @param   i   Boolean to obtain the value from
 * @return      VXIbool boolean value, either TRUE or FALSE
 */
VXIVALUE_API VXIbool VXIBooleanValue(const VXIBoolean *i);

/**
 * Create an Integer from a 32 bit integer
 *
 * @param   n   32 bit integer value
 * @return      Integer with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXIInteger *VXIIntegerCreate(VXIint32 n);

/**
 * Integer destructor
 *
 * @param   i   Integer to destroy
 */
VXIVALUE_API void VXIIntegerDestroy(VXIInteger **i);

/**
 * Get the value of an Integer
 *
 * @param   i   Integer to obtain the value from
 * @return      32 bit integer value
 */
VXIVALUE_API VXIint32 VXIIntegerValue(const VXIInteger *i);

/**
 * Create an Long from a 32 bit long
 *
 * @param   n   32 bit long value
 * @return      Long with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXILong *VXILongCreate(VXIlong n);

/**
 * Long destructor
 *
 * @param   i   Long to destroy
 */
VXIVALUE_API void VXILongDestroy(VXILong **i);

/**
 * Get the value of an Long
 *
 * @param   i   Long to obtain the value from
 * @return      32 bit long value
 */
VXIVALUE_API VXIlong VXILongValue(const VXILong *i);

/**
 * Create an ULong from a 32 bit unsigned long
 *
 * @param   n   32 bit unsigned long value
 * @return      ULong with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXIULong *VXIULongCreate(VXIulong n);

/**
 * ULong destructor
 *
 * @param   i   ULong to destroy
 */
VXIVALUE_API void VXIULongDestroy(VXIULong **i);

/**
 * Get the value of an ULong
 *
 * @param   i   ULong to obtain the value from
 * @return      32 bit unsigned long value
 */
VXIVALUE_API VXIulong VXIULongValue(const VXIULong *i);

/**
 * Create a Float from a 32 bit floating point number
 *
 * @param   n   32 bit floating point value
 * @return      Float with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXIFloat *VXIFloatCreate(VXIflt32 n);

/**
 * Float destructor
 *
 * @param   f   Float to destroy
 */
VXIVALUE_API void VXIFloatDestroy(VXIFloat **f);

/**
 * Get the value of a Float
 *
 * @param   f   Float to get the value from
 * @return      32 bit floating point value
 */
VXIVALUE_API VXIflt32 VXIFloatValue(const VXIFloat *f);

/**
 * Create a Double from a 32 bit floating point number
 *
 * @param   n   32 bit floating point value
 * @return      Double with the specified value on success, 
 *              NULL otherwise
 */
VXIVALUE_API VXIDouble *VXIDoubleCreate(VXIflt64 n);

/**
 * Double destructor
 *
 * @param   f   Double to destroy
 */
VXIVALUE_API void VXIDoubleDestroy(VXIDouble **f);

/**
 * Get the value of a Double
 *
 * @param   f   Double to get the value from
 * @return      32 bit floating point value
 */
VXIVALUE_API VXIflt64 VXIDoubleValue(const VXIDouble *f);

/**
 * Create a Ptr from a C pointer
 *
 * Note: This only stores the pointer blindly, it does not perform a
 * deep copy and the reference memory is not freed on
 * destruction. Thus the user is responsible for ensuring the
 * referenced memory location remains valid, and for freeing memory
 * when appropriate on Ptr destruction.
 *
 * @param   n     Pointer to memory
 */
VXIVALUE_API VXIPtr *VXIPtrCreate(void *n);

/**
 * Ptr destructor
 *
 * @param   p   Ptr to destroy
 */
VXIVALUE_API void VXIPtrDestroy(VXIPtr **p);

/**
 * Get the value of a Ptr
 *
 * @param   p   Ptr to retrieve the pointer from
 * @return      Pointer to memory retrieved
 */
VXIVALUE_API void *VXIPtrValue(const VXIPtr *p);

/**
 * Create a Content from MIME content typed data
 *
 * Thread-safe reference counting is used to allow sharing the data
 * (typically large) across multiple clones while minimizing memory
 * use. The passed Destroy( ) function is only called when the
 * reference count drops to zero.
 *
 * @param   contentType       MIME content type for the data
 * @param   content           Data to store, this pointer will merely be
 *                            copied (no deep copy of the data will be done)
 *                            so this pointer must remain valid until the
 *                            Destroy function is called.
 * @param   contentSizeBytes  Size of the data, in bytes
 * @param   Destroy           Destructor called to release the data when
 *                            no longer needed. Since this construction
 *                            merely copies the pointer, this is mandatory.
 * @param   userData          Optional user data pointer passed to destroy,
 *                            typically used to hold a pointer to some
 *                            larger data structure that contains the
 *                            content so that larger data structure can
 *                            be destroyed when the content is no longer
 *                            required.
 */
VXIVALUE_API VXIContent *
VXIContentCreate(const VXIchar  *contentType,
		 VXIbyte        *content,
		 VXIulong        contentSizeBytes,
		 void          (*Destroy)(VXIbyte **content, void *userData),
		 void           *userData);

/**
 * Create a Content from MIME content typed data
 *
 * Thread-safe reference counting is used to allow sharing the data
 * (typically large) across multiple clones while minimizing memory
 * use. The passed Destroy( ) function is only called when the
 * reference count drops to zero.
 *
 * @param   contentType       MIME content type for the data
 * @param   content           Data to store, this pointer will merely be
 *                            copied (no deep copy of the data will be done)
 *                            so this pointer must remain valid until the
 *                            Destroy function is called.
 * @param   contentSizeBytes  Size of the data, in bytes
 * @param   Destroy           Destructor called to release the data when
 *                            no longer needed. Since this construction
 *                            merely copies the pointer, this is mandatory.
 * @param   GetValue          Method called to override the reading of the 
 *                            memory stream from the content, this allows
 *                            for the Content to be a place holder and the real
 *                            memory content could be stored elsewhere.
 * @param   userData          Optional user data pointer passed to destroy,
 *                            typically used to hold a pointer to some
 *                            larger data structure that contains the
 *                            content so that larger data structure can
 *                            be destroyed when the content is no longer
 *                            required.
 */
VXIVALUE_API VXIContent *
VXIContentCreateEx(const VXIchar  *contentType,
		 VXIbyte        *content,
		 VXIulong        contentSizeBytes,
		 void          (*Destroy)(VXIbyte **content, void *userData),
         void          (*GetValue)(void *userData, const VXIbyte *currcontent, const VXIbyte **realcontent, VXIulong* realcontentSizeBytes),
		 void           *userData);


/**
 * Retrieve the unique data pointer they passed to VXIContentCreate
 *
 * @param   c       Content to retrieve the data from
 * @param   u       Returns the unique pointer associated with the content
 */
VXIVALUE_API void VXIContentGetUserData(const VXIContent  *c, void** u);

/**
 * Set the Content-Transfer-Encoding for MIME
 *
 * @param   c       Content to retrieve the data from
 * @param   e       Sets the Content-Transfer-Encoding for MIME
 */
VXIVALUE_API void VXIContentSetTransferEncoding(const VXIContent  *c, const VXIchar  *e);


/**
 * Retrieve the Content-Transfer-Encoding for MIME
 *
 * @param   c       Content to retrieve the data from
 */
VXIVALUE_API const VXIchar* VXIContentGetTransferEncoding(const VXIContent  *c);

/**
 * Retrieve the content size (in bytes)
 *
 * @param   c       Content to retrieve the data from
 */
VXIVALUE_API VXIulong VXIContentGetContentSizeBytes(const VXIContent *c);


/**
 * Content destructor
 *
 * @param   c   Content to destroy
 */
VXIVALUE_API void VXIContentDestroy(VXIContent **c);

/**
 * Get the value of a Content
 *
 * @param   c                 Content to retrieve the data from
 * @param   contentType       Returns the MIME content type for the data
 * @param   content           Returns the pointer to the data
 * @param   contentSizeBytes  Returns the size of the data, in bytes
 * @return                    VXIvalue_RESULT_SUCCESS on success 
 */
VXIVALUE_API VXIvalueResult 
VXIContentValue(const VXIContent  *c,
		const VXIchar    **contentType,
		const VXIbyte    **content,
		VXIulong          *contentSizeBytes);

/**
 * Create a String from a null-terminated character array
 *
 * @param   str   NULL-terminated character array
 * @return        String with the specified value on success, 
 *                NULL otherwise
 */
VXIVALUE_API VXIString *VXIStringCreate(const VXIchar *str);

/**
 * Create a String from a known-length character array
 *
 * @param   str   Character array (null characters may be embedded in 
 *                the array)
 * @param   len   Number of characters which will be copied.
 * @return        String with the specified value on success, 
 *                NULL otherwise
 */
VXIVALUE_API VXIString *VXIStringCreateN(const VXIchar *str, VXIunsigned len);

/**
 * String destructor
 *
 * @param   s   String to destroy
 */
VXIVALUE_API void VXIStringDestroy(VXIString **s);

/**
 * String clone
 *
 * Note: functionally redundant with VXIValueClone( ), but provided to
 * reduce the need for C casts for this common operation
 *
 * @param    s   String to clone
 * @return       Clone of the string on success, NULL otherwise
 */
VXIVALUE_API VXIString *VXIStringClone(const VXIString *s);

/**
 * Set the value of a String from a null-terminated character array
 *
 * Note: this functionality is provided to allow defining interfaces
 * where the caller passes in a VXIString from VXIStringCreate( )
 * (typically with an empty string as its value) with the interface
 * changing that value to return a string as output. This avoids
 * having to define interfaces where the client has to provide a
 * fixed length buffer (and thus worry about "buffer too small" errors
 * and complicated handling).
 *
 * @param   s     String to change the value of
 * @param   str   NULL-terminated character array
 * @return        VXIvalue_RESULT_SUCCESS on success 
 */
VXIVALUE_API VXIvalueResult VXIStringSetValue(VXIString      *s, 
                                              const VXIchar  *str);

/**
 * Get the value of a String
 *
 * @param   s     String to access
 * @param   buf   Character buffer to copy the value into as a
 *                NULL-terminated character array.  The buffer size must be
 *                at least VXIStringLength() + 1.
 * @param   len   Size of the buffer, in characters
 * @return        Pointer to buf on success, NULL on failure (most likely
 *                buffer too small) 
 */
VXIVALUE_API VXIchar *VXIStringValue(const VXIString  *s, 
                                     VXIchar          *buf, 
                                     VXIunsigned       len);

/**
 * Get direct access to the NULL-terminated character value
 *
 * Note: the returned buffer must never be modified, and is only
 * provided for transient use (i.e. immediately logging it, comparing
 * it, etc. rather than storing or returning the pointer for longer
 * term access).
 *
 * @param   s   String to retrieve the data from
 * @return      Pointer to the NULL-terminated character array retrieved
 */
VXIVALUE_API const VXIchar* VXIStringCStr(const VXIString *s);


/**
 * Get the number of characters in a String's value
 *
 * Note: Add one byte for the NULL terminator when using this to determine
 * the length of the array required for a VXIStringValue( ) call.
 *
 * @param   s   String to access
 * @return      Length of the string, in characters
 */
VXIVALUE_API VXIunsigned VXIStringLength(const VXIString *s);

/**
 * Compares two Strings
 *
 * @param   s1   First String to compare
 * @param   s2   Second String to compare
 * @return       Returns a value that is less than, equal to, or greater
 *               than zero depending on whether s1 is lexicographically
 *               less than, equal to, or greater than s2
 */
VXIVALUE_API VXIint VXIStringCompare(const VXIString *s1, 
                                     const VXIString *s2);

/**
 * Compares a String to a NULL-terminated character array
 *
 * @param   str   String to compare
 * @param   buf   NULL-terminated character array to compare
 * @return        Returns a value that is less than, equal to, or greater
 *                than zero depending on whether str is lexicographically
 *                less than, equal to, or greater than buf
 */
VXIVALUE_API VXIint VXIStringCompareC(const VXIString *str, 
                                      const VXIchar   *buf);

/**
 * Create an empty Map
 *
 * @return   New map on success, NULL otherwise
 */
VXIVALUE_API VXIMap *VXIMapCreate(void);

/**
 * Clear the content of the map and return an empty Map
 *
 * @return   None
 */
VXIVALUE_API void VXIMapClear(VXIMap *m);

/**
 * Map destructor
 *
 * Note: this recursively destroys all the values contained within the
 * Map, including all the values of Maps and Vectors stored
 * within this map. However, for Ptr values the user is
 * responsible for freeing the held memory if appropriate.
 *
 * @param m   Map to destroy 
 */
VXIVALUE_API void VXIMapDestroy(VXIMap **m);

/**
 * Map clone
 *
 * Recursively copies all values contained within the map,
 * including all the values of Maps and Vectors stored within this
 * map.
 *
 * Note: functionally redundant with VXIValueClone( ), but provided to
 * reduce the need for C casts for this common operation
 *
 * @param    m   Map to clone
 * @return       Clone of the Map on success, NULL otherwise 
 */
VXIVALUE_API VXIMap *VXIMapClone(const VXIMap *m);

/**
 * Set a named property on an Map
 *
 * The value can be an Map so a tree can be constructed.
 *
 * If the property already exists, the existing value is first
 * destroyed using VXIValueDestroy( ) (thus recursively deleting held
 * values within it if it is an Map or Vector), then does the
 * set operation with the new value.
 *
 * @param   m     Map to access
 * @param   key   NULL terminated property name
 * @param   val   Value to set the property to, ownership is passed
 *                to the Map (a simple pointer copy is done), so on
 *                success the user must not delete, modify, or otherwise
 *                use this. Also be careful to not add a Map as a
 *                property of itself (directly or indirectly), otherwise
 *                infinite loops may occur on access or deletion.
 * @return        VXIvalue_RESULT_SUCCESS on success
 */
VXIVALUE_API VXIvalueResult VXIMapSetProperty(VXIMap         *m, 
                                              const VXIchar  *key,
                                              VXIValue       *val);

/**
 * Get a named property from an Map
 *
 * The property value is returned for read-only access and is
 * invalidated if the Map is modified. The client must clone it if
 * they wish to perform modifications or wish to retain the value even
 * afer modifying this Map.
 *
 * @param   m     Map to access
 * @param   key   NULL terminated property name
 * @return        On success the value of the property for read-only 
 *                access (invalidated if the Map is modified), NULL
 *                if the property was never set or was deleted 
 */
VXIVALUE_API const VXIValue *VXIMapGetProperty(const VXIMap    *m, 
                                               const VXIchar   *key);

/**
 * Delete a named property from an Map
 *
 * This does a VXIValueDestroy( ) on the value for the named property
 * (thus recursively deleting held values within it if it is an Map
 * or Vector). However, for Ptr properties the user is responsible for
 * freeing the held memory if appropriate.
 *
 * @param   m     Map to access
 * @param   key   NULL terminated property name
 * @return        VXIvalue_RESULT_SUCCESS on success 
 */
VXIVALUE_API VXIvalueResult VXIMapDeleteProperty(VXIMap         *m, 
                                                 const VXIchar  *key);

/**
 * Return the number of properties for an Map
 *
 * Note: this only returns the number of properties that are direct
 * children of the Map, it does not include the number of properties
 * held in Maps and Vectors stored within this map.
 *
 * @param   m   Map to access
 * @return      Number of properties stored in the Map
 */
VXIVALUE_API VXIunsigned VXIMapNumProperties(const VXIMap *m);

/**
 * Get the first property of an Map and an iterator
 *
 * Note: this is used to traverse all the properties within an map,
 * there is no guarantee on what order the properties will be
 * returned. The iterator must be eventually freed with
 * VXIMapIteratorDestroy( ), and is invalidated if the Map is
 * modified in any way.
 *
 * @param   m      Map to access
 * @param   key    Set to point at the property name for read-only 
 *                 access (must not be modified)                 
 * @param   value  Set to point at the property value for read-only 
 *                 access (must not be modified)
 * @return         Pointer to an iterator that may be used to get
 *                 additional properties via VXIMapGetNextProperty( ),
 *                 or NULL on failure (typically no properties in the map)
 */
VXIVALUE_API 
VXIMapIterator *VXIMapGetFirstProperty(const VXIMap     *m,
                                       const VXIchar   **key,
                                       const VXIValue  **value);

/**
 * Get the next property of an Map based on an iterator
 *
 * Note: this is used to traverse all the properties within an map,
 * there is no guarantee on what order the properties will be
 * returned.
 *
 * @param   it     Iterator used to access the map as obtained
 *                 from VXIMapGetFirstProperty( ), this operation
 *                 will advance the iterator to the next property on
 *                 success
 * @param   key    Set to point at the property name for read-only 
 *                 access (must not be modified, invalidated if the
 *                 Map is modified)                 
 * @param   value  Set to point at the property value for read-only 
 *                 access (must not be modified, invalidated if the
 *                 Map is modified)
 * @return         VXIvalue_RESULT_SUCCESS on success (property name 
 *                 and value returned, iterator advanced), 
 *                 VXIvalue_RESULT_FAILURE if there are no more properties
 *                 to read, or a VXIvalueResult error code for severe errors
 */
VXIVALUE_API VXIvalueResult VXIMapGetNextProperty(VXIMapIterator  *it,
                                                  const VXIchar  **key,
                                                  const VXIValue **value);

/**
 * Destroy an iterator
 *
 * @param   it     Iterator to destroy as obtained from 
 *                 VXIMapGetFirstProperty( )
 */
VXIVALUE_API void VXIMapIteratorDestroy(VXIMapIterator **it);

/**
 * Create an empty Vector
 *
 * @return   New vector on success, NULL otherwise
 */
VXIVALUE_API VXIVector *VXIVectorCreate(void);

/**
 * Vector destructor
 *
 * Note: this recursively destroys all the values contained within the
 * Vector, including all the values of Vectors stored within this
 * vector. However, for Ptr values the user is responsible for
 * freeing the held memory if appropriate.
 *
 * @param   v   Vector to destroy 
 */
VXIVALUE_API void VXIVectorDestroy(VXIVector **v);

/**
 * Vector clone
 *
 * Recursively copies all values contained within the vector,
 * including all the values of Vectors and Maps stored within this
 * vector.
 *
 * Note: functionally redundant with VXIValueClone( ), but provided to
 * reduce the need for C casts for this common operation
 *
 * @param    v   Vector to clone
 * @return Clone of the Vector on success, NULL otherwise */
VXIVALUE_API VXIVector *VXIVectorClone(const VXIVector *v);

/**
 * Adds an element to the end of the Vector
 *
 * The value can be a Vector so frames can be implemented.
 *
 * @param   v    Vector to access
 * @param   val  Value to append to the vector, ownership is passed
 *               to the Vector (a simple pointer copy is done), so on
 *               success the user must not delete, modify, or otherwise
 *               use this. Also be careful to not add a Vector as a
 *               element of itself (directly or indirectly), otherwise
 *               infinite loops may occur on access or deletion.
 * @return       VXIvalue_RESULT_SUCCESS on success
 */
VXIVALUE_API VXIvalueResult VXIVectorAddElement(VXIVector      *v, 
                                                VXIValue       *val);

/**
 * Set an indexed vector element
 *
 * Overwrites the specified element with the new value. The existing
 * value is first destroyed using VXIValueDestroy( ) (thus recursively
 * deleting held values within it if it is an Map or Vector), then
 * does the set operation with the new value.
 *
 * The value can be a Vector so frames can be implemented.
 *
 * @param   v     Vector to access
 * @param   n     Element index to set, it is an error to pass a
 *                index that is greater then the number of values
 *                currently in the vector
 * @param   val   Value to set the element to, ownership is passed
 *                to the Vector (a simple pointer copy is done), so on
 *                success the user must not delete, modify, or otherwise
 *                use this. Also be careful to not add a Vector as a
 *                element of itself (directly or indirectly), otherwise
 *                infinite loops may occur on access or deletion.
 * @return        VXIvalue_RESULT_SUCCESS on success
 */
VXIVALUE_API VXIvalueResult VXIVectorSetElement(VXIVector      *v, 
                                                VXIunsigned     n, 
                                                VXIValue       *val);

/**
 * Get an indexed vector element
 *
 * The element value is returned for read-only access and is
 * invalidated if the Vector is modified. The client must clone it if
 * they wish to perform modifications or wish to retain the value even
 * after modifying this Vector.
 *
 * @param   v     Vector to access
 * @param   n     Element index to set, it is an error to pass a
 *                index that is greater or equal to then the number of values
 *                currently in the vector (i.e. range is 0 to length-1)
 * @return        On success the value of the property for read-only 
 *                access (invalidated if the Vector is modified), NULL
 *                on error 
 */
VXIVALUE_API const VXIValue *VXIVectorGetElement(const VXIVector *v, 
                                                 VXIunsigned      n);

/**
 * Return number of elements in a Vector
 *
 * This computes only the length of the Vector, elements within
 * Vectors and Maps within it are not counted.
 *
 * @param   v    Vector to access
 * @return       Number of elements stored in the Vector
 */
VXIVALUE_API VXIunsigned VXIVectorLength(const VXIVector *v);

/**
 * Formats for the string result of VXIValueToString, currently:
 *
 * VALUE_FORMAT_URL_QUERY_ARGS
 *
 * URL query argument format as specified in IETF RFC 2396. Note: this
 * encoding is not appropriate for generically serializing and later
 * restoring VXIValue based types, see below.

 * This will return a string of the format
 * "key1=value1&key2=value2[...]" where '=' seperates keys and '&'
 * seperates key/value pairs. Map keys are output by using dot
 * notation such as "mymap.mymember=true" for "mymember" in "mymap".
 * Similarly, vector keys are output using dot notation, such as
 * "myvec.1=200" for myvec element 1. This dot notation may be
 * arbitrarily deep for handling nested maps/vectors. Boolean values
 * are output as "true" or "false", and VXIContent values are encoded
 * by inserting the escaped bytes (see below).
 *
 * As required by IETF RFC 2396, all keys and values are escaped to
 * ensure the resulting string is only composed of a subset of visible
 * ASCII characters. All other characters/bytes (including a percent
 * sign) are encoded as a percent sign followed by the byte value such
 * as "%20" for a space. Since VXIMap key names and VXIStrings are
 * wchar_t based data that may include Unicode characters, each
 * character in those are first converted to the Unicode UTF-8
 * character encoding (where each character is represented by 1 to 6
 * bytes, with the UTF-8 byte code and ASCII byte codes identical for
 * the ASCII character set, and Latin 1 [ISO-8859] and Unicode
 * characters consuming 2 or more bytes), then each byte is escaped as
 * necessary. NOTE: The use of UTF-8 to encode Latin 1 and Unicode
 * characters is Vocalocity defined and thus may not seemlessly
 * interoperate with other software: IETF RFC 2396 acknoledges the
 * issue of the transmission of non-ASCII character set characters,
 * and allows for the use of UTF-8, but does not mandate the use of
 * UTF-8 to solve this problem and thus encoding choices vary between
 * systems (although UTF-8 is the clearest choice).
 *
 * Note that with this format the types of each key is ambiguous: for
 * example the VXIString "100" and the VXIInteger 100 have an
 * identical representation, and VXIContent byte streams are not
 * distinguishable from other types (particulary VXIStrings) unless
 * they contain byte codes that are not permissable in the other
 * types, such as NULL (0) bytes.  When used for HTTP operations, this
 * ambiguity is not an issue, as the target CGI/servlet/etc. knows
 * what data to expect and thus the appropriate data type for each
 * key. Thus while useful for serializing and transmitting application
 * defined data over HTTP or other ASCII based protocols for delivery
 * to the application elsewhere, this encoding is not appropriate for
 * generically serializing and later restoring VXIValue based types.
 */
typedef enum VXIValueStringFormat {
  VALUE_FORMAT_URL_QUERY_ARGS
} VXIValueStringFormat;


/**
 * Generic Value to string conversion
 *
 * This converts any VXIValue type to a string.
 *
 * @param   v       Value to convert to a string
 * @param   name    Name to use for labeling the VXIValue data
 * @param   format  Format to use for the string, see above
 * @return          VXIString, NULL on error
 */
VXIVALUE_API VXIString *VXIValueToString(const VXIValue      *v,
					 const VXIchar       *name,
					 VXIValueStringFormat format);

#ifdef __cplusplus
} /* This ends the extern "C". */


/**
 * C++ wrapper class that makes it easier to work with VXIMaps
 *
 * Used correctly, this class can eliminate common memory leaks associated with
 * VXIMap usage.  For instance:<p>
 * \code
 *   VXIMap * GenerateMapFromParams(void)
 *   { 
 *     VXIMapHolder result;
 *     DoStuff();  // this code may throw an exception.
 *     return result.Release();
 *   }
 * \endcode
 * If an exception is raised by the DoStuff function, the map will be cleaned
 * up automatically when the stack is unwound.  Likewise, the calling code may
 * benefit:<p>
 * \code
 *   int Foo(void)
 *   {
 *     VXIMapHolder params(GenerateMapFromParams());
 *     ProcessParams(params);  // This code may throw an exception.
 *     return 0;
 *   }
 * \endcode
 * If either an exception occurs or the function exits normally, the associated
 * map will be freed.<p>
 */
class VXIMapHolder {
public:
  /**
   * Creates an empty VXIMap.
   */
  VXIMapHolder() : _map(NULL)        { _map = VXIMapCreate(); }

  /**
   * Takes ownership of a VXIMap.
   * @param m The VXIMap to own
   */
  VXIMapHolder(VXIMap * m) : _map(m) { }

  /**
   * Destorys owned VXIMap.
   */
  ~VXIMapHolder()                    { if (_map != NULL) VXIMapDestroy(&_map);}

  /**
   */
  VXIMapHolder & operator=(const VXIMapHolder & x)
  { if (this != &x) {
      if (_map != NULL) VXIMapDestroy(&_map);
      _map = VXIMapClone(x._map); }
    return *this; }

  /** 
   * Returns the internal map.  The standard map manipulation
   * functions may then be used.
   */
  VXIMap * GetValue() const       { return _map; }

  /**
   * Give up ownership of the contained VXIMap.
   *
   * This does not destroy the contained VXIMap.  Rather, a 
   * a pointer is returned, and it is the callers responsibility
   * to destroy the map.
   *
   * @return The contained VXIMap.
   */
  VXIMap * Release()              { VXIMap * m = _map; _map = NULL; return m; }

  /**
   * Acquires a new map, destroying the contained one first.
   */
  void Acquire(VXIMap * m)        { if (_map != NULL) VXIMapDestroy(&_map);
                                    _map = m; }

  /**
   * Clones the contained map to a new VXIMapHolder.
   */
  VXIMapHolder(const VXIMapHolder & mh) {
    if(mh._map) {
      _map = VXIMapClone(mh._map);
    }
  }

private:
  VXIMap * _map;
};

/**
 * C++ wrapper class that makes it easier to work with VXIContent.
 */
class VXIContentHolder {
public:
  /**
   */
  VXIContentHolder() : _content(NULL), _contentSize(0),
                       _contentData(NULL), _contentType(NULL) { }

  /**
   * Takes ownership of an existing VXIContent.
   * @param c An existing VXIContent.
   */
  VXIContentHolder(VXIContent* c) : _content(NULL){ Acquire(c); }

  /**
   * Destroys the contained VXIContent.
   */
  virtual ~VXIContentHolder()    { if (_content) VXIContentDestroy(&_content); }

  /**
   * Returns the VXIContent content.
   * Same as would be returned by VXIContentCreate.
   */
  const VXIbyte* GetContent()    { return _contentData; }

  /**
   * Returns the VXIContent type.
   * Same as would be returned by VXIContentCreate.
   */
  const VXIchar* GetType()       { return _contentType; }

  /**
   * Returns the VXIContent size.
   * Same as would be returned by VXIContentCreate.
   */
  VXIulong GetSize()             { return _contentSize; }

  /** 
   * Returns the internal map.
   */
  const VXIContent* GetValue() { return _content; }

  /**
   * Give up ownership of the contained VXIContent.
   *
   * This does not destroy the contained VXIContent.  Rather, a 
   * a pointer is returned, and it is the callers responsibility
   * to destroy the map with a call to VXIContentDestroy.
   *
   * @return The contained VXIContent.
   */
  VXIContent* Release(void)
    { VXIContent* c = _content; _content = NULL; return c; }

  /**
   * Creates a new contained VXIContent, destroying the current
   * content if needed.
   * @param   contentType       MIME content type for the data
   * @param   content           Data to store, this pointer will merely be
   *                            copied (no deep copy of the data will be done)
   *                            so this pointer must remain valid until the
   *                            Destroy function is called.
   * @param   contentSizeBytes  Size of the data, in bytes
   * @param   Destroy           Destructor called to release the data when
   *                            no longer needed. Since this construction
   *                            merely copies the pointer, this is mandatory.
   * @param   userData          Optional user data pointer passed to destroy,
   *                            typically used to hold a pointer to some
   *                            larger data structure that contains the
   *                            content so that larger data structure can
   *                            be destroyed when the content is no longer
   *                            required.
   */
  bool Create(
      const VXIchar  *contentType,
      VXIbyte        *content,
      VXIulong        contentSizeBytes,
      void          (*Destroy)(VXIbyte **content, void *userData),
      void           *userData)
  {
    if (_content) VXIContentDestroy(&_content);
	_content = VXIContentCreate(contentType, content, contentSizeBytes, Destroy, userData);
	return _content != NULL;
  }

  /**
   * Acquires an existing VXIContent, destroying the contained 
   * content if needed.
   */
  void Acquire(VXIContent* content){
    if (_content) VXIContentDestroy(&_content);
    _content = content; 
    if( _content )
      VXIContentValue(_content, &_contentType, &_contentData, &_contentSize);
	else {
      _contentType = NULL;
      _contentData = NULL;
      _contentSize = 0;
    }
  }
  
private:
  VXIContent* _content;
  const VXIchar* _contentType;
  const VXIbyte* _contentData;
  VXIulong _contentSize;
  
  // do not allow these operations
  VXIContentHolder(const VXIContentHolder &);
  VXIContentHolder & operator=(const VXIContentHolder &);
};


/*@}*/
#endif /* end #ifdef _cplusplus */

#include "VXIheaderSuffix.h"

#endif  /* include guard */
