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

#ifndef VGLUE_PROMPT_H
#define VGLUE_PROMPT_H

#ifndef __cplusplus
#error "This is a C++ only header file"
#endif

#include <VXIvalue.h>
#include <VXIprompt.h>

VXIpromptResult voiceglue_prompt (const VXIchar *prompt_spec,
				  const VXIMap *properties);

#endif /* include guard VGLUE_PROMPT_H */
