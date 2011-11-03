#
#/****************License************************************************
# * Vocalocity OpenVXI
# * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
# * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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
ifeq ("$(CFG)","debug")
SPIDERMONKEYLIB_PATH = "$(SPIDERMONKEYDIR)/src/Linux_All_DBG.OBJ"
else
SPIDERMONKEYLIB_PATH = "$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ"
endif	

PROJ_CFLAGS = \
	-DCOMPANY_DOMAIN=L\"com.vocalocity\" \
	-DMODULE_PREFIX=L\"swi:\" \
	-DHAVE_XERCES \
	-I"$(XERCESDIR)/include/xercesc" \
	-I"$(XERCESDIR)/include"
PORJ_CFLAGS += $(JSCFLAGS)
PROJ_CFLAGS +=	-I"include" \
	-L"$(XERCESDIR)/lib" \
	-L$(SPIDERMONKEYLIB_PATH)

# PUBLIC_HEADERS = 
# PUBLIC_HEADERS = \
# 	SBlogListeners.h \
# 	VXIclient.h \
# 	VXIplatform.h \
# 	VXIclientUtils.h \
# 	VXIobjectAPI.h \
# 	VXIclientConfig.h \
# 	include/VXIpromptAPI.h \
# 	include/VXIrecAPI.h \
# 	include/VXItelAPI.h \
# 	ConfigFile.h
PUBLIC_HEADERS = \
	VXIvximain.h

PUBLIC_ERROR_FILES = \
	rec/VXIrecErrors.xml \
	rec/VXIrecDiagnostics.xml \
	prompt/VXIpromptErrors.xml \
	prompt/VXIpromptDiagnostics.xml \
	tel/VXItelErrors.xml \
	tel/VXItelDiagnostics.xml \
	VXIobjectErrors.xml \
	VXIobjectDiagnostics.xml \
	VXIclientErrors.xml \
	VXIclientDiagnostics.xml \
	SBclient.cfg

#PROJ_RPATH_LINK = $(SPIDERMONKEYLIB_PATH)

# Suppress using a *.def file to define exports from DLLs, use the
# SYMBOL_EXPORT_DECL definition from VXIheaderPrefix.h instead
NO_DEF_FILES = 1
 
# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define libaries to be built
# soup start
# LIBS = VXIrec VXItel VXIprompt
LIBS = VXIrec VXItel VXIprompt VXIclient

VXIclient_SRC = \
	VXIvximain.c \
	VXIclient.c \
	VXIclientUtils.c \
	ConfigFile.cpp \
	VXIobject.cpp \
	SBlogListeners.cpp \
	SBlogOSUtils.cpp

VXIclient_LDLIBS = \
	-l$(PRODUCT_LIB_PREFIX)jsi$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)char$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)log$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)logMapper$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)cache$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)inet$(CFG_SUFFIX) \
	-lVXIvalue$(CFG_SUFFIX) \
	-lVXI$(CFG_SUFFIX)
VXIclient_LDLIBS += $(JSLIBFLAGS)

# Define version file
VXIclient_VER = VXIclient.ver

# soup end

VXIrec_SRC = \
	rec/VXIrec.cpp \
	rec/VXIrec_utils.cpp

VXIrec_LDLIBS = \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)char$(CFG_SUFFIX) \
	-lVXIvalue$(CFG_SUFFIX) \
	-l$(XERCES_LIB_PREFIX)xerces-c

# Define version file
VXIrec_VER = rec/VXIrec.ver

VXIprompt_SRC = \
  prompt/VXIprompt.cpp

VXIprompt_LDLIBS = \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)char$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)cache$(CFG_SUFFIX) \
	-lVXIvalue$(CFG_SUFFIX)

# Define version file
VXIprompt_VER = prompt/VXIprompt.ver

VXItel_SRC = \
  tel/VXItel.cpp

VXItel_LIBS = \
  -l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
  -l$(PRODUCT_LIB_PREFIX)char$(CFG_SUFFIX) \
  -lVXIvalue$(CFG_SUFFIX)

# Define version file
VXItel_VER = tel/VXItel.ver

#-------------------------------------
# Programs
#-------------------------------------

# PROGS = RunVXI

RunVXI_SRC = \
	VXImain.c \
	VXIclient.c \
	VXIclientUtils.c \
	ConfigFile.cpp \
	VXIobject.cpp \
	SBlogListeners.cpp \
	SBlogOSUtils.cpp

RunVXI_LDLIBS = \
	-l$(PRODUCT_LIB_PREFIX)jsi$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)char$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)log$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)logMapper$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)cache$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)inet$(CFG_SUFFIX) \
	-lVXIvalue$(CFG_SUFFIX) \
	-lVXI$(CFG_SUFFIX)
RunVXI_LDLIBS += $(JSLIBFLAGS)

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
