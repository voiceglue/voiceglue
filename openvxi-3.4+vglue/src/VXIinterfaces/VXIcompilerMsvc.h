
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

/************************************************************************
 *
 *
 * Settings specific to the Microsoft Visual C++ compiler
 *
 ************************************************************************
 */

#if !defined(_MSC_VER)
#error This file should not be included.
#endif
#if (_MSC_VER < 1200) 
#error Visual C++ versions prior to 6.0 are not supported.
#endif

/* error if calling a function not prototyped */
#pragma warning(1 : 4013 4071 4072)

/* warning if empty statement at global scope */
#pragma warning(3 : 4019)

/* warning if two pointers in an expression point to different base types. */
#pragma warning(3 : 4057)

/* warning if enumerate not an explicit case inside a switch statement */
#pragma warning(3 : 4061)

/* warning if enumerate not a case inside a switch statement and there is no
   default case */
#pragma warning(3 : 4062)

/* warning if comparing to string constant */
#pragma warning(3 : 4130)

/* warning if const variable not initialized */
#pragma warning(3 : 4132)

/* warning if statement has no effect */
#pragma warning(3 : 4705)

/* warning if assignment within conditional expression */
#pragma warning(3 : 4706)

/* warning if comma inside array index */
#pragma warning(3 : 4709)

/* error if c++ style comments in c code */
#pragma warning(error : 4001)

/* error if too many arguments to a macro */
#pragma warning(error : 4002)

/* error if too few  arguments to a macro */
#pragma warning(error : 4003)

/* error if incorrect usage of "defined" operator */
#pragma warning(error : 4004)

/* error if too many arguments to a function */
#pragma warning(error : 4020)

/* error if too few  arguments to a function */
#pragma warning(error : 4021)

/* error if a function's parameter list is declared non-empty but defined 
   empty. */
#pragma warning(error : 4026)

/* error if a function's parameter list is declared empty but declared/called 
   with a non-empty list. */
#pragma warning(error : 4027 4087)

/* error if a function parameter's type is incorrect */
#pragma warning(error : 4028)

/* error if function's parameter types differ between declaration and
   definition */
#pragma warning(error : 4029)

/* error if function declared twice with different parameter lists */
#pragma warning(error : 4030 4031)

/* error if function has return type but no value returned */
#pragma warning(error : 4033)

/* error if array bounds overflow in array initializer */
#pragma warning(error : 4045)

/* error if function declared void but trying to return a value. */
#pragma warning(error : 4098)

/* error if assigning pointers to functions with mismatched parameter lists */
#pragma warning(error : 4113)

/* error if floating point function doesn't return value */
#pragma warning(error : 4137)

/* error if using nameless struct/union */
#pragma warning(error : 4201)

#ifdef NDEBUG
// disable those silly MSVC warnings about all the values in an enum
// not being used in a switch statement.
#pragma warning(disable : 4061)
#pragma warning(disable : 4062)
#endif

#ifdef __cplusplus
/* disable warnings about long symbol names */
#pragma warning(disable : 4786 4503)
#endif
