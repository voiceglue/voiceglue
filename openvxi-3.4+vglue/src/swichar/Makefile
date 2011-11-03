#/****************License************************************************
# * Vocalocity OpenVXI
# * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
# * This program is free software; you can redistribute it and/or
# * modify it under the terms of the GNU General Public License
# * as published by the Free Software Foundation; either version 2
# * of the License, or (at your option) any later version.
# *  
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
# * registered trademarks of Vocalocity, Inc. 
# * OpenVXI is a trademark of Scansoft, Inc. and used under license 
# * by Vocalocity.
# ***********************************************************************/
# 
#**********************************************************************
#
# swichar, Vocalocity character function library
# UNIX make file 
#
#
#**********************************************************************/

#--------------------------------------------
# Required Environment Variables
#--------------------------------------------
SHELL = /bin/sh

ifndef SWISBSDK
$(error The environment variable SWISBSDK is not defined (it should point to your baseline directory))
endif

# By default, set PRODUCT_LIB_PREFIX to SB
ifndef PRODUCT_LIB_PREFIX
PRODUCT_LIB_PREFIX = SB
$(message PRODUCT_LIB_PREFIX not defined. Defaulting to SB)
endif

#--------------------------------------------
# Include the common def's and config logic
#--------------------------------------------
PROJ_ROOT = ..

include ../make/i386-linux/make.defs

#--------------------------------------------
# Project specific settings
#--------------------------------------------
PROJ_CFLAGS = \
  -Iinclude

PROJ_LIBS =

PUBLIC_HEADERS = \
  include/SWIstring.h \
  include/SWIutfconversions.h \
  include/SWIprintf.h \
  include/SWIchar.h

# Mask the exports of shared objects by explicitly listing public functions
EXPLICIT_VER_FILES = 1

# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define shared libaries to be build
# multiple libries seperated by space
LIBS = $(PRODUCT_LIB_PREFIX)char

# Define library sources
$(PRODUCT_LIB_PREFIX)char_SRC = \
              SWIstring.c \
              SWIutfconversions.c \
              SWIprintf.c \
              genchar.c \
              li_string.c

# Define any linked libraries
$(PRODUCT_LIB_PREFIX)char_LDLIBS =

# Define version file
$(PRODUCT_LIB_PREFIX)char_VER = SWIchar.ver

#--------------------------------
# Programs
#--------------------------------
PROGS =

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
