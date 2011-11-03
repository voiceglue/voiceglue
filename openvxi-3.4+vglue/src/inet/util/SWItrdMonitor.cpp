
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

#include "SWIutilInternal.h"

#ifndef _WIN32
#include <sys/time.h>
#include <errno.h>
#define INFINITE (~0)
#endif

#include "SWItrdMonitor.hpp"
#include <VXItrd.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#endif

// ---------- Constants ----------

const int SWItrdMonitor::SUCCESS = 0;
const int SWItrdMonitor::FAILURE = -1;
const int SWItrdMonitor::NOT_OWNER = 1;

// Static initialization for WIN32
#ifdef _WIN32

static void *createMutex(bool locked)
{
  HANDLE *k = new HANDLE;
  *k = CreateMutex(NULL, locked ? TRUE : FALSE, NULL);
  return k;
}

static void deleteMutex(void *mutex)
{
  HANDLE *k = (HANDLE *) mutex;
  CloseHandle(*k);
  delete k;
}

static int lockMutex(void* mutex)
{
  int rc = SWItrdMonitor::SUCCESS;
  switch (WaitForSingleObject(*(static_cast<HANDLE *>(mutex)), INFINITE))
  {
   case WAIT_FAILED:
     rc = SWItrdMonitor::FAILURE;
     break;
   case WAIT_ABANDONED:
     // should we log this?
     break;
  }
  return rc;
}

static int unlockMutex(void* mutex)
{
  return ReleaseMutex(*(static_cast<HANDLE *>(mutex))) ?
    SWItrdMonitor::SUCCESS :
    SWItrdMonitor::FAILURE;
}

static int waitForEvent(void *mutex,
                        void *event,
                        unsigned long millisecs,
                        bool *expiredF)
{
  if (::unlockMutex(mutex) != SWItrdMonitor::SUCCESS)
  {
    return SWItrdMonitor::FAILURE;
  }

  int rc = SWItrdMonitor::SUCCESS;

  DWORD waitrc = WaitForSingleObject(*(static_cast<HANDLE *>(event)),
                                     millisecs);
  switch (waitrc)
  {
   case WAIT_OBJECT_0:
     if (expiredF != NULL) *expiredF = false;
     break;
   case WAIT_TIMEOUT:
     if (expiredF != NULL) *expiredF = true;
     break;
   case WAIT_FAILED:
     rc = SWItrdMonitor::FAILURE;
     break;
   case WAIT_ABANDONED:
     // should we log this?
     break;
  }
  ::lockMutex(mutex);
  return rc;
}

static int notifyEvent(void *event, bool broadcast)
{
  SetEvent(*(static_cast<HANDLE *>(event)));
  return SWItrdMonitor::SUCCESS;
}

#else

static void *createMutex(bool locked)
{
  pthread_mutex_t *lock = new pthread_mutex_t;
  pthread_mutex_init(lock,NULL);
  if (locked) pthread_mutex_lock(lock);
  return lock;
}

static void deleteMutex(void *mutex)
{
  pthread_mutex_t *lock = (pthread_mutex_t *) mutex;
  pthread_mutex_destroy(lock);
  delete lock;
}

static int lockMutex(void *mutex)
{
  return pthread_mutex_lock(static_cast<pthread_mutex_t *>(mutex)) == 0 ?
    SWItrdMonitor::SUCCESS :
    SWItrdMonitor::FAILURE;
}

static int unlockMutex(void *mutex)
{
  return pthread_mutex_unlock(static_cast<pthread_mutex_t *>(mutex)) == 0 ?
    SWItrdMonitor::SUCCESS :
    SWItrdMonitor::FAILURE;
}

