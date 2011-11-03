//                  -*-C++-*-

//  Copyright 2006,2007,2010 Ampersand Inc., Doug Campbell
//
//  This file is part of libvglue.
//
//  libvglue is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  libvglue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with libvglue; if not, see <http://www.gnu.org/licenses/>.

#ifndef VGLUE_TOSTRING_H
#define VGLUE_TOSTRING_H

#ifndef __cplusplus
#error "This is a C++ only header file"
#endif

#include <VXIvalue.h>
#include <list>
#include <string>
#include <vector>
typedef std::basic_string<VXIchar> vxistring;

std::string VXIchar_to_Std_String (const VXIchar *const value);
std::string Pointer_to_Std_String (const void *value);
void * Std_String_to_Pointer (std::string value);
std::string VXIValue_to_Std_String (const VXIValue *value);
std::string VXIMap_Property_to_Std_String (const VXIMap *map, const char *key);
std::string VXIMap_Property_to_Std_String (const VXIMap *map, std::string key);
vxistring Std_String_to_vxistring (const char *in_string);
vxistring Std_String_to_vxistring (std::string in_string);
std::vector<std::string> &split
(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split (const std::string &s, char delim);
std::vector<std::string> split (const std::string &s);

#endif /* include guard VGLUE_TOSTRING_H */
