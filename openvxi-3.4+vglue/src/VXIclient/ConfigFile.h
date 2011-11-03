
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

#ifndef _CONFIGFILE_H
#define _CONFIGFILE_H

#include "VXIplatform.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constants for configuration file parsing
 */
#define CONFIG_FILE_SEPARATORS " \t\r\n"
#define CONFIG_FILE_ENV_VAR_BEGIN_ID "$("
#define CONFIG_FILE_ENV_VAR_END_ID ")"
#define CONFIG_FILE_COMMENT_ID '#'

/**
 * Parse a SBclient configuration file
 *
 * @param configArgs     Properties read from the configuration file are
 *                       stored in this array using the corresponding keys
 * @param fileName       Name and path of the configuration file to be
 *                       parsed. If NULL, the default configuration file
 *                       name defined above will be used.
 *
 * @result VXIplatformResult 0 on success
 */
VXIplatformResult ParseConfigFile (VXIMap **configArgs, const char *fileName);

/**
 * Parse a SBclient configuration line
 *
 * @param buffer         The configuration file line to be parsed, is modified
 *                       during the parse
 * @param configArgs     The property read from the configuration line is
 *                       stored in this array using the corresponding key
 *
 * @result VXIplatformResult 0 on success
 */
VXIplatformResult ParseConfigLine(char* buffer, VXIMap *configArgs);

#ifdef __cplusplus
}
#endif

#endif