static int waitForEvent(void *mutex, void *event,
                        unsigned long millisecs,
                        bool *expiredF)
{
  int rc = SWItrdMonitor::SUCCESS;

  if (millisecs == INFINITE)
  {
    if (pthread_cond_wait(static_cast<pthread_cond_t *>(event),
                          static_cast<pthread_mutex_t *>(mutex)) != 0)
      rc = SWItrdMonitor::FAILURE;
  }
  else
  {
    struct timeval now;
    struct timespec timeout;

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + millisecs / 1000;
    unsigned long microsecs = now.tv_usec + (millisecs % 1000) * 1000;
    if (microsecs >= 1000000)
    {
      microsecs -= 1000000;
      timeout.tv_sec++;
    }
    timeout.tv_nsec = microsecs * 1000;

    rc = pthread_cond_timedwait(static_cast<pthread_cond_t *>(event),
                                static_cast<pthread_mutex_t *>(mutex),
                                &timeout);

    switch (rc)
    {
     case 0:
       if (expiredF != NULL) *expiredF = false;
       rc = SWItrdMonitor::SUCCESS;
       break;
     case ETIMEDOUT:
       if (expiredF != NULL) *expiredF = true;
       rc = SWItrdMonitor::SUCCESS;
       break;
     default:
       rc = SWItrdMonitor::FAILURE;
    }
  }
  return rc;
}

static int notifyEvent(void *event, bool broadcast)
{
  pthread_cond_t *p = static_cast<pthread_cond_t *>(event);
  int rc;

  if (broadcast)
    rc = pthread_cond_broadcast(p);
  else
    rc = pthread_cond_signal(p);

  if (rc == 0)
    rc = SWItrdMonitor::SUCCESS;
  else
    rc = SWItrdMonitor::FAILURE;

  return rc;
}

#endif

void* SWItrdMonitor::_globalLock = createMutex(false);
SWItrdMonitor::HandleListItem * volatile SWItrdMonitor::_freeThreadList = NULL;
SWItrdMonitor::HandleListItem * volatile SWItrdMonitor::_freeMutexList = NULL;

// SWItrdMonitor::SWItrdMonitor
// Refer to SWItrdMonitor.hpp for doc.
SWItrdMonitor::SWItrdMonitor(Policy policy):
  _ownerThread((VXIthreadID) -1),_lockCount(0),
  _firstWaitingThread(NULL),_lastWaitingThread(NULL),
  _mutex(NULL),
  _policy(policy)
{
  if (policy != POOLED)
  {
    _mutex = new HandleListItem;
    // We don't initialize the other fields as we don't really care
    _mutex->_handle = ::createMutex(false);
  }
}

// SWItrdMonitor::~SWItrdMonitor
// Refer to SWItrdMonitor.hpp for doc.
SWItrdMonitor::~SWItrdMonitor()
{
  ::lockMutex(_globalLock);

  // If the list of waiting threads is not NULL, we unlock them and put them
  // back on the free list.  This way, wait() will return with an error rather
  // than block indefinitely.
  if (_firstWaitingThread != NULL)
  {
    notifyAllHandles();
  }

  if (_policy == POOLED)
  {
    // release the mutex if we have one, and put it back on the free list.
    if (_mutex != NULL)
    {
      ::unlockMutex(_mutex->_handle);
      _mutex->_next = _freeMutexList;
      _freeMutexList = _mutex;
      _mutex = NULL;
    }
    ::unlockMutex(_globalLock);
  }
  else
  {
    ::unlockMutex(_globalLock);
    ::deleteMutex(_mutex->_handle);
    delete _mutex;
  }
}

// lock
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::lock()
{
  // body
  VXIthreadID threadId = VXItrdThreadGetID();
  if (threadId == _ownerThread)
  {
    _lockCount++;
    return SUCCESS;
  }

  int rc = SUCCESS;
  if (_policy == POOLED)
  {
    if (::lockMutex(_globalLock) != SUCCESS)
      return FAILURE;

    bool newMutex = false;

    if (_mutex == NULL)
    {
      if (_freeMutexList == NULL)
      {
        _mutex = new HandleListItem;
        // We don't initialize the other fields as we don't really care
        _mutex->_handle = ::createMutex(true);
        newMutex = true;
        if (_mutex->_handle == NULL)
        {
          delete _mutex;
          _mutex = NULL;
        }
      }
      else
      {
        // Take a mutex from the free list.  This actually is a stack of mutex
        // as it is.  I don't think it causes a problem anyway.
        _mutex = _freeMutexList;
        _freeMutexList = _freeMutexList->_next;
      }
      _mutex->_count = 1;
    }
    else
    {
      _mutex->_count++;
    }

    rc = ::unlockMutex(_globalLock);
    if (rc != SUCCESS || _mutex == NULL)
    {
      return FAILURE;
    }

    if (!newMutex)
    {
      rc = ::lockMutex(_mutex->_handle);
    }
  }
  else
  {
    rc = ::lockMutex(_mutex->_handle);
  }

  if (rc == SUCCESS)
  {
    _ownerThread = threadId;
    _lockCount = 1;
  }

  return rc;
}

