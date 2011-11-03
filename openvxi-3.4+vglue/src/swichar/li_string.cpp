/* li_string, locale independant conversions */

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

/* -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <wctype.h>
#include <wchar.h>
#include <string.h>
#include <float.h>
#include "SWIstring.h"

const int    MAX_PRECISION = 6;
const int    precision_multiplier = 1000000; /* 10^6 */

const double VIRTUAL_ZERO = pow(10, -MAX_PRECISION);
const double TOO_BIG      = pow(10, MAX_PRECISION) - 1;

/*=========================================================================
** SWIatof(string) -- Converts character string to double
**
** Arguments:
**           string -- string to be converted to double 
**
** Returns:  double representation of value in wstring
**
** Description:
**          Ignores leading whitespace in string of this form:
**          [(+|-)[0123456789]+(.[0123456789])]
**          charcater.  Converts the integer parts and the
**          decimal part separately, adding them together at the
**          end and applying the sign
** 
**=========================================================================
*/
double SWIatof(const char *str)
{
   double result = 0.0;
   int negative = 0;
   char *cp = NULL;

   // NOTE:  unicode values for white space, '+', and '-'
   // are unambiguous

   // Skip over whitespace
   while (SWIchar_isspace(*str) )
     str++;

   // Handle sign
   if (*str == '+')
     *str++;
   else if (*str == '-')
   {
     negative = 1;
     *str++;
   }

   // NOTE: This is very clever code. It intentionally calls the
   // locale dependant C library string to double function, strtod( ),
   // in an attempt for very efficient conversion with locale
   // dependant fraction delimiters (, for some locales)
   // supported. But then it goes on to ensure the SWI cannonical
   // fraction delimiter (.) is supported as well by doing a secondary
   // conversion where required.

   result = strtod(str, &cp);   // integer part
   if ( (cp != 0) && (*cp == '.'))  // has fraction?
   {
     cp++;  // skip over radix point
     int decDigits = strspn(cp, "0123456789");
     if (decDigits > 0) /* avoid divide by zero */
       result += strtod(cp, &cp) / pow(10, decDigits); // add fraction
   }

   // Apply the sign 
   if (negative)
      result = -result;

   return(result);
}

/*=======================================================================
SWIatofloat -- as above but converts a string to a float
**=======================================================================
*/
float SWIatofloat(const char *str)
{
  double d = SWIatof(str);
  return( (float)d );
}

/*========================================================================
** SWIdtowcs -- converts floating point number to a string
**
** Arguments:
**              double toConvert -- number to be converted
**              wchar_t* result  -- container for wide char representation
**              int len          -- size in characters of output buffer
**
** Returns:     void
**
** Description:
**              Notes the sign of the number, using only its absolute
**              value.  Converts the integer and fractional parts
**              separately, combining them later, stripping extraneous
**              trailing zeros, and applying the sign character.
**
** NOTE:        The conversion fails if the number of digits in the
**              integer part exceeds MAX_PRECISION - 1 (there must
**              be at least a single digit in the fraction.)
**
**========================================================================
*/
SWIcharResult SWIdtowcs(const double toConvert, 
                             wchar_t *result,
                             int len)
{
  SWIcharResult rc;
  int integerPart = (int) toConvert;
  int slen, fractionPart;
  double fraction = 0.0;

  fractionPart = (int) ( (toConvert-integerPart)*((double)precision_multiplier)+0.5 );
  if ( fractionPart ) {
    fraction = fractionPart/(double) precision_multiplier;
    int overflow = (int) fraction;
    if ( overflow ) {
      integerPart += overflow;
      fraction -= overflow;
    }
  }

  rc = SWIitowcs(integerPart, result, len);
  if ( rc != SWIchar_SUCCESS ) return rc;
 
  slen = wcslen(result);
  if ( slen+2+MAX_PRECISION > len ) 
    return SWIchar_BUFFER_OVERFLOW;

  if ( fractionPart ) {
    int i;
    wchar_t *p = result+slen;
    *p++ = '.';
    for ( i = 0; i < MAX_PRECISION; ++i ) {
      int nextDigit;
      fraction *= 10;
      nextDigit = (int) (fraction+0.0000000001);
      *p++ = (char)nextDigit+'0';
      fraction -= nextDigit;
    }
    *p = 0;

    // Remove extraneous zeros
    while (  (p[-1] == L'0') && ( p[-2] != L'.'))
      *--p = 0;
  }
  return SWIchar_SUCCESS;
}

#if 0
// old code!!!
{
  wchar_t integerString[SWIchar_MAXSTRLEN];
  wchar_t fractionString[SWIchar_MAXSTRLEN] = {L'.', L'0'};
  double d = toConvert;
  int integerPart = (int)d;  // overflow caught by TOO_BIG check below.
 
  SWIcharResult status = (d > TOO_BIG) ? SWIchar_BUFFER_OVERFLOW : SWIitowcs(integerPart, integerString, MAX_PRECISION-2);
  if (status == SWIchar_SUCCESS)
  {
    d -= integerPart;  // fraction remaining
    if (d < 0)         // fraction should be positive 
      d = -d;

    if ( (d > 0.0) && (d > VIRTUAL_ZERO) ) // has fraction?
    {
      int decimalPlaces = MAX_PRECISION - wcslen(integerString);
      if (decimalPlaces >= 1)  // least one digit after the decimal
      {
        wchar_t* fp = &fractionString[1];  // after decimal.
        while (d < 0.1)  // account for leading zeros.
        {
          *fp++ = L'0';
          d *= 10.0;
          decimalPlaces--;
        }
        int integerFract = (int)((d * pow(10, decimalPlaces)) + 0.5);
        status = SWIitowcs(integerFract, fp, SWIchar_MAXSTRLEN);
        if (status == SWIchar_SUCCESS)
        {
          // Remove extraneous zeros
          wchar_t* wp = fractionString + wcslen(fractionString) - 1;
          while (  (*wp == L'0') && ( *(wp-1) != L'.'))
            *wp-- = L'\0';
        }
      }
      else // too small for handle fraction
        status = SWIchar_BUFFER_OVERFLOW;
    }

    // Combine the integer and fractional parts
    if (status == SWIchar_SUCCESS)
    {
      if (wcslen(integerString)+wcslen(fractionString)+1 <= (unsigned int)len)
      {
        wcscpy(result, integerString);
        wcscat(result, fractionString);
      }
      else
        status = SWIchar_BUFFER_OVERFLOW;
    }
  }

  return(status);
}
#endif

/*=========================================================================
** SWIdtostr -- as above but converts from double to character string
**=========================================================================
*/
SWIcharResult SWIdtostr(const double toConvert, char *str, int len)
{
  wchar_t wideArray[SWIchar_MAXSTRLEN];
  SWIcharResult status = SWIdtowcs(toConvert, wideArray, len);
  if (status == SWIchar_SUCCESS)
  {
    const wchar_t* wp = wideArray;
    status = SWIwcstostr(wp, str, len); 
  }

  return(status);
}
