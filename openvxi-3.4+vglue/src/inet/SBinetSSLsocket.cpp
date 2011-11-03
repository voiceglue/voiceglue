
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

#include "SBinetSSLsocket.hpp"
#include "VXItrd.h"

SSL_CTX *SBinetSSLsocket::_ctx = NULL;
const SSL_METHOD *SBinetSSLsocket::_meth = NULL;

static VXItrdMutex ** sslMutex = NULL;

static void ssl_locking_callback(int mode, int type,
                                 char *file, int line)
{

  if (mode & CRYPTO_LOCK)
    VXItrdMutexLock(sslMutex[type]);
  else
    VXItrdMutexUnlock(sslMutex[type]);
}

struct CRYPTO_dynlock_value
{
 public:
  CRYPTO_dynlock_value():_mutex(NULL)
  {}

  ~CRYPTO_dynlock_value()
  {
    if (_mutex) VXItrdMutexDestroy(&_mutex);
  }

  VXItrdMutex *_mutex;
};

static CRYPTO_dynlock_value *dyn_create_function(const char *file,
                                                 int line)
{
  CRYPTO_dynlock_value *p = new CRYPTO_dynlock_value;
  if (VXItrdMutexCreate(&p->_mutex) != VXItrd_RESULT_SUCCESS)
  {
    delete p;
    p = NULL;
  }
  return p;
}

static void dyn_lock_function(int mode,
                              CRYPTO_dynlock_value *p,
                              const char *file,
                              int line)
{
  if (mode & CRYPTO_LOCK)
    VXItrdMutexLock(p->_mutex);
  else
    VXItrdMutexUnlock(p->_mutex);
}

static void dyn_destroy_function(CRYPTO_dynlock_value *p,
                                 const char *file,
                                 int line)
{
  delete p;
}

static int thread_cleanup()
{
  for (int i = CRYPTO_num_locks() - 1; i >= 0; i--)
  {
    VXItrdMutexDestroy(&sslMutex[i]);
    sslMutex[i] = NULL;
  }
  delete [] sslMutex;
  sslMutex = NULL;

  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);

  return 0;
}

static int thread_setup()
{
  int n = CRYPTO_num_locks();
  sslMutex = new VXItrdMutex *[n];

  int i;

  for (i = 0; i < n; i++) sslMutex[i] = NULL;

  for (i = 0; i < n; i++)
  {
    if (VXItrdMutexCreate(&sslMutex[i]) != VXItrd_RESULT_SUCCESS)
      goto failure;
  }

  CRYPTO_set_locking_callback((void (*)(int,int, const char *,int))
                              ssl_locking_callback);

  CRYPTO_set_id_callback((unsigned long(*)()) VXItrdThreadGetID);

  CRYPTO_set_dynlock_create_callback((CRYPTO_dynlock_value *
                                      (*)(const char *, int))
                                     dyn_create_function);
  CRYPTO_set_dynlock_lock_callback((void (*)(int,
                                             CRYPTO_dynlock_value *,
                                             const char *,
                                             int))
                                   dyn_lock_function);

  CRYPTO_set_dynlock_destroy_callback((void (*)(CRYPTO_dynlock_value *,
                                                const char *,
                                                int))
                                      dyn_destroy_function);

  return 0;

 failure:
  thread_cleanup();

  return -1;
}

int SBinetSSLsocket::initialize()
{
  SSLeay_add_ssl_algorithms();
  _meth = SSLv23_client_method();
  SSL_load_error_strings();
  _ctx = SSL_CTX_new ((SSL_METHOD *) _meth);
  if (_ctx == NULL) return -1;

  SSL_CTX_set_mode(_ctx, SSL_MODE_AUTO_RETRY);

  return thread_setup();
}


int SBinetSSLsocket::shutdown()
{
  thread_cleanup();
  SSL_CTX_free(_ctx);
  return 0;
}

// SBinetSSLsocket::SBinetSSLsocket
// Refer to SBinetSSLsocket.hpp for doc.
SBinetSSLsocket::SBinetSSLsocket(Type socktype, SWIutilLogger *logger):
  SWIsocket(socktype, logger),_ssl(NULL)
{}

