
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

#ifndef SWIstream_H
#define SWIstream_H

#include "SWIutilHeaderPrefix.h"

class SWIUTIL_API_CLASS SWIstream
{
 public:
  enum Result
  {
    SUCCESS = 0,
    END_OF_FILE = -1,
    INVALID_ARGUMENT = -2,
    ILLEGAL_STATE = -3,
    READ_ERROR = -4,
    WRITE_ERROR = -5,
    OPEN_ERROR = -6,
    NOT_SUPPORTED = -7,
    PARSE_ERROR = -8,
    WOULD_BLOCK = -9,
    TIMED_OUT = -10,
    BUFFER_OVERFLOW = -11,
    FAILURE = -12,
    CONNECTION_ABORTED = -13,
    CONNECTION_RESET = -14
  };
};

#endif
