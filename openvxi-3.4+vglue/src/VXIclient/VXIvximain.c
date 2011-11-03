
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>

#include <VXItrd.h>
#include <VXIlog.h>
#include <SBlog.h>
#include "SBlogListeners.h"
#include "VXIclient.h"
#include "VXIplatform.h"
#include "VXIclientUtils.h"
#include "VXIclientConfig.h"
#include "ConfigFile.h"

#include <vglue_tostring_c.h>

#include "VXIvximain.h"

/* Helper Macros */
#ifdef WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#define CHANNEL_CHECK_RESULT(_platform, _func, _res)                         \
    if ((VXIint)(_res) != 0) {                                               \
      VXIclientError((_platform), MODULE_NAME, 200, L"Function call failed", \
        L"%s%S%s%d%s%S%s%d", L"FUNC", _func, L"RESULT", (_res),              \
        L"FILE", __FILE__, L"LINE", __LINE__);                               \
      return ((VXItrdThreadArg)_res);                                        \
    }

/* Structure for controlling channel threads */
typedef struct ChannelThreadArgs
{
    VXIunsigned    channelNum;     //  The callid
    VXIunsigned    ipcfd;          //  The fd for IPC with voiceglue
    long           maxCalls;
    const VXIMap  *configArgs;
    const VXIchar *vxmlURL;
    VXIString     *vxmlURLString;  //  Container for vxmlURL above
    VXItrdThread  *threadHandle;
    long           delayCallSec;
    VXIchar       *ani;
    VXIString     *aniString;	   //  Container for ani above
    VXIchar       *dnis;
    VXIString     *dnisString;     //  Container for dnis above
    VXIString     *javascriptInitString;  //  taken from javascript_init
} ChannelThreadArgs;

/* Defaults for command line arguments */
#define MODULE_NAME                   L"VXIClient"
#define DEFAULT_CONFIG_FILENAME       "SBclient.cfg"
#define DEFAULT_MAX_CHANNELS          1
#define DEFAULT_MAX_CALLS             -1 /* loop forever */

/* Helper funtions */
void Usage(void)
{
  printf("\nCommand-line arguments :\n"
        "[-v ] [-version] "
        "[-url vxmlDocURL] [-channels nbChannels] [-config configFile]\n"
        "[-calls maxCalls] [-delay nbSeconds] [-sbinet]\n"
        "To show version specifiy either -v or -version\n"
        "To run multiple channels, set nbChannels to desired number\n"
        "To take unlimited calls, set maxCalls to -1\n"
        "To simulate delay between call, set nbSeconds to desired seconds\n"
        "-sbinet used by Vocalocity only to indicate sbinet is passed to OSR\n\n");
}

void ErrorMsg(const char* msg, const char* arg) {
  printf("***ERROR***: %s%s\n", msg, (arg ? arg : "") );
//  _exit(-1);  
}

static void 
ShowResult(const VXIValue *result_val, int ChanNum, VXIplatform *platform)
{
  switch (VXIValueGetType(result_val)) {
  case VALUE_INTEGER:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIInteger Result Value: %d",
     VXIIntegerValue((const VXIInteger *) result_val));
    break;

  case VALUE_FLOAT:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIFloat Result Value: %f", 
     VXIFloatValue((const VXIFloat *) result_val));
    break;

  case VALUE_DOUBLE: {
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIDouble Result Value: %f", VXIDoubleValue((const VXIDouble *) result_val));
  } break;

  case VALUE_ULONG:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIULong Result Value: %u", 
     VXIULongValue((const VXIULong *) result_val));
    break;

  case VALUE_LONG:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXILong Result Value: %u", 
     VXILongValue((const VXILong *) result_val));
    break;

  case VALUE_STRING:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIString Result Value: %s", 
     VXIStringCStr((const VXIString *) result_val));
    break;

  case VALUE_PTR:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"Result Type: VXIPtr: 0x%p",
     VXIPtrValue((const VXIPtr *) result_val));
    break;
    
  case VALUE_MAP:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"Result Type: VXIMap");
    break;

  case VALUE_VECTOR:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"Result Type: VXIVector");
    break;

  case VALUE_CONTENT:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"Result Type: VXIContent");
    break;
  
  case VALUE_BOOLEAN:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"VXIBoolean Result Value: %d", 
     VXIBooleanValue((const VXIBoolean *) result_val));
    break;
  
  default:
    VXIclientDiag(platform, 60001, L"testClient::ShowResult",
     L"Result Type: Unknown type: %d", VXIValueGetType(result_val));
    break;
  }
}

