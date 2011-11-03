#
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
  -DCOMPANY_DOMAIN=L\"com.vocalocity\" \
  -DMODULE_PREFIX=L\"swi:\" \
	-DVXITRD_RECURSIVE_MUTEX \
	-DVXITRD_KERNEL_MUTEX

PROJ_LIBS =
PUBLIC_HEADERS = \
	trdUtil/SBtrdMutex.hpp \
	trdUtil/SBtrdRefCount.hpp \
	trdUtil/SBtrdEvent.hpp \
	trdUtil/SBtrdTimeOfDay.hpp \

# Mask the exports of shared objects by explicitly listing public functions
EXPLICIT_VER_FILES = 1

# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define libaries to be build
STATIC_LIBS = $(PRODUCT_LIB_PREFIX)trdUtil
LIBS = $(PRODUCT_LIB_PREFIX)trd

# Define library sources
$(PRODUCT_LIB_PREFIX)trd_SRC = \
	SBtrdPthreads.cpp

$(PRODUCT_LIB_PREFIX)trdUtil_SRC = \
	trdUtil/SBtrdMutex.cpp \
	trdUtil/SBtrdRefCount.cpp \
	trdUtil/SBtrdEvent.cpp \
	trdUtil/SBtrdTimeOfDay.cpp

# Define any linked libraries
$(PRODUCT_LIB_PREFIX)trd_LDLIBS =

# Define version file
$(PRODUCT_LIB_PREFIX)trd_VER = SBtrdUnix.ver

#--------------------------------
# Programs
#--------------------------------
PROGS =

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
