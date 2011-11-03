//  -*- C++ -*-

/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#ifndef _VXIVXIMAIN_H
#define _VXIVXIMAIN_H 1

#include <VXIvalue.h>

#ifdef __cplusplus
extern "C" {
#endif

int voiceglue_parse_config_file (VXIMap**, const char *);
int voiceglue_init_platform (VXIMap*, VXIunsigned *);
int voiceglue_free_vxmldocument (const VXIchar *addr);
int vxiStartOneThread (void *, int, int, char *,
		       char *, char*, char *, void **);
int vxiFinishThread (void *);
int vxiStopPlatform (void *);
void vxiSetTrace (int);
void vxiFreeVXMLDocument (void *);

int vximain (int, char **);

#ifdef __cplusplus
}
#endif

#endif  /* VXIvximain.h */
