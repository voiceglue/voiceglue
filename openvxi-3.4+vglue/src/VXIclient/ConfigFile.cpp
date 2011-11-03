
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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include "stdio.h"
#include <iostream>
using namespace std;
#include <string.h>
#include <math.h>
#include <cstdlib>

#include "ConfigFile.h"           // Header for these functions
#include "VXIclientUtils.h"       
#include "VXIclient.h"            // For VXIclientError( )

#define READ_BUFFER_SIZE 2048
#define MODULE_NAME L"VXIClient"

/* Do not use atof( ), is not OS locale safe */
static double StringToFloat(const char *str)
{
   double result = 0.0;
   char *cp = NULL;
   result = strtod(str, &cp);   // integer part
   if ((cp != 0) && (*cp == '.'))  // has fraction?
   {
     cp++;  // skip over radix point
     int decDigits = strspn(cp, "0123456789");
     if (decDigits > 0) /* avoid divide by zero */
#ifdef _solaris_
       result += strtod(cp, &cp) / pow(10.0f, decDigits); // add fraction
#else
#if defined(__GNUC__) && ((__GNUC__ >= 3) && (__GLIBCPP__ >= 20030205))
       result += strtod(cp, &cp) / pow(10.0, double(decDigits)); // add fraction
#else 
       result += strtod(cp, &cp) / pow(10, decDigits); // add fraction
#endif
#endif
   }

   return result;
}

extern "C"
VXIplatformResult ParseConfigLine(char* buffer, VXIMap *configArgs)
{
  char *end;
  const char *configKey, *configType, *configValue;

  /* Skip blank lines or those starting with comments */
  if((buffer[0] == '\0') || (buffer[0] == CONFIG_FILE_COMMENT_ID) ||
     (strspn(buffer, CONFIG_FILE_SEPARATORS) == strlen(buffer))) {
    return VXIplatform_RESULT_SUCCESS;
  }

  /* Get the configuration key */
  configKey = buffer;
  if(strchr(CONFIG_FILE_SEPARATORS, configKey[0]) != NULL) {
     return VXIplatform_RESULT_FAILURE;
  }
  end = (char *) strpbrk(configKey, CONFIG_FILE_SEPARATORS);
  if(end == NULL) {
    return VXIplatform_RESULT_FAILURE;
  }
  *end = '\0'; /* terminate configKey */

  /* Get the configuration type */
  configType = end + 1;
  configType = &configType[strspn(configType, CONFIG_FILE_SEPARATORS)];
  if(configType[0] == '\0') {
    return VXIplatform_RESULT_FAILURE;
  }
  end = (char *) strpbrk(configType, CONFIG_FILE_SEPARATORS);
  if(end == NULL) {
    return VXIplatform_RESULT_FAILURE;
  }
  *end = '\0'; /* terminate configType */

  /* Get the configuration value, stripping trailing whitespace */
  configValue = end + 1;
  configValue = &configValue[strspn(configValue, CONFIG_FILE_SEPARATORS)];
  if(configValue[0] == '\0') {
    return VXIplatform_RESULT_FAILURE;
  }
  end = (char *) strchr(configValue, '\0');
  while((end > configValue) &&
        (strchr(CONFIG_FILE_SEPARATORS, *(end - 1)) != NULL)) end--;
  if(end == configValue) {
    return VXIplatform_RESULT_FAILURE;
  }
  *end = '\0'; /* terminate configValue */

  string expStr("");
  if(configValue != NULL) {
    // Resolve environment variables, if any
    const char *envVarBgn, *envVarEnd;
    const VXIint lenBgn = strlen(CONFIG_FILE_ENV_VAR_BEGIN_ID);
    const VXIint lenEnd = strlen(CONFIG_FILE_ENV_VAR_END_ID);
    const char *ptrValue = configValue;

    while((envVarBgn = strstr(ptrValue, CONFIG_FILE_ENV_VAR_BEGIN_ID)) != NULL)
    {
      if (envVarBgn != ptrValue)
        expStr.assign (ptrValue, envVarBgn - ptrValue);
      envVarBgn += lenBgn;
      if(envVarBgn[0] == 0)
        return VXIplatform_RESULT_FAILURE;

      envVarEnd = strstr(envVarBgn, CONFIG_FILE_ENV_VAR_END_ID);
      if((envVarEnd == NULL) || (envVarEnd == envVarBgn))
        return VXIplatform_RESULT_FAILURE;

      string envStr(envVarBgn, envVarEnd - envVarBgn);
      const char *envVarValue = getenv(envStr.c_str());
      if((envVarValue == NULL) || (envVarValue[0] == 0)) {
        VXIclientError (NULL, MODULE_NAME, 110,
                        L"Configuration file references an environment "
                        L"variable that is not set",
                        L"%s%S", L"envVar", envStr.c_str( ));
        return VXIplatform_RESULT_FAILURE;
      }

      expStr += envVarValue;
      ptrValue = envVarEnd + lenEnd;
    }

    expStr += ptrValue;
    configValue = expStr.c_str();
  }

  VXIint keyLen = strlen(configKey) + 1;
  VXIchar *wConfigKey = new VXIchar [keyLen];
  if(wConfigKey == NULL) {
    return VXIplatform_RESULT_OUT_OF_MEMORY;
  }
  char2wchar(wConfigKey, configKey, keyLen);

  if(strcmp(configType, "Environment") == 0) { // Set environment variable
    int cfgValLen = configValue ? strlen(configValue) : 0;
    char *envVar = new char [strlen(configKey) + cfgValLen + 2];
    if(envVar == NULL) {
      delete [] wConfigKey;
      return VXIplatform_RESULT_OUT_OF_MEMORY;
    }
#ifdef WIN32
    sprintf(envVar, "%s=%s", configKey, configValue ? configValue : "");
    _putenv(envVar);
#else
    if(configValue)
      sprintf(envVar, "%s=%s", configKey, configValue);
    else
      strcpy(envVar, configKey); // Remove variable from environment
    putenv(envVar);
#endif
    // 'envVar' must NOT be freed !!!
  }
  else if(strcmp(configType, "VXIPtr") == 0) {
    // Only NULL pointers are supported
    VXIMapSetProperty(configArgs, wConfigKey,
                     (VXIValue *)VXIPtrCreate(NULL));
  }
  else {
    if(configValue == NULL) {
      delete [] wConfigKey;
      return VXIplatform_RESULT_FAILURE;
    }

    if(strcmp(configType, "VXIInteger") == 0) {
      VXIint32 intValue = (VXIint32)atoi(configValue);
      if((intValue == 0) && (configValue[0] != '0')) {
        delete [] wConfigKey;
        return VXIplatform_RESULT_FAILURE;
      }
      VXIMapSetProperty(configArgs, wConfigKey, 
                       (VXIValue *)VXIIntegerCreate(intValue));
    }
    else if(strcmp(configType, "VXIFloat") == 0) {
      /* Do not use atof( ), is not OS locale safe */
      VXIflt32 fltValue = (VXIflt32)StringToFloat(configValue);
      if((fltValue == 0.0) && 
        ((configValue[0] != '0') || (configValue[0] != '.'))) {
        delete [] wConfigKey;
        return VXIplatform_RESULT_FAILURE;
      }
      VXIMapSetProperty(configArgs, wConfigKey, 
                       (VXIValue *)VXIFloatCreate(fltValue));
    }
    else if(strcmp(configType, "VXIString") == 0) {
      VXIint valLen = strlen(configValue) + 1;
      VXIchar *wConfigValue = new VXIchar [valLen];
      if(wConfigValue == NULL) {
        delete [] wConfigKey;
        return VXIplatform_RESULT_OUT_OF_MEMORY;
      }
      char2wchar(wConfigValue, configValue, valLen);

      VXIMapSetProperty(configArgs, wConfigKey, 
                       (VXIValue *)VXIStringCreate(wConfigValue));
      delete [] wConfigValue;
    }
    else {
      delete [] wConfigKey;
      return VXIplatform_RESULT_UNSUPPORTED;
    }
  }

  delete [] wConfigKey;
  return VXIplatform_RESULT_SUCCESS;
}


