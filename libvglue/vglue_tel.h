//                 -*-C++-*-

//  Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#ifndef VGLUE_TEL_H
#define VGLUE_TEL_H

#ifndef __cplusplus
#error "This is a C++ only header file"
#endif

#include <VXIvalue.h>
#include <VXItel.h>

VXItelStatus voiceglue_get_line_status ();
VXItelResult voiceglue_transfer (std::string dest,
				 std::string from,
				 int type,
				 std::string timeout,
				 std::string &result);

#endif /* include guard VGLUE_TEL_H */
