//  Copyright 2006,2007 Ampersand Inc., Doug Campbell
//
//  This file is part of libvglue.
//
//  libvglue is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  libvglue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with libvglue; if not, see <http://www.gnu.org/licenses/>.

#include "stdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

#include "vglue_tostring.h"
#include "vglue_ipc.h"
#include "vglue_ipc_c.h"
#include "vglue_run.h"

int main (int argc, char *argv[])
{
    //  First try a simple string
    VXIString *theStringObj = VXIStringCreate (L"A test string");
    std::string the_std_string =
	VXIValue_to_Std_String ((const VXIValue *) theStringObj);
    printf ("Result of translation: %s\n", the_std_string.c_str());
    //  And convert it back
    vxistring wide_guy = Std_String_to_vxistring (the_std_string);
    printf ("Result of back-translation: %ls\n",
	    wide_guy.c_str());
    
    //  Now try a whole map
    VXIBoolean *a_false_boolean = VXIBooleanCreate (FALSE);
    VXIBoolean *a_true_boolean = VXIBooleanCreate (TRUE);
    VXIInteger *an_integer = VXIIntegerCreate (-123);
    VXILong *a_long = VXILongCreate (-1234);
    VXIULong *a_ulong = VXIULongCreate (1234);
    VXIFloat *a_float = VXIFloatCreate (1.23);
    VXIDouble *a_double = VXIDoubleCreate (1.234);
    VXIPtr *a_ptr = VXIPtrCreate (NULL);
    VXIMap *a_map = VXIMapCreate();
    VXIVector *a_vector = VXIVectorCreate();
    VXIVectorAddElement (a_vector, (VXIValue *) an_integer);
    VXIVectorAddElement (a_vector, (VXIValue *) a_long);
    VXIVectorAddElement (a_vector, (VXIValue *) a_ulong);
    VXIMapSetProperty (a_map, L"false", (VXIValue *) a_false_boolean);
    VXIMapSetProperty (a_map, L"true", (VXIValue *) a_true_boolean);
    VXIMapSetProperty (a_map, L"vector", (VXIValue *) a_vector);
    VXIMapSetProperty (a_map, L"1.23", (VXIValue *) a_float);
    VXIMapSetProperty (a_map, L"1.234", (VXIValue *) a_double);
    VXIMapSetProperty (a_map, L"0P", (VXIValue *) a_ptr);
    VXIMapSetProperty (a_map, L"test string key", (VXIValue *) theStringObj);
    the_std_string =
	VXIValue_to_Std_String ((const VXIValue *) a_map);
    printf ("Map: %s\n", the_std_string.c_str());

    //  Try a map look-up
    the_std_string = VXIMap_Property_to_Std_String (a_map, "test string key");
    printf ("Map lookup: %s\n", the_std_string.c_str());

    //  Try to initialize the VXI platform
    int r;
    void *platform_handle;
    int pipes[2];
    int flags;
    char cbuf[1024];
    if ((r = pipe (pipes)) != 0)
    {
	printf ("pipe() failed with return code %d\n", r);
	exit (1);
    };
    flags = fcntl (pipes[0], F_GETFL);
    flags |= O_NONBLOCK;
    r = fcntl (pipes[0], F_SETFL, flags);
    if (r != 0)
    {
	printf ("fcntl() failed with return code %d\n", r);
	exit (1);
    };
    r = voiceglue_start_platform (1024, pipes[1], 7, &platform_handle);
    if (r != 0)
    {
	printf ("voiceglue_start_platform() failed with return code %d\n", r);
	exit (1);
    };
    printf ("Output from VXI-voiceglue\n");
    printf ("-------------------------\n");
    while ((r = read (pipes[0], cbuf, 1023)) > 0)
    {
	cbuf[r] = '\0';
	printf ("%s", cbuf);
    };
    printf ("-------------------------\n");
    /*
    r = voiceglue_stop_platform (platform_handle);
    if (r != 0)
    {
	printf ("voiceglue_stop_platform() failed with return code %d\n", r);
	exit (1);
    };
    printf ("Output from VXI-voiceglue\n");
    printf ("-------------------------\n");
    while ((r = read (pipes[0], cbuf, 1023)) > 0)
    {
	cbuf[r] = '\0';
	printf ("%s", cbuf);
    };
    printf ("-------------------------\n");
    */
};