// unlock
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::unlock()
{
  VXIthreadID threadId = VXItrdThreadGetID();
  if (threadId != _ownerThread)
  {
    return NOT_OWNER;
  }

  int rc = SUCCESS;

  if (_policy == POOLED)
  {
    ::lockMutex(_globalLock);

    if (--_lockCount == 0)
    {
      _ownerThread = (VXIthreadID) -1;

      rc = ::unlockMutex(_mutex->_handle);

      if (--_mutex->_count == 0)
      {
        _mutex->_next = _freeMutexList;
        _freeMutexList = _mutex;
        _mutex = NULL;
      }
    }
    ::unlockMutex(_globalLock);
  }
  else
  {
    if (--_lockCount == 0)
    {
      _ownerThread = (VXIthreadID) -1;
      rc = ::unlockMutex(_mutex->_handle);
    }
  }

  return rc;
}

// wait
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::wait()
{
  return wait(INFINITE,NULL);
}

// wait
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::wait(unsigned long millisecs, bool *expiredF)
{
  // body
  VXIthreadID threadId = VXItrdThreadGetID();
  if (threadId != _ownerThread)
  {
    return NOT_OWNER;
  }

  int count = _lockCount;
  _ownerThread = (VXIthreadID) -1;
  _lockCount = 0;

  HandleListItem *listItem = getHandle();

  int rc = waitForEvent(_mutex->_handle, listItem->_handle,
                        millisecs, expiredF);

  resetHandle(listItem);

  if (rc == SUCCESS)
  {
    _ownerThread = threadId;
    _lockCount = count;
  }

  return rc;
}

// notify
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::notify()
{
  // body
  VXIthreadID threadId = VXItrdThreadGetID();
  if (threadId != _ownerThread)
  {
    return NOT_OWNER;
  }

  int rc = SUCCESS;

  ::lockMutex(_globalLock);
  if (_firstWaitingThread != NULL)
  {
#ifdef _WIN32
    rc = ::notifyEvent(_firstWaitingThread->_handle, false);

    // Remove the handle from the waiting threads list.
    HandleListItem *tmp = _firstWaitingThread;

    _firstWaitingThread = _firstWaitingThread->_next;
    tmp->_next = NULL;

    if (_firstWaitingThread == NULL)
      _lastWaitingThread = NULL;
    else
      _firstWaitingThread->_prev = NULL;
#else
  rc = ::notifyEvent(_firstWaitingThread->_handle, false);
#endif
  }

  ::unlockMutex(_globalLock);

  return rc;
}

// notifyAll
// Refer to SWItrdMonitor.hpp for doc.
int SWItrdMonitor::notifyAll()
{
  // body
  VXIthreadID threadId = VXItrdThreadGetID();
  if (threadId != _ownerThread)
  {
    return NOT_OWNER;
  }

  ::lockMutex(_globalLock);
  int rc = notifyAllHandles();
  ::unlockMutex(_globalLock);

  return rc;
}

int SWItrdMonitor::notifyAllHandles()
{
  int rc = SUCCESS;

#ifdef _WIN32
  while (_firstWaitingThread != NULL)
  {
    HandleListItem *tmp = _firstWaitingThread;
    _firstWaitingThread = _firstWaitingThread->_next;
    tmp->_next = tmp->_prev = NULL;
    if (notifyEvent(tmp->_handle, true) != SUCCESS)
      rc = FAILURE;
  }
  _lastWaitingThread = NULL;
#else
  if (_firstWaitingThread != NULL)
  {
    rc = notifyEvent(_firstWaitingThread->_handle, true);
  }
#endif
  return rc;
}

