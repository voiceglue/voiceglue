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
# VXI, OpenSpeech Browser implementation of the VXI interface
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

ifndef XERCESDIR
$(error The environment variable XERCESDIR is not defined)
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
	-DNO_SWIREC \
	-DHAVE_XERCES \
	-I"$(XERCESDIR)/include/xercesc" \
	-I"$(XERCESDIR)/include" \
	-L"$(XERCESDIR)/lib"	

PUBLIC_HEADERS = 

PUBLIC_ERROR_FILES = \
	VXIErrors.xml \
	VXIDiagnostics.xml \
	defaults.xml	

# Mask the exports of shared objects by explicitly listing public functions
EXPLICIT_VER_FILES = 1
 
# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define libaries to be build
LIBS = VXI

# Define library sources
VXI_SRC = \
	SimpleLogger.cpp \
	DocumentModel.cpp \
	md5.c \
	DocumentStorage.cpp \
	DocumentConverter.cpp \
	DocumentParser.cpp \
	VXICacheStream.cpp \
	GrammarManager.cpp \
	PromptManager.cpp \
	PropertyList.cpp \
	Counters.cpp \
	DialogEventCounter.cpp \
	Scripter.cpp \
	AnswerParser.cpp \
	VXI.cpp \
	VXI_api.c \
	AccessControl.cpp \
	TokenList.cpp

# Define any linked libraries
VXI_LDLIBS = \
	-lVXIvalue$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-lxerces-c
	
# Define version file
VXI_VER = VXI.ver

#-------------------------------------
# Programs
#-------------------------------------

# PROGS = ValidateDoc

ValidateDoc_SRC = progs/ValidateDoc.cpp

ValidateDoc_LDLIBS = \
	-lVXI$(CFG_SUFFIX) \
	-lVXIvalue$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)log$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)inet$(CFG_SUFFIX) \
	-lxerces-c

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
