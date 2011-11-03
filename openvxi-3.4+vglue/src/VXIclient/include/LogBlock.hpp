
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

#ifndef __LOG_BLOCK_HPP__
#define __LOG_BLOCK_HPP__

//
// Logging class to aid for simple log machanism.
// The use of this class is to create a static object in the local scope,
// and to be destroyed when exiting the scope.  It is not designed
// to be dynamically created and passed.
//
class LogBlock
{
  /**
   * Constructor printing a message that we are entering a block.  The return
   * code of the block is initialized to the value of <code>rc</code> which
   * defaults to <code>0</code>.
   **/
 public:
  LogBlock
  (
  	VXIlogInterface *log, 
  	unsigned int tag, 
  	const wchar_t* subtag,
  	const wchar_t* module,
  	int rc = 0
  )
  : _logHandle(log), _tag(tag), _subtag(subtag), _module(module), _rc(rc), 
  	_tagIsEnabled(2), _isEnableFlag(false)
  {
    if (isEnabled())
      logDiagnostic(L"entered.");
  }

  /**
   * Destructor printing a message that we are exiting a block.
   **/
  ~LogBlock()
  {
   	if (isEnabled())
     	logDiagnostic(L"return: rc = %d", _rc);
  }

 public:
  /**
   * Sets the return code for this block.
   *
   * @param _rc The new value of the return code for the block.
   * @return <code>this</code>.
   **/
  LogBlock& operator=(int rc)
  {
    _rc = rc;
    return *this;
  }

 public:
  /**
   * Converts a LogBlock into an int.
   * @return the current return code for this LogBlock.
   **/
  operator int()
  {
    return _rc;
  }

 public:
  bool isEnabled()
  {
    if(_tagIsEnabled ==2)
    {
      _isEnableFlag = isEnabled(_tag);
      _tagIsEnabled = 1;
    }
    return _isEnableFlag;
  }

 public:
  bool isEnabled(unsigned int tag)
  {
    return (_logHandle != NULL &&
            _logHandle->DiagnosticIsEnabled != NULL &&
            _logHandle->DiagnosticIsEnabled(_logHandle, tag));
  }

 public:
 	// Log diagnostic messages based on the tag number passed at construction time
  void logDiagnostic(const wchar_t *format, ...)
  {
    va_list vargs;
    va_start(vargs, format);
    if (_logHandle != NULL && isEnabled() && _logHandle->VDiagnostic != NULL)
    {
      (_logHandle->VDiagnostic)(_logHandle, _tag, _subtag, format, vargs);
    }
    va_end(vargs);
  }

  void logDiagnostic(const wchar_t *format, va_list vargs)
  {
  	if (_logHandle != NULL && isEnabled() && _logHandle->VDiagnostic != NULL)
  	{
    	(_logHandle->VDiagnostic)(_logHandle, _tag, _subtag, format, vargs);
  	}	
	}

 	// Log diagnostic messages based on the tag number passed at construction time 
 	// plus the offset argument
  void logDiag(int offset, const wchar_t *format, ...)
  {
    va_list vargs;
    va_start(vargs, format);
    if (_logHandle != NULL && _logHandle->VDiagnostic != NULL)
    {
      (_logHandle->VDiagnostic)(_logHandle, _tag + offset, _subtag, format, vargs);
    }
    va_end(vargs);
  }

  void logDiag(int offset, const wchar_t *format, va_list vargs)
  {
  	if (_logHandle != NULL && _logHandle->VDiagnostic != NULL)
  	{
    	(_logHandle->VDiagnostic)(_logHandle, _tag + offset, _subtag, format, vargs);
  	}	
	}

 public:
  void logError(VXIunsigned errorID, const wchar_t *format, ...)
  {
    va_list vargs;
    va_start(vargs, format);
    if (_logHandle != NULL && _logHandle->VError != NULL)
    {
    	if( format )
      	(_logHandle->VError)(_logHandle, _module, errorID, format, vargs);
    	else
      	(_logHandle->Error)(_logHandle, _module, errorID, NULL);    		
    }
    va_end(vargs);
  }

  void logError(VXIunsigned errorID, const wchar_t *format, va_list vargs)
  {
    if (_logHandle != NULL && _logHandle->VError != NULL)
    {
      (_logHandle->VError)(_logHandle, _module, errorID, format, vargs);
    }
  }
  
  /* Just to ensure that LogBlock cannot be copied. */
 private:
  LogBlock(const LogBlock&);
  LogBlock& operator=(const LogBlock&);

 private:
  // members
  VXIlogInterface *_logHandle;
  int _tagIsEnabled;
  bool _isEnableFlag;
  unsigned int _tag;
  const wchar_t* _subtag;
  const wchar_t* _module;
  int _rc;
};

#endif

