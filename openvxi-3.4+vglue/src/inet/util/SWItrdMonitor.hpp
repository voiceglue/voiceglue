#ifndef SWITRDMONITOR_HPP
#define SWITRDMONITOR_HPP

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

#include "SWIutilHeaderPrefix.h"

#include "VXItypes.h"

/**
 * Class encapsulating thread monitor.  Thread monitors can be locked,
 * unlocked, can be waited on and can be notified.  Monitors implement
 * so-called recursive locking, meaning that a thread owning the monitor can
 * call lock without blocking and will have to call unlock() as many time
 * lock() was called.
 *
 * @doc <p>
 **/

class SWIUTIL_API_CLASS SWItrdMonitor
{
 public:
  enum Policy
  {
    /*
     * Indicates that mutex used by SWItrdMonitor is taken from a pool so that
     * different SWItrdMonitor can reuse the same OS mutex.
     */
    POOLED = 0,

    /*
     * Indicates that mutex used by SWItrdMonitor are dedicated to that
     * SWItrdMonitor and cannot be reused.
     */
    DEDICATED = 1
  };

  /**
   * Status indicating success of an operation.
   **/
 public:
  static const int SUCCESS;

  /**
   * Status indicating failure of an operation.
   **/
 public:
  static const int FAILURE;

  /**
   * Status indicating failure of an operation because the current thread does
   * not own the monitor.
   **/
  static const int NOT_OWNER;

  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SWItrdMonitor(Policy policy = POOLED);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWItrdMonitor();

  /**
   * Locks the monitor.
   **/
 public:
  int lock();

  /**
   * Unlocks the monitor.
   **/
 public:
  int unlock();

  /**
   * Causes current thread to wait until another thread invokes the
   * <code>notify()</code> method or the
   * <code>notifyAll()</code> method for this monitor.
   * <p>
   * The current thread must own this monitor. The thread
   * releases ownership of this monitor and waits until another thread
   * notifies threads waiting on this object's monitor to wake up
   * either through a call to the <code>notify</code> method or the
   * <code>notifyAll</code> method. The thread then waits until it can
   * re-obtain ownership of the monitor and resumes execution.
   * <p>
   **/
 public:
  int wait();

  /**
   * Causes current thread to wait until either another thread invokes the
   * <code>notify()</code> method or the
   * <code>notifyAll()</code> method for this monitor, or a
   * specified amount of time has elapsed.
   **/
 public:
  int wait(unsigned long millisecs, bool *expiredF = 0);

  /**
   * Wakes up a single thread that is waiting on this
   * monitor. If more than one thread are waiting on this object, one of them
   * is arbitrarily chosen to be awakened. A thread waits on the
   * monitor by calling one of the <code>wait</code> methods.
   *
   * <p>
   * The awakened thread will not be able to proceed until the current
   * thread relinquishes the lock on this object. The awakened thread will
   * compete in the usual manner with any other threads that might be
   * actively competing to synchronize on this object; for example, the
   * awakened thread enjoys no reliable privilege or disadvantage in being
   * the next thread to lock this monitor.
   *
   * <p>
   * This method should only be called by a thread that is the owner
   * of this monitor.
   **/
 public:
  int notify();

  /**
   * Wakes up all threads that are waiting on this monitor. A
   * thread waits on a monitor by calling one of the
   * <code>wait</code> methods.
   *
   * <p>
   * The awakened threads will not be able to proceed until the current
   * thread relinquishes the monitor. The awakened threads
   * will compete in the usual manner with any other threads that might
   * be actively competing to synchronize on this monitor; for example,
   * the awakened threads enjoy no reliable privilege or disadvantage in
   * being the next thread to lock this object.
   * <p>
   * This method should only be called by a thread that is the owner
   * of this object's monitor.
   **/
   public:
  int notifyAll();

  /**
    * Diabled copy constructor.
   **/
 private:
  SWItrdMonitor(const SWItrdMonitor&);

  /**
    * Disabled assignment operator.
   **/
 private:
  SWItrdMonitor& operator=(const SWItrdMonitor&);

 private:
  struct HandleListItem
  {
    void* _handle;
    HandleListItem * volatile _next;
    HandleListItem * volatile _prev;
    int _count;
  };

  volatile VXIthreadID _ownerThread;
  volatile int _lockCount;

  // List of free events that can be used for wait/notifcation purpose.
  static HandleListItem * volatile _freeThreadList;

  // Mutex used to handle modification to the freeLists.
  static void * _globalLock;

  HandleListItem * volatile _firstWaitingThread;
  HandleListItem * volatile _lastWaitingThread;

  HandleListItem *getHandle();
  void resetHandle(HandleListItem *);
  int notifyAllHandles();
  static HandleListItem * volatile _freeMutexList;
  HandleListItem* _mutex;
  Policy _policy;
};

#endif