VXIString *utf8_to_vxistring (char *utf8_string)
{
    VXIString *result;
    int len = strlen(utf8_string);
    VXIchar* tempstr = (VXIchar*)malloc(sizeof(VXIchar)*(len+1));
    if( !tempstr )
    {
	if (voiceglue_c_loglevel() >= LOG_ERR)
	{
	    char logmsg[256];
	    sprintf (logmsg, "malloc failed: %s errno=%d",
		     strerror(errno), errno);
	    voiceglue_c_log ((char) LOG_ERR, logmsg);
	};
	return (NULL);
    };
    if ( char2wchar(tempstr, utf8_string, len) != 0 )
    {
	if (voiceglue_c_loglevel() >= LOG_ERR)
	{
	    voiceglue_c_log ((char) LOG_ERR,
			     "Unable to convert char to wchar_t");
	};
	return (NULL);
    };
    result = VXIStringCreate(tempstr);
    free(tempstr);
    return result;
}

/*!  This translates to:
**   VXItrdThreadArg ChannelThread (VXItrdThreadArg userData)
**   and is the entry point for a newly created call thread
*/
static VXITRD_DEFINE_THREAD_FUNC(ChannelThread, userData)
{
  const ChannelThreadArgs *channelArgs = (const ChannelThreadArgs *)userData;
  unsigned long numCalls = 0;
  VXIplatformResult platformResult;
  VXIplatform *platform;
  VXIMap *channelConfig;
  VXItrdTimer *delayTimer = NULL;

  /*  Register this thread's callid and ipcfd  */
  voiceglue_c_registeripcfd (channelArgs->ipcfd, channelArgs->channelNum);

  /* Create delay timer */  
  VXItrdTimerCreate(&delayTimer);

  /*  Create private copy of platform handle map,  */
  /*  have to de-allocate it at the end.  */
  if ( channelArgs->configArgs )
    channelConfig = VXIMapClone (channelArgs->configArgs);
  else
    channelConfig = NULL;

  platformResult = VXIplatformCreateResources(channelArgs->channelNum,
                                              channelConfig,
                                              &platform);
  if( platformResult != 0 ) {
    CHECK_RESULT_NO_RETURN(NULL, "VXIplatformCreateResources()", platformResult);
    return (VXItrdThreadArg)platformResult;
  }

  while ((channelArgs->maxCalls == -1) || (numCalls < channelArgs->maxCalls)) {
    numCalls++;

    platform->numCall = numCalls;
    if (voiceglue_c_loglevel() >= LOG_NOTICE)
    {
	char logmsg[256];
	sprintf (logmsg, "Channel %d: Starting VXML Call Processing\n",
		 channelArgs->channelNum);
	voiceglue_c_log ((char) LOG_NOTICE, logmsg);
    };
    platformResult = VXIplatformEnableCall(platform);
    if( platformResult != 0 ) {
      CHECK_RESULT_NO_RETURN(platform, "VXIplatformEnableCall()", platformResult);
      return (VXItrdThreadArg)platformResult;
    }
    
    platformResult = VXIplatformWaitForCall(platform);
    if( platformResult != 0 ) {
      CHECK_RESULT_NO_RETURN(platform, "VXIplatformWaitForCall()", platformResult);
      return (VXItrdThreadArg)platformResult;
    }

    //  Replace the bogus, test ani & dnis with the proper ones
    VXIMapSetProperty (platform->telephonyProps, L"dnis",
		       (VXIValue *)VXIStringCreate(channelArgs->dnis));
    VXIMapSetProperty (platform->telephonyProps, L"ani", 
		       (VXIValue *)VXIStringCreate(channelArgs->ani));

    VXIclientDiag(platform, 60001, L"testClient::ChannelThread", L"In a Call");
    {
      VXIchar *sessionArgScript = NULL;
      VXIValue *result = NULL ;

      /*  callInfo contains call-specific parameters  */
      VXIMap *callInfo = VXIMapCreate();
      VXIMapSetProperty (callInfo, L"dnis",
			 (VXIValue *)VXIStringCreate(channelArgs->dnis));
      VXIMapSetProperty (callInfo, L"ani",
			 (VXIValue *)VXIStringCreate(channelArgs->ani));
      VXIMapSetProperty (callInfo, L"vgid",
			 (VXIValue *)VXIULongCreate(channelArgs->channelNum));
      VXIMapSetProperty (callInfo, L"javascript_init",
			 (VXIValue *)
			 VXIStringClone (channelArgs->javascriptInitString));

      platformResult = VXIplatformProcessDocument(channelArgs->vxmlURL,
						  callInfo,
                                                  &sessionArgScript, 
                                                  &result, platform);

      VXIMapDestroy(&callInfo);
      if((VXIint) platformResult != 0){
        fprintf(stderr, "Error: ProcessDocument returned error code %i\n",
                platformResult);
      }
      else
      {
	  if(result)
	  {
	      ShowResult(result, channelArgs->channelNum, platform);

	      //  Report the result to the log
	      char *result_str = voiceglue_c_vxivalue_to_utf8string (result);
	      int result_strlen = strlen (result_str);
	      char *logmsg = malloc(result_strlen + 32);
	      sprintf (logmsg, "VXML processing returns: %s\n", result_str);
	      voiceglue_c_log ((char) LOG_NOTICE, logmsg);
	      free (logmsg);
	      free (result_str);

	      //  Send each returned value back to perl
	      if (VXIValueGetType(result) == VALUE_MAP)
	      {
		  const VXIchar *key = NULL;
		  const VXIValue *gvalue = NULL;
		  VXIMapIterator *it = VXIMapGetFirstProperty
		      ((const VXIMap*) result, &key, &gvalue);
		  int ret = 0;
		  while (ret == 0 && key && gvalue)
		  {
		      VXIvalueType gvalue_type = VXIValueGetType(gvalue);
		      if ((gvalue_type == VALUE_INTEGER) ||
			  (gvalue_type == VALUE_LONG) ||
			  (gvalue_type == VALUE_ULONG) ||
			  (gvalue_type == VALUE_FLOAT) ||
			  (gvalue_type == VALUE_DOUBLE) ||
			  (gvalue_type == VALUE_BOOLEAN) ||
			  (gvalue_type == VALUE_STRING))
		      {
			  //  A valid scalar result to return
			  char *buf;
			  char *bufptr;
			  int buflen = 8;
			  VXIString *stringified =
			      VXIValueToString(gvalue,key,
					       VALUE_FORMAT_URL_QUERY_ARGS);
			  int valuelen = SWIwcstoutf8len
			      (VXIStringCStr(stringified));
			  buflen += valuelen + 2;
			  buf = malloc(buflen);
			  bufptr = buf;
			  strcpy (bufptr, "exitval ");
			  bufptr += 8;
			  SWIwcstoutf8(VXIStringCStr(stringified),
				       bufptr,valuelen);
			  bufptr += valuelen;
			  *bufptr = '\n';
			  ++bufptr;
			  *bufptr = '\0';
			  voiceglue_c_sendipcmsg (buf);
			  free(buf);
			  buf = voiceglue_c_getipcmsg();
			  if (strcmp (buf,"exitedval"))
			  {
			      char *logmsg = malloc(strlen(buf) + 32);
			      sprintf (logmsg,
				       "Invalid response to exitval: %s\n",
				       buf);
			      voiceglue_c_log ((char) LOG_ERR, logmsg);
			      free (logmsg);
			  };
			  free(buf);
		      };
		      ret = VXIMapGetNextProperty(it, &key, &gvalue);
		  };
		  VXIMapIteratorDestroy(&it);
	      };

	      VXIValueDestroy(&result);  /* Notes: must deallocate result */
	  }
	  else
	  {
	      VXIclientDiag(platform, 60001, L"testClient::ChannelThread", L"NULL result");
	  }
      }
        
      /* free session ECMAScript */
      if( sessionArgScript )
      {
        free(sessionArgScript);
        sessionArgScript = NULL;
      }
    }
    VXIclientDiag(platform, 60001, L"testClient::ChannelThread", L"Call Terminated");
    if (voiceglue_c_loglevel() >= LOG_NOTICE)
    {
	char logmsg[256];
	sprintf (logmsg, "Channel %d: Call Terminated\n",
		 channelArgs->channelNum);
	voiceglue_c_log ((char) LOG_NOTICE, logmsg);
    };
    
    /* Simulate delay if user specify delay time */
    if( (channelArgs->delayCallSec > 0) &&
        (channelArgs->maxCalls == -1) || (numCalls < channelArgs->maxCalls) )
      VXItrdTimerSleep(delayTimer, channelArgs->delayCallSec * 1000, NULL);  
  }

  platformResult = VXIplatformDestroyResources(&platform);
  if( delayTimer ) VXItrdTimerDestroy(&delayTimer);   
  if (channelConfig)
    VXIMapDestroy(&channelConfig);

  //  This closes the thread's IPC fd
  voiceglue_c_unregisteripcfd();

  if( platformResult != 0 ) {
    CHECK_RESULT_NO_RETURN(platform, "VXIplatformDestroyResources()", platformResult);
    return (VXItrdThreadArg)platformResult;
  }
  return 0;
}

