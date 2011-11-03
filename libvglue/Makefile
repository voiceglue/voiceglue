##  Copyright 2006,2007,2010 Ampersand Inc., Doug Campbell
##
##  This file is part of libvglue.
##
##  libvglue is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  libvglue is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with libvglue; if not, see <http://www.gnu.org/licenses/>.

SHELL = /bin/sh
RM = rm -f
INSTALL = install

.SUFFIXES:
.SUFFIXES: .c .o .so .cc .cpp .h .hpp

CXXFLAGS = -g
ALL_CXXFLAGS = -I. -L. -fPIC $(CXXFLAGS)

INCS = vglue_tostring.h vglue_tostring_c.h vglue_ipc.h vglue_ipc_c.h vglue_run.h vglue_rec.h vglue_tel.h vglue_prompt.h vglue_inet.h

OBJS = vglue_tostring.o vglue_tostring_c.o vglue_ipc.o vglue_ipc_c.o vglue_run.o vglue_rec.o vglue_tel.o vglue_prompt.o vglue_inet.o

LINKLIBS = -lSBtrdD -lSBcharD -lVXIvalueD -lVXIclientD -lstdc++ -lpthread -ldl
LINKLIBS += $(JSLIBFLAGS)

all: vglue_test libvglue.so

$(OBJS): %.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(ALL_CXXFLAGS) $< -o $@

`vglue_test.o : vglue_test.cc vglue_tostring.h

libvglue.so : $(OBJS)
	$(CXX) $(CPPFLAGS) -shared -Wl,-soname,libvglue.so $(ALL_CXXFLAGS) $(OBJS) -o $@ $(LINKLIBS)

vglue_test : libvglue.so vglue_test.o $(OJBS) $(INCS)
	$(CXX) $(CPPFLAGS) $(ALL_CXXFLAGS) -Wl,-rpath=$(shell pwd): vglue_test.o -o $@  -lvglue $(LINKLIBS)

install : libvglue.so
	$(INSTALL) $(INCS) /usr/include
	$(INSTALL) libvglue.so /usr/lib
	( if [ -d /usr/lib64 ] ; then cp libvglue.so /usr/lib64 ; fi )

install-headers:
	$(INSTALL) $(INCS) /usr/include

uninstall:
	( cd /usr/include; $(RM) $(INCS) )
	$(RM) /usr/lib/libvglue.so

clean:
	$(RM) *.o *.so vglue_test