// SBinetSSLsocket::~SBinetSSLsocket
// Refer to SBinetSSLsocket.hpp for doc.
SBinetSSLsocket::~SBinetSSLsocket()
{
  close();
  if (_ssl) SSL_free(_ssl);
}

SWIstream::Result SBinetSSLsocket::connect(const SWIipAddress& endpoint, long timeout)
{
  SWIstream::Result rc = SWIsocket::connect(endpoint, timeout);

  if (rc != SWIstream::SUCCESS) return rc;

  rc = SWIstream::FAILURE;

  if ((_ssl = SSL_new(_ctx)) == NULL)
    error(L"SSL_new", 700);
  else if (SSL_set_fd(_ssl, getSD()) == 0)
    error(L"SSL_set_fd", 700);
  else if (SSL_connect(_ssl) == -1)
    error(L"SSL_connect", 700);
  else
    rc = SWIstream::SUCCESS;

  if (rc != SWIstream::SUCCESS && _ssl != NULL)
  {
    SSL_free(_ssl);
    _ssl = NULL;
  }

  return rc;

}

int SBinetSSLsocket::read(void* buf, int len)
{
  return recv(buf, len);
}

int SBinetSSLsocket::recv(void* buf, int len, int msgf)
{
  for (;;)
  {
    int rc1 = SSL_read(_ssl, buf, len);
    int rc2 = SSL_get_error(_ssl, rc1);

    switch (rc2)
    {
     case SSL_ERROR_NONE:
       return rc1;
     case SSL_ERROR_ZERO_RETURN:
       return SWIstream::END_OF_FILE;
     case SSL_ERROR_WANT_READ:
     case SSL_ERROR_WANT_WRITE:
       // should not really happend because of the SSL_MODE_AUTO_RETRY.
       // Do nothing, just try again.
       // Maybe we should yield the thread or sleep to avoid CPU hog
       error(L"SSL_read", 701, L"SSL_get_error", rc2);
       break;
     default:
       error(L"SSL_read", 700, L"SSL_get_error", rc2);
       return SWIstream::READ_ERROR;
    }
  }
}

int SBinetSSLsocket::recvfrom(SWIipAddress& sa, void* buf, int len, int msgf)
{
  int rc = recv(buf, len, msgf);
  if (rc >= 0)
  {
    const SWIipAddress *p = getRemoteAddress();
    //if (p != NULL) sa = *p;
  }

  return rc;
}

int SBinetSSLsocket::write(const void* buf, int len)
{
  return send(buf, len);
}

int SBinetSSLsocket::send(const void* buf, int len, int msgf)
{
  for (;;)
  {
    int rc1 = SSL_write(_ssl, const_cast<void *>(buf), len);
    int rc2 = SSL_get_error(_ssl, rc1);

    switch (rc2)
    {
     case SSL_ERROR_NONE:
       return rc1;
     case SSL_ERROR_ZERO_RETURN:
       return SWIstream::END_OF_FILE;
     case SSL_ERROR_WANT_READ:
     case SSL_ERROR_WANT_WRITE:
       // should not really happend because of the SSL_MODE_AUTO_RETRY.
       // Do nothing, just try again.
       // Maybe we should yield the thread or sleep to avoid CPU hog
       error(L"SSL_write", 701, L"SSL_get_error", rc2);
       break;
     default:
       error(L"SSL_write", 700, L"SSL_get_error", rc2);
       return SWIstream::READ_ERROR;
    }
  }
}

int SBinetSSLsocket::sendto(const SWIipAddress& sa, const void* buf, int len, int msgf)
{
  return send(buf,len,msgf);
}

SWIstream::Result SBinetSSLsocket::shutdown(shuthow sh)
{
  // Not sure how to handler shutdown in SSL context.  However, shutdown
  // should not be used in SBinet.
  error(L"SBinetSSLsocket::shutdown", 701);
  return SWIstream::FAILURE;
}

SWIstream::Result SBinetSSLsocket::close()
{
  if (_ssl != NULL)
  {
    SSL_shutdown(_ssl);
    SSL_free(_ssl);
    _ssl = NULL;
  }

  return SWIsocket::close();
}
