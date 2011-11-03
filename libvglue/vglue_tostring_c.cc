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

#include <vglue_tostring_c.h>
#include <vglue_tostring.h>
#include <stdlib.h>
#include <string.h>

/*  voiceglue tostring in C routines  */

/*
** Return a malloc'd (must be free'd by caller) null-terminated string
** representing the tostring version of value
** @param value the value to represent
*/
char *voiceglue_c_vxivalue_to_utf8string (const VXIValue *value)
{
    std::string result_std_string = VXIValue_to_Std_String (value);
    size_t stringlen = result_std_string.size() + 1;
    char *result = (char *) malloc(stringlen);
    if (result == NULL) {return result;};
    strncpy (result, result_std_string.c_str(), stringlen);
    return result;
};
