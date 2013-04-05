//
//  thread.cpp
//
//  Created by Mickey Kawick on 03Apr13.
//  Copyright (c) 2013 Playdek games. All rights reserved.
//

#include "StdAfx.h"
#include <assert.h>

#include "Thread.h"


const int timeoutUponDeleteMs = 1000;
const int mutexTimeout = 1000;

//////////////////////////////////////////////////////////////////////////////////////////////

Mutex::Mutex() :
      m_isLocked( false )
{
#ifdef _WIN32
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
   DWORD dwWaitResult = WaitForSingleObject( 
            m_mutex,    
            timeoutUponDeleteMs);  // time-out interval

   // WAIT_OBJECT_0
   // WAIT_ABANDONED
#ifdef _WIN32
   CloseHandle( m_mutex );
#else
   pthread_mutex_destroy( &m_mutex );
#endif
}

//----------------------------------------------------------------

bool  Mutex::lock()
{
#ifdef _WIN32
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

#ifdef _WIN32
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
                  m_threadId( 0 ), 
                  m_needsThreadProtection( needsThreadProtections ), 
                  m_sleepTime( sleepTime ),
                  m_running( false ),
                  m_markedForCleanup( false ),
                  m_isPaused( paused )
{
   CreateThread();
}

//----------------------------------------------------------------

CAbstractThread::~CAbstractThread() {}

void  CAbstractThread::Cleanup()
{
   CallbackOnCleanup();
   DestroyThread();
}

//----------------------------------------------------------------

void  CAbstractThread::SetPriority( ePriority priority )
{
   switch( priority )
   {
   case ePriorityLow:
#ifdef _WIN32
      SetThreadPriority( m_thread, THREAD_PRIORITY_BELOW_NORMAL );
#else
      sched_param param;
      param.sched_priority = PRIORITY_MIN;
      pthread_setschedparam( SCHED_OTHER, param );
#endif
      break;

   case ePriorityNormal:
#ifdef _WIN32
      SetThreadPriority( m_thread, THREAD_PRIORITY_NORMAL );
#else
      sched_param param;
      param.sched_priority = ( PRIORITY_MAX + PRIORITY_MIN ) /2;
      pthread_setschedparam( SCHED_OTHER, param );
#endif
      break;

   case ePriorityHigh:
#ifdef _WIN32
      SetThreadPriority( m_thread, THREAD_PRIORITY_ABOVE_NORMAL );
#else
      sched_param param;
      param.sched_priority = PRIORITY_MAX;
      pthread_setschedparam( SCHED_OTHER, param );
#endif
      break;

   default:
      assert( 0 );
   }
}

//----------------------------------------------------------------

int CAbstractThread::CreateThread()
{
   int threadError = 0;
#ifndef _WIN32
   pthread_attr_init( &m_attr );
	pthread_attr_setdetachstate( &m_attr, PTHREAD_CREATE_DETACHED );
	threadError = pthread_create( &m_threadId, &m_attr, &CAbstractThread::ThreadFunction, this );
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

#ifdef _WIN32
DWORD    CAbstractThread::ThreadFunction( void* data )
#else
void     CAbstractThread::ThreadFunction( void* data )
#endif
{
   CAbstractThread* thread = reinterpret_cast< CAbstractThread* > ( data );

   // before a thread can start, we need to give the owner a little time to finish creating. 
   // in cases where we have 100 threads or more, the creation can take a while and the threads
   // can be scheduled in such a way so that this threadFunction is invoked before creation
   // completes which leads to a crash.
   const int defaultSleepTime = 50;
#ifdef _WIN32       
         Sleep( defaultSleepTime );
#else
         sleep( (float)(defaultSleepTime) * 0.001 );
#endif
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
#ifdef _WIN32       
         Sleep( thread->m_sleepTime );
#else
         sleep( (float)(thread->m_sleepTime) * 0.001 );
#endif
      }
   }

   if( thread->m_markedForCleanup )
   {
#ifdef _WIN32
      DWORD dwWaitResult = WaitForSingleObject(  thread->m_thread, timeoutUponDeleteMs );
      CloseHandle( thread->m_thread );
#else
      int ret = pthread_cancel( thread->m_threadId );
#endif
   }

   delete thread;

#ifdef _WIN32
   return 0;
#endif
}




