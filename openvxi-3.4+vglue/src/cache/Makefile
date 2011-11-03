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
#**********************************************************************
#
# SBcache, OpenSpeech Browser implementation of the VXIcache interface
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

ifndef SPIDERMONKEYDIR
$(error The environment variable SPIDERMONKEYDIR is not defined)
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
	-DMODULE_PREFIX=L\"swi:\"

PUBLIC_HEADERS = \
	SBcache.h

PUBLIC_ERROR_FILES = \
	SBcacheErrors.xml \
	SBcacheDiagnostics.xml

# Mask the exports of shared objects by explicitly listing public functions
EXPLICIT_VER_FILES = 1
 
# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define libaries to be build
LIBS = $(PRODUCT_LIB_PREFIX)cache

# Define library sources
$(PRODUCT_LIB_PREFIX)cache_SRC = \
	md5.cpp \
	base64.cpp \
	SBcacheMisc.cpp \
	SBcacheEntry.cpp \
	SBcacheManager.cpp \
	SBcache.cpp \
	SBinetLogger.cpp

# Define any linked libraries
$(PRODUCT_LIB_PREFIX)cache_LDLIBS = \
	-lVXIvalue$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trdUtil$(CFG_SUFFIX)
	
# Define version file
$(PRODUCT_LIB_PREFIX)cache_VER = SBcache.ver

#--------------------------------
# Programs
#--------------------------------
PROGS =

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
