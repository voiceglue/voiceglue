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

#include <sstream>

#include <VXIvalue.h>
#include <SWIutfconversions.h>
#include <VXItrd.h>

#include <vglue_tostring.h>

/*!
** Return the std::string (UTF-8) version of a VXIchar *.
** @param value the value
*/
std::string VXIchar_to_Std_String (const VXIchar *value)
{
    //  Watch out for NULL pointer
    if (! value) {return ("");};
    int utf8_len = SWIwcstoutf8len (value);
    if (utf8_len == 0) {return ("");};
    char utf8_buf[utf8_len];
    SWIwcstoutf8 (value, (unsigned char *) utf8_buf, utf8_len);
    std::string result (utf8_buf);
    return result;
};

/*!
** Return the std::string (UTF-8) version of a void *.
** @param value the value
*/
std::string Pointer_to_Std_String (const void *value)
{
    char *utf8_buf = new char [24];
    sprintf (utf8_buf, "%p", value);
    std::string result (utf8_buf);
    delete[] utf8_buf;
    return result;
};

/*!
** Return the void * version of a const char * (UTF-8).
** @param value the C string
*/
void * Std_String_to_Pointer (const char *value)
{
    void *result = NULL;
    sscanf (value, "%p", &result);
    return result;
};

/*!
** Return the void * version of a std::string (UTF-8).
** @param value the string value
*/
void * Std_String_to_Pointer (std::string value)
{
    return Std_String_to_Pointer (value.c_str());
};

/*!
** Return the std::string (UTF-8) version of a VXIValue.
** @param value the value
*/
std::string VXIValue_to_Std_String (const VXIValue *value)
{
    std::ostringstream result;
    if( value == 0 ) return result.str();
    VXIvalueType type = VXIValueGetType(value);

    switch (type)
    {
    case VALUE_INTEGER:
	result << VXIIntegerValue ((const VXIInteger*) value);
	break;
    case VALUE_LONG:
	result << VXILongValue ((const VXILong*) value);
	break;
    case VALUE_ULONG:
	result << VXIULongValue ((const VXIULong*) value);
	break;
    case VALUE_FLOAT:
	result << VXIFloatValue ((const VXIFloat*) value);
	break;
    case VALUE_DOUBLE:
	result << VXIDoubleValue ((const VXIDouble*) value);
	break;
    case VALUE_BOOLEAN:
	if (VXIBooleanValue ((const VXIBoolean*) value) == TRUE)
	{
	    result << "true";
	}
	else
	{
	    result << "false";
	};
	break;
    case VALUE_STRING:
	result << VXIchar_to_Std_String(VXIStringCStr((const VXIString*)value));
	break;
    case VALUE_PTR:
	result << Pointer_to_Std_String (VXIPtrValue ((const VXIPtr *) value));
	break;
    case VALUE_CONTENT:
    {
	const VXIchar *content_type_string;
	VXIbyte *content_ptr;
	VXIulong content_size;
	VXIContentValue ((const VXIContent*) value,
			 &content_type_string,
			 (const VXIbyte **) &content_ptr,
			 &content_size);
	result << VXIchar_to_Std_String (content_type_string)
	       << ":len(" << content_size << "):p"
	       << Pointer_to_Std_String (content_ptr);
    }
    break;
    case VALUE_MAP:
    {
	int items_shown = 0;
	const VXIchar *key = NULL;
	const VXIValue *gvalue = NULL;
	result << "{";
	VXIMapIterator *it =
	    VXIMapGetFirstProperty ((const VXIMap*) value, &key, &gvalue);
	int ret = 0;
	while (ret == 0 && key && gvalue)
	{
	    if (items_shown++)
	    {
		result << " ";
	    };
	    result << VXIchar_to_Std_String (key) << "="
		   << VXIValue_to_Std_String (gvalue);
	    ret = VXIMapGetNextProperty(it, &key, &gvalue);
	}
	VXIMapIteratorDestroy(&it);
	result << "}";
    }   
    break;
    case VALUE_VECTOR:
    {
	int items_shown = 0;
	VXIunsigned vlen = VXIVectorLength((const VXIVector*)value);
	result << "[";
	for (VXIunsigned i = 0; i < vlen; ++i)
	{
	    if (items_shown++)
	    {
		result << " ";
	    };
	    const VXIValue *gvalue =
		VXIVectorGetElement ((const VXIVector*) value, i);
	    result << i << "="
		   << VXIValue_to_Std_String (gvalue);
	}
	result << "]";
    }
    break;
    default:
	result << "<UNKNOWN>";
    }          

    return result.str();
}

/*!
** Returns the property in the map indexed by the key
** @param map the VXIMap in which to find the key
** @param key the key to look up, in UTF8 form
** @return The std::string UTF8 version of the value, or "" if none found
*/
std::string VXIMap_Property_to_Std_String (const VXIMap *map, const char *key)
{
    vxistring keystring = Std_String_to_vxistring (key);
    const VXIchar *vxikey = keystring.c_str();
    const VXIValue *gvalue = VXIMapGetProperty (map, vxikey);
    if (gvalue == NULL)
    {
	return ("");
    };
    return (VXIValue_to_Std_String (gvalue));
};

/*!
** Returns the property in the map indexed by the key
** @param map the VXIMap in which to find the key
** @param key the key to look up, in UTF8 form
** @return The std::string UTF8 version of the value, or "" if none found
*/
std::string VXIMap_Property_to_Std_String (const VXIMap *map, std::string key)
{
    return VXIMap_Property_to_Std_String (map, key.c_str());
}

/*!
** Converts a std::string to a vxistring, which is a wide-character version.
** @param in_string the string to convert
*/
vxistring Std_String_to_vxistring (const char *in_string)
{
    int wide_length = SWIutf8towcslen ((unsigned char *) in_string);
    if (wide_length == 0) {return (L"");};
    wchar_t wide_buf[wide_length];
    SWIutf8towcs ((unsigned char *) in_string, wide_buf, wide_length);
    vxistring result (wide_buf);
    return result;
};

/*!
** Converts a std::string to a vxistring, which is a wide-character version.
** @param in_string the string to convert
*/
vxistring Std_String_to_vxistring (std::string in_string)
{
    return (Std_String_to_vxistring (in_string.c_str()));
};

std::vector<std::string> &split (const std::string &s,
				 char delim,
				 std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split (const std::string &s, char delim)
{
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

std::vector<std::string> split (const std::string &s)
{
    return split (s, ' ');
}