#ifdef _WIN32
SWItrdMonitor::HandleListItem *SWItrdMonitor::getHandle()
{
  HandleListItem *handle = NULL;

  ::lockMutex(_globalLock);

  if (_freeThreadList == NULL)
  {
    handle = new HandleListItem;
    // No need to initialize _owner, _next and _prev at this point.  They are
    // initialized below.
    HANDLE *k = new HANDLE;
    *k = CreateEvent(NULL, FALSE, FALSE, NULL);
    handle->_handle = k;
  }
  else
  {
    // Take the first NT event in the free list and make sure it is reset
    // Altough it should be.
    handle = _freeThreadList;
    _freeThreadList = handle->_next;
    ResetEvent(*(static_cast<HANDLE *>(handle->_handle)));
  }

  // Insert in list of pending notifications.
  if (_lastWaitingThread == NULL)
  {
    // empty list.
    _lastWaitingThread = _firstWaitingThread = handle;
    handle->_prev = handle->_next = NULL;
  }
  else
  {
    // add to the end of the list
    _lastWaitingThread->_next = handle;
    handle->_prev = _lastWaitingThread;
    handle->_next = NULL;
    _lastWaitingThread = handle;
  }

  ::unlockMutex(_globalLock);
  return handle;
}

void SWItrdMonitor::resetHandle(HandleListItem *handle)
{
  ::lockMutex(_globalLock);

  ResetEvent(*(static_cast<HANDLE *>(handle->_handle)));

  // Remove from the pending list.
  if (handle == _firstWaitingThread)
  {
    // The handle is the first one on the pending list.
    _firstWaitingThread = handle->_next;
    if (handle == _lastWaitingThread) _lastWaitingThread = NULL;
  }
  else if (handle == _lastWaitingThread)
  {
    // The handle is the last one on the pending list, but not the first.
    _lastWaitingThread = handle->_prev;
    _lastWaitingThread->_next = NULL;
  }
  else if (handle->_prev != NULL && handle->_next != NULL)
  {
    // The handle is neither the first one, nor the last one.
    handle->_prev->_next = handle->_next;
    handle->_next->_prev = handle->_prev;
  }

  // Now put back in the free list
  handle->_next = _freeThreadList;
  handle->_prev = NULL;
  _freeThreadList = handle;

  ::unlockMutex(_globalLock);
}

#else
SWItrdMonitor::HandleListItem *SWItrdMonitor::getHandle()
{
  HandleListItem *handle = NULL;

  ::lockMutex(_globalLock);

  if (_firstWaitingThread != NULL)
  {
    handle = _firstWaitingThread;
    handle->_count++;
  }
  else
  {
    if (_freeThreadList == NULL)
    {
      handle = new HandleListItem;
      // No need to initialize _owner, _next and _prev at this point.  They are
      // initialized below.
      pthread_cond_t *p = new pthread_cond_t;
      pthread_cond_init(p, NULL);
      handle->_handle = p;
    }
    else
    {
      // Take the first event in the free list.
      // Altough it should be.
      handle = _freeThreadList;
      _freeThreadList = handle->_next;
    }

    _lastWaitingThread = _firstWaitingThread = handle;
    handle->_prev = handle->_next = NULL;
    handle->_count = 1;
  }

  ::unlockMutex(_globalLock);
  return handle;
}

void SWItrdMonitor::resetHandle(HandleListItem *handle)
{
  ::lockMutex(_globalLock);

  // Remove from the pending list if not already removed.
  if (--handle->_count == 0)
  {
    _firstWaitingThread = _lastWaitingThread = NULL;

    // Now put back in the free list
    handle->_next = _freeThreadList;
    handle->_prev = NULL;
    _freeThreadList = handle;
  }

  ::unlockMutex(_globalLock);
}


#endif // _WIN32
