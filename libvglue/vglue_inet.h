//                 -*-C++-*-

//  Copyright 2010 Ampersand Inc., Doug Campbell
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

#ifndef VGLUE_INET_H
#define VGLUE_INET_H

#ifndef __cplusplus
#error "This is a C++ only header file"
#endif

#include <VXIvalue.h>
#include <VXIprompt.h>

void voiceglue_http_get (const VXIchar *method,
			 const VXIchar *url,
			 const VXIMap *postdata_map,
			 int parsevxml,
			 std::string &path,
			 std::string &content_type,
			 std::string &parse_tree_addr);

#endif /* include guard VGLUE_INET_H */
