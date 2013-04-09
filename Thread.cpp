//
//  thread.cpp
//
//  Created by Mickey Kawick on 03Apr13.
//  Copyright (c) 2013 Playdek games. All rights reserved.
//

#include "StdAfx.h"
#include "Platform.h"
#if PLATFORM == PLATFORM_WINDOWS
#else
#endif

#include <assert.h>
#include <deque>
#include "Thread.h"
using namespace std;
using namespace Threading;

const int timeoutUponDeleteMs = 1000;
const int mutexTimeout = 1000;

//////////////////////////////////////////////////////////////////////////////////////////////

Mutex::Mutex() :
      m_isLocked( false ),
	  m_pendingLockReqs( 0 )
{
#if PLATFORM == PLATFORM_WINDOWS
   m_mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
#else
   pthread_mutex_init( &m_mutex, NULL );
#endif
}

//----------------------------------------------------------------

Mutex::~Mutex()
{
   // WAIT_OBJECT_0
   // WAIT_ABANDONED
#if PLATFORM == PLATFORM_WINDOWS
   DWORD dwWaitResult = WaitForSingleObject(
                                            m_mutex,
                                            timeoutUponDeleteMs);  // time-out interval
   CloseHandle( m_mutex );
#else
   //pthread_join( m)
   pthread_mutex_destroy( &m_mutex );
#endif
}

//----------------------------------------------------------------

bool  Mutex::lock()
{
   m_pendingLockReqs++;
#if PLATFORM == PLATFORM_WINDOWS
   DWORD dwWaitResult = WaitForSingleObject( m_mutex, mutexTimeout );
   // many error conditions including timeout.
#else
   pthread_mutex_lock( &m_mutex );
#endif
   m_isLocked = true;

   return true;
}

//----------------------------------------------------------------

bool  Mutex::unlock()
{
   m_isLocked = false;
   m_pendingLockReqs--;

#if PLATFORM == PLATFORM_WINDOWS
   if( ReleaseMutex( m_mutex ) )
      return true;
#else
   pthread_mutex_unlock( &m_mutex );
#endif

   return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////

CAbstractThread::CAbstractThread( bool needsThreadProtections, int sleepTime, bool paused ) : 
                  m_thread( NULL ),
                  m_needsThreadProtection( needsThreadProtections ), 
                  m_sleepTime( sleepTime ),
                  m_running( false ),
                  m_markedForCleanup( false ),
                  m_isPaused( paused )
{
#if PLATFORM == PLATFORM_WINDOWS
   m_threadId = 0;
#endif
   
   CreateThread();
}

//----------------------------------------------------------------

CAbstractThread::~CAbstractThread() {}

//----------------------------------------------------------------

void  CAbstractThread::Cleanup()
{
   CallbackOnCleanup();
   DestroyThread();
   
#ifndef WIN32
   void* result;
   pthread_join( m_thread, &result );
#endif
}

//----------------------------------------------------------------

void  CAbstractThread::SetPriority( ePriority priority )
{
#if PLATFORM == PLATFORM_WINDOWS
   int winPriority = THREAD_PRIORITY_NORMAL;
#else
   sched_param param;
   param.sched_priority = ( sched_get_priority_max( SCHED_OTHER ) + sched_get_priority_min( SCHED_OTHER ) ) /2;
#endif
   
   switch( priority )
   {
   case ePriorityLow:
#if PLATFORM == PLATFORM_WINDOWS
      winPriority = THREAD_PRIORITY_BELOW_NORMAL;
#else
      param.sched_priority = sched_get_priority_min( SCHED_OTHER );
      
#endif
      break;

   case ePriorityNormal:
      break;

   case ePriorityHigh:
#if PLATFORM == PLATFORM_WINDOWS
      winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
#else
      param.sched_priority = sched_get_priority_max( SCHED_OTHER );
#endif
      break;

   default:
      assert( 0 );
   }
   
#if PLATFORM == PLATFORM_WINDOWS
   SetThreadPriority( m_thread, winPriority );
#else
   //sched_param param;
   pthread_setschedparam( m_thread, SCHED_OTHER, &param );
#endif
}

//----------------------------------------------------------------

int CAbstractThread::CreateThread()
{
   int threadError = 0;
#ifndef WIN32
   pthread_attr_init( &m_attr );
	pthread_attr_setdetachstate( &m_attr, PTHREAD_CREATE_DETACHED );
	threadError = pthread_create( &m_thread, &m_attr, &CAbstractThread::ThreadFunction, (void*)this );
   if( threadError == 0 )
      m_running = true;
#else
   m_threadId = 0;
   m_thread = ::CreateThread( 
            NULL,                               // default security attributes
            0,                                  // use default stack size  
            &CAbstractThread::ThreadFunction,   // thread function name
            this,				                     // argument to thread function 
            0,                                  // use default creation flags 
            &m_threadId);				            // returns the thread identifier 
   if( m_thread == NULL )
      threadError = 1;
   else
      m_running = true;
#endif
   return threadError;
}

//----------------------------------------------------------------

int CAbstractThread::DestroyThread()
{
   if( m_running == false )
      return false;

   m_running = false;
   m_markedForCleanup = true;

   return true;
}

//----------------------------------------------------------------

#if PLATFORM == PLATFORM_WINDOWS
DWORD    CAbstractThread::ThreadFunction( void* data )
#else
void*    CAbstractThread::ThreadFunction( void* data )
#endif
{
   CAbstractThread* thread = reinterpret_cast< CAbstractThread* > ( data );

   // before a thread can start, we need to give the owner a little time to finish creating. 
   // in cases where we have 100 threads or more, the creation can take a while and the threads
   // can be scheduled in such a way so that this threadFunction is invoked before creation
   // completes which leads to a crash.
   const int defaultSleepTime = 50;
    
   Sleep( defaultSleepTime );

   while( thread->m_running )
   {
      if( thread->m_isPaused == false )// TODO, should I sleep if not active and how log if so? What if the SleepTime is not set.
      {
         if( thread->m_needsThreadProtection )
         {
            thread->m_mutex.lock();
         }

         if( thread->m_isPaused == false &&
            thread->m_running == true &&
            thread->m_markedForCleanup == false ) // don't do this work if we are about to exit
         {
            thread->CallbackFunction();
         }

         if( thread->m_needsThreadProtection )
         {
            thread->m_mutex.unlock();
         }
      }

      if( thread->m_sleepTime )
      {
         Sleep( thread->m_sleepTime );
      }
   }

   if( thread->m_markedForCleanup )
   {
#if PLATFORM == PLATFORM_WINDOWS
      DWORD dwWaitResult = WaitForSingleObject(  thread->m_thread, timeoutUponDeleteMs );
      CloseHandle( thread->m_thread );
#else
      pthread_cancel( thread->m_thread );
#endif
   }

   while( thread->m_mutex.NumPendingLockRequests() > 0 )
	   Sleep( 200 );
   delete thread;

#if PLATFORM == PLATFORM_WINDOWS
   return 0;
#else
   pthread_exit( 0 );
   return NULL;
#endif
}