/*!
**  Parses config file
**  @param config_args Where to place the arguments found in the config file
**  @param file_path What file to read the config args from
*/
int voiceglue_parse_config_file (VXIMap** config_args, const char *file_path)
{
    return (ParseConfigFile (config_args, file_path));
};

//  Used as permanent platform handle for voiceglue_free_vxmldocument
VXIplatform *docfree_platform = NULL;

/*!
**  Initializes VXImap platform profile
**  @param config_args arguments found in the config file
**  @param num_channesl maximum number of channels to permit
*/
int voiceglue_init_platform (VXIMap *config_args, VXIunsigned *num_channels)
{
    VXIplatformResult result;
    result = VXIplatformInit (config_args, num_channels);
    VXIplatformCreateResources (-1, config_args, &docfree_platform);
    return result;
};

/*!
**  Frees a cached VXMLDocument* parse tree.
**  @param addr_string pointer in string form
*/
int voiceglue_free_vxmldocument (const VXIchar *addr)
{
    VXIinterpreterValidate(docfree_platform->VXIinterpreter, addr);
};

/*!
** Starts a VXML thread call processing thread.
** @param platformHandle Handle returned from vxiStartPlatform().
** @param callid Unique integer to refer to this call.
** @param ipcfd The file descriptor used for IPC with voiceglue.
** @param url The ASCII URL of the VXML start page.
** @param ani The ASCII ANI (caller-ID) of the incoming call.
** @param dnis The ASCII DNIS (DID) of the incoming call.
** @param javascript_init The call-specific session-level javascript init
** @param channelHandle Memory in which is returned the channel thread handle.
** @return 0 on success, 1 on failure (on failure, check message log)
*/
int vxiStartOneThread (void *platformHandle,
		       int callid, int ipcfd, char *url,
		       char *ani, char *dnis,
		       char *javascript_init,
		       void **channelHandle)
{
  int showVersion       = 0;
  int nbChannels        = 1;
  VXIplatformResult pfResult;
  ChannelThreadArgs *channelArgs = NULL;
  VXItrdThreadArg status;
  VXItrdResult trdResult;

  /* Start the call processing thread */

  //  Have to de-allocate: channelArgs
  channelArgs = (ChannelThreadArgs *)calloc(1, sizeof(ChannelThreadArgs));
  CHECK_MEMALLOC_RETURN(NULL, channelArgs, "ChannelThreadArgs array");

  //  Have to de-allocate: channelArgs, channelArgs->vxmlURLString
  channelArgs->vxmlURLString = utf8_to_vxistring(url);

  channelArgs->channelNum = callid;
  channelArgs->ipcfd = ipcfd;
  channelArgs->maxCalls = 1;
  channelArgs->configArgs = (VXIMap *) platformHandle;
  channelArgs->vxmlURL = VXIStringCStr(channelArgs->vxmlURLString);
  channelArgs->delayCallSec = 0;
  //  Have to de-allocate: channelArgs, channelArgs->vxmlURLString,
  //                       channelArgs->aniString, channelArgs->dnisString
  //                       channelArgs->javascriptInitString
  channelArgs->aniString = utf8_to_vxistring(ani);
  channelArgs->ani = VXIStringCStr(channelArgs->aniString);
  channelArgs->dnisString = utf8_to_vxistring(dnis);
  channelArgs->dnis = VXIStringCStr(channelArgs->dnisString);
  channelArgs->javascriptInitString = utf8_to_vxistring(javascript_init);

  //  channelArgs->threadHandle is where the thread handle gets returned.
  //  ChannelThread is the function to call.
  //  channelArgs gets passed to the ChannelThread() function.
  trdResult = VXItrdThreadCreate(&(channelArgs->threadHandle),
				 ChannelThread,
				 (VXItrdThreadArg) channelArgs);
  CHECK_RESULT_RETURN(NULL, "VXItrdThreadCreate()", trdResult);

  *channelHandle = (void *) channelArgs;
  return (0);
};

