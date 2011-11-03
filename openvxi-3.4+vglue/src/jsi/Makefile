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
# SBjsi, OpenSpeech Browser implementation of the VXIjsi interface
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
SPIDERMONKEY_CFLAGS = \
	-DHAVE_SPIDERMONKEY \
	-DXP_UNIX \
	-DJSI_MUST_DECLARE_VARS \
	-DJSUSESETOPERATIONCALLBACK=$(JSUSESETOPERATIONCALLBACK) \
	-I"$(SPIDERMONKEYDIR)/src" \
	-I"$(SPIDERMONKEYDIR)"
SPIDERMONKEY_CFLAGS += $(JSCFLAGS)

ifeq ("$(CFG)","debug")
SPIDERMONKEYLIB = \
	-L"$(SPIDERMONKEYDIR)/src/Linux_All_DBG.OBJ"
SPIDERMONKEYLIB += $(JSLIBFLAGS)
SPIDERMONKEY_CFLAGS += \
	-L"$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ" \
	-I"$(SPIDERMONKEYDIR)/src/Linux_All_DBG.OBJ"
SPIDERMONKEYLIB_PATH = $(SPIDERMONKEYDIR)/src/Linux_All_DBG.OBJ
else
SPIDERMONKEYLIB = \
	-L"$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ"
SPIDERMONKEYLIB += $(JSLIBFLAGS)
SPIDERMONKEY_CFLAGS += \
	-L"$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ" \
	-I"$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ"
SPIDERMONKEYLIB_PATH = "$(SPIDERMONKEYDIR)/src/Linux_All_OPT.OBJ"
endif	

PROJ_CFLAGS = \
	-DCOMPANY_DOMAIN=L\"com.vocalocity\" \
	-DMODULE_PREFIX=L\"swi:\" \
	-DHAVE_XERCES \
	-I"./" \
	-I"dom" \
	-I"$(XERCESDIR)/include/xercesc" \
	-I"$(XERCESDIR)/include" \
	-L"$(XERCESDIR)/lib" \
	$(SPIDERMONKEY_CFLAGS)

PUBLIC_HEADERS = \
	SBjsi.h

PUBLIC_ERROR_FILES = \
	SBjsiDiagnostics.xml \
	SBjsiErrors.xml

#PROJ_RPATH_LINK = $(SPIDERMONKEYLIB_PATH)

# Mask the exports of shared objects by explicitly listing public functions
EXPLICIT_VER_FILES = 1
 
# Set defaults for the library version and product name burnt into
# shared libraries
LIBVER = 3

# Define libaries to be build
LIBS = $(PRODUCT_LIB_PREFIX)jsi

# Define library sources
$(PRODUCT_LIB_PREFIX)jsi_SRC = \
	SBjsi.cpp \
	SBjsiFuncs.cpp \
	JsiRuntime.cpp \
	JsiContext.cpp \
	SBjsiLogger.cpp \
	dom/JSDOMNode.cpp \
	dom/JSDOMDocument.cpp \
	dom/JSDOMNodeList.cpp \
	dom/JSDOMNamedNodeMap.cpp \
	dom/JSDOMElement.cpp \
	dom/JSDOMCharacterData.cpp \
	dom/JSDOMAttr.cpp \
	dom/JSDOMText.cpp \
	dom/JSDOMComment.cpp \
	dom/JSDOMCDATA.cpp \
	dom/JSDOMEntityReference.cpp \
	dom/JSDOMProcessingInstruction.cpp \
	dom/JSDOMException.cpp \
	dom/JSDOM.cpp

# Define any linked libraries
$(PRODUCT_LIB_PREFIX)jsi_LDLIBS = \
	-lVXIvalue$(CFG_SUFFIX) \
	-l$(PRODUCT_LIB_PREFIX)trd$(CFG_SUFFIX) \
	$(SPIDERMONKEYLIB)

# Define version file
$(PRODUCT_LIB_PREFIX)jsi_VER = SBjsi.ver

#--------------------------------
# Programs
#--------------------------------
PROGS =

#---------------------------------------------
# Include some rules common to all makefiles
#---------------------------------------------

include ../make/i386-linux/make.rules
