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

#ifndef VGLUE_TOSTRING_C_H
#define VGLUE_TOSTRING_C_H

#include <VXIvalue.h>

#ifdef __cplusplus
extern "C" {
#endif

char *voiceglue_c_vxivalue_to_utf8string (const VXIValue *value);

#ifdef __cplusplus
}
#endif

#endif /* include guard VGLUE_TOSTRING_C_H */