/*!
** Terminates a channel thread with the handle channelHandle
** from vxiStartOneThread() (really a ChannelThreadArgs *)
** @param channelHandle The handle returned from vxiStartOneThread().
** @return 0 on success.
*/
int vxiFinishThread (void *channelHandle)
{
    VXItrdThreadArg status;
    VXItrdResult trdResult;
    VXIplatformResult pfResult;
    ChannelThreadArgs *channelArgs = (ChannelThreadArgs *) channelHandle;

    /* Wait for a thread to finish */
    trdResult = VXItrdThreadJoin (channelArgs->threadHandle, &status, -1);
    CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadJoin()", trdResult);

    trdResult = VXItrdThreadDestroyHandle (&(channelArgs->threadHandle));
    CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadDestroyHandle()",trdResult);

    if ( status != 0 ) {
      /* Need to cast this twice, some compilers won't allow going directly
         to VXIplatformResult */
      pfResult = (VXIplatformResult) status;
      CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadJoin()", pfResult);
    }

    if(channelArgs)
    {
	//  de-allocate: channelArgs, channelArgs->vxmlURLString,
	//               channelArgs->aniString, channelArgs->dnisString
	//               channelArgs->javascriptInitString
	VXIStringDestroy (&channelArgs->vxmlURLString);
	channelArgs->vxmlURLString = (VXIString *) NULL;
	VXIStringDestroy (&channelArgs->aniString);
	channelArgs->aniString = (VXIString *) NULL;
	VXIStringDestroy (&channelArgs->dnisString);
	channelArgs->dnisString = (VXIString *) NULL;
	VXIStringDestroy (&channelArgs->javascriptInitString);
	channelArgs->javascriptInitString = (VXIString *) NULL;
	free(channelArgs);
    }

    return (0);
}