extern "C"
VXIplatformResult ParseConfigFile (VXIMap **configArgs, const char *fileName)
{
  VXIplatformResult platformResult = VXIplatform_RESULT_SUCCESS;
  char buffer[READ_BUFFER_SIZE];
  VXIint lineNum = 0;
  char *readResult;

  if((configArgs == NULL) || (fileName == NULL)) {
    return VXIplatform_RESULT_INVALID_ARGUMENT;
  }

  *configArgs = VXIMapCreate();
  if( !*configArgs ) return VXIplatform_RESULT_OUT_OF_MEMORY;
    
  FILE *configFile = fopen(fileName, "r");
  if(configFile == NULL) {
    VXIclientError(NULL, MODULE_NAME, 108, 
                   L"Cannot open configuration file", L"%s%S",
                   L"file", fileName);
    return VXIplatform_RESULT_SYSTEM_ERROR;
  }

  readResult = fgets(buffer, READ_BUFFER_SIZE, configFile);

  while(readResult != NULL) {
    lineNum++;
    platformResult = ParseConfigLine(buffer, *configArgs);

    if(platformResult == VXIplatform_RESULT_FAILURE) {
      VXIclientError (NULL, MODULE_NAME, 300, 
                      L"Configuration file syntax error", L"%s%S%s%d",
                      L"file", fileName, L"line", lineNum);
      platformResult = VXIplatform_RESULT_SUCCESS;
    }
    else if(platformResult == VXIplatform_RESULT_UNSUPPORTED) {
      VXIclientError (NULL, MODULE_NAME, 301, 
                      L"Unsupported configuration file data", L"%s%S%s%d",
                      L"file", fileName, L"line", lineNum);
      platformResult = VXIplatform_RESULT_SUCCESS;
    }
    else if(platformResult != VXIplatform_RESULT_SUCCESS) {
      break;
    }

    readResult = fgets(buffer, READ_BUFFER_SIZE, configFile);
  }

  if((readResult == NULL) && !feof(configFile)) {
    VXIclientError (NULL, MODULE_NAME, 109, 
                    L"Configuration file read failed", L"%s%S",
                    L"file", fileName);
    platformResult = VXIplatform_RESULT_SYSTEM_ERROR;
  }

  fclose(configFile);
  return platformResult;
}