/*!
** Stops the VXI platform.  The
** config args configArgs are from vxiStartPlatform().
** @param platformHandle The handle returned from vxiStartPlatform
** @return 0 on success, o.w. errmsg is filled with an error message
*/
int vxiStopPlatform (void *platformHandle)
{
    VXIplatformResult pfResult;
    VXIMap *configArgs = (VXIMap *) platformHandle;

  /* Shut down the platform */
  pfResult = VXIplatformShutdown();
  CHECK_RESULT_NO_RETURN(NULL, "VXIplatformShutdown()", pfResult);
  
  /* clean up memory */
  if (configArgs) VXIMapDestroy(&configArgs);

  return (int)pfResult;
}

/*!
**   Turns VXI platform diagnostic tracing on or off.
**   @param state true means to turn on, false means off.
*/
void vxiSetTrace (int state)
{
    VXIplatformSetTrace (state);
};

void vxiFreeVXMLDocument (void *ptr)
{
    /*
    VXMLDocument *doc = (VXMLDocument *) ptr;
    delete doc;
    */
};

int vximain( int argc, char **argv)
{
  int i                 = 0;
  int len               = 0;
  int showVersion       = 0;
  int nbChannels        = 1;
  int maxCalls          = 1;
  int nbSeconds         = 0;
  int usedInternalInet  = 0;
  VXIMap* configArgs    = NULL;
  VXIString* vxmlURL    = NULL;
  VXIchar* tempstr      = NULL;
  char* cfgURL          = NULL;
  const char* vxmlURLptr= NULL;
  const char* cfgURLptr = NULL;
  VXIplatformResult pfResult;
  ChannelThreadArgs *channelArgs = NULL;
  VXItrdThreadArg status;
  VXItrdResult trdResult;
  
  Usage();
  
  /* Parse command line */  
  for( i=1; i<argc; ++i ) {
    if( strcmp(argv[i], "-url") == 0 ) {
      if( vxmlURL ) VXIStringDestroy(&vxmlURL);
      if( argv[++i] == NULL ) ErrorMsg("Missing vxmlDocURL argument", NULL);
      vxmlURLptr = argv[i];
      len = strlen(vxmlURLptr);
      tempstr = (VXIchar*)malloc(sizeof(VXIchar)*(len+1));
      if( !tempstr ) ErrorMsg("System memory exhausted!", NULL);
      if ( char2wchar(tempstr, vxmlURLptr, len) != 0 )
        ErrorMsg("Unable to convert char to wchar_t", NULL);
      vxmlURL = VXIStringCreate(tempstr);
      free(tempstr);      
    }
    else if( strcmp(argv[i], "-channels") == 0 ) {
      if( argv[++i] == NULL ) ErrorMsg("Missing nbChannels argument", NULL);
      nbChannels = atoi(argv[i]);
      if( nbChannels < 1 ) ErrorMsg("nbChannels must be greater than 0.", NULL);
      if( nbChannels == UINT_MAX ) ErrorMsg("nbChannels is too big", NULL);       
    }
    else if( strcmp(argv[i], "-calls") == 0 ) {
      if( argv[++i] == NULL ) ErrorMsg("Missing maxCalls argument", NULL);
      maxCalls = atoi(argv[i]);
    }
    else if( strcmp(argv[i], "-delay") == 0 ) {
      if( argv[++i] == NULL ) ErrorMsg("Missing nbSeconds argument", NULL);
      nbSeconds = atoi(argv[i]);
      if( nbSeconds < 0 ) ErrorMsg("nbSeconds must be positive.", NULL);
    }
    else if( strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
      showVersion = 1;
    }
    else if( strcmp(argv[i], "-config") == 0 ) {
      if( argv[++i] == NULL ) ErrorMsg("Missing configFile argument", NULL);
      cfgURL = argv[i];
      cfgURLptr = cfgURL;
    }   
    else if( strcmp(argv[i], "-sbinet") == 0 ) {
      usedInternalInet = 1;  
    }
    else ErrorMsg("Unknown argument: ", argv[i]);
  }
    
  /* Make sure the vxml url is given */
  if( vxmlURL == NULL && !showVersion ) 
    ErrorMsg("Required argument -url is missing!", NULL);
  
  /* If user does not specify -config, use defaut config. file */
  {
    const char* sbenv = getenv( "SWISBSDK" );
    if( cfgURL == NULL ) {
      if( !sbenv ) 
        ErrorMsg("Required environment SWISBSDK is not defined!", NULL);
      cfgURL = (char*)malloc
               (sizeof(char)*(strlen(DEFAULT_CONFIG_FILENAME)+strlen(sbenv)+10));
      if( !cfgURL ) ErrorMsg("System memory exhausted!", NULL);
      sprintf(cfgURL, "%s%cconfig%c%s", sbenv, PATH_SEPARATOR,
              PATH_SEPARATOR, DEFAULT_CONFIG_FILENAME);
    } 
  }
  
  /* Show argument list */
  if( !showVersion ) {
    printf("\n===========================================================\n");
    printf("vxmlDocURL: %s\n", (vxmlURLptr ? vxmlURLptr : "") );
    printf("configFile: %s%s\n", cfgURL, (cfgURLptr ? "" : " (default)") ); 
    printf("nbChannels: %d, maxCalls: %d%s, nbSeconds: %d\n", nbChannels, 
            maxCalls, (maxCalls < 0 ? "(unlimited calls)" : ""), nbSeconds);
    printf("===========================================================\n\n");
  }
  
  /* Parse the configuration file */
  pfResult = ParseConfigFile(&configArgs, cfgURL);
  CHECK_RESULT_RETURN(NULL, "ParseConfigFile()", pfResult);

  /* For use by Vocalocity only */
  if( usedInternalInet ) {
    VXIMapSetProperty(configArgs, OSR_USE_INTERNAL_SBINET, 
                      (VXIValue*)VXIIntegerCreate(1));
  }

  /* For show version mode, enable the version output diagnostic tag,
     and turn on logging to standard output */
  if( showVersion ) {
    char ntemp[64];
    wchar_t temp[1024];
    const VXIValue *clientBase = VXIMapGetProperty(configArgs, CLIENT_CLIENT_DIAG_BASE);
    if (VXIValueGetType(clientBase) != VALUE_INTEGER) {
      ErrorMsg("client.diagBase must be set to a VXIInteger value", NULL);
    }
      
    sprintf(ntemp, "%d", VXIIntegerValue((const VXIInteger*)clientBase) + 1);
    wcscpy(temp, CLIENT_LOG_DIAG_TAG_KEY_PREFIX);
    char2wchar(&temp[wcslen(temp)], ntemp, strlen(ntemp) + 1);
    VXIMapSetProperty(configArgs, temp, (VXIValue *) VXIIntegerCreate(1));      
    VXIMapSetProperty(configArgs, CLIENT_LOG_LOG_TO_STDOUT,
		                 (VXIValue *) VXIIntegerCreate(1));
  }
  
  /* Initialize the platform */
  pfResult = VXIplatformInit(configArgs, (VXIunsigned*)&nbChannels);
  CHECK_RESULT_RETURN(NULL, "VXIplatformInit()", pfResult);

  /* In show version mode, initialize one channel to get the version
     numbers, then exit */
  if( showVersion ) {
    VXIplatform *platform = NULL;
    pfResult = VXIplatformCreateResources(0, configArgs, &platform);
    CHECK_RESULT_RETURN(NULL, "VXIplatformCreateResources()",	pfResult);
    pfResult = VXIplatformDestroyResources(&platform);
    CHECK_RESULT_RETURN(NULL,"VXIplatformDestroyResources()", pfResult);
    goto shutdown;
  }
  
  /* Start the call processing threads */
  if (nbChannels > 0) {
    channelArgs = (ChannelThreadArgs *)calloc(nbChannels, sizeof(ChannelThreadArgs));
    CHECK_MEMALLOC_RETURN(NULL, channelArgs, "ChannelThreadArgs array");

    for (i = 0; i < nbChannels ; i++) {
      channelArgs[i].channelNum = i;
      channelArgs[i].maxCalls = maxCalls;
      channelArgs[i].configArgs = configArgs;
      channelArgs[i].vxmlURL = VXIStringCStr(vxmlURL);
      channelArgs[i].delayCallSec = nbSeconds;
      
      trdResult = VXItrdThreadCreate(&channelArgs[i].threadHandle,
             ChannelThread,
             (VXItrdThreadArg) &channelArgs[i]);
      CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadCreate()", trdResult);
    }
  }
  else {
    fprintf(stderr, "ERROR: No channels available, file %s, line %i\n",
      __FILE__, __LINE__);
  }

  /* Wait for threads to finish */
  for (i = 0; i < nbChannels; i++) {
    trdResult = VXItrdThreadJoin (channelArgs[i].threadHandle, &status, -1);
    CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadJoin()", trdResult);

    trdResult = VXItrdThreadDestroyHandle (&channelArgs[i].threadHandle);
    CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadDestroyHandle()",trdResult);

    if ( status != 0 ) {
      /* Need to cast this twice, some compilers won't allow going directly
         to VXIplatformResult */
      pfResult = (VXIplatformResult) status;
      CHECK_RESULT_NO_RETURN(NULL, "VXItrdThreadJoin()", pfResult);
    }
  }

shutdown:    
  /* Shut down the platform */
  pfResult = VXIplatformShutdown();
  CHECK_RESULT_NO_RETURN(NULL, "VXIplatformShutdown()", pfResult);
  
  /* clean up memory */
  if( !cfgURLptr ) free(cfgURL);
  if( vxmlURL ) VXIStringDestroy(&vxmlURL);
  if( configArgs ) VXIMapDestroy(&configArgs);
  if( channelArgs ) free(channelArgs);

  if (pfResult == VXIplatform_RESULT_SUCCESS)
    printf("Successfully exiting\n");
  else
    printf("Exiting with errors, rc = %d\n", pfResult);
  
  return (int)pfResult;
}
