#pragma once // replaced the compile guards, this can be faster in VS2010 or earlier


#ifdef _WIN32

#include <windows.h>
typedef HANDLE             ThreadMutex;
typedef HANDLE             ThreadId;

#else

#define _MULTI_THREADED
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

typedef pthread_mutex_t    ThreadMutex;
typedef pthread_t          ThreadId;
typedef pthread_attr_t     ThreadAttributes;
#define Sleep(a)           usleep((float)(a) * 1000)

#endif

#include <list>

/////////////////////////////////////////////////////////////////////////////

class Mutex
{
public:
   Mutex();
   ~Mutex();
   bool  lock();
   bool  unlock();

   bool  IsLocked() const { return m_isLocked; }
   int	 NumPendingLockRequests() const { return m_pendingLockReqs; }

private:
   ThreadMutex    m_mutex;
   bool           m_isLocked;
   int            m_pendingLockReqs;
};

class MutexLock
{
public:
	explicit MutexLock( Mutex& mutex ): m_mutex( mutex ) { m_mutex.lock(); }
	~MutexLock() { m_mutex.unlock(); }

	Mutex& m_mutex;
};

/////////////////////////////////////////////////////////////////////////////

// typical use case
/*
   class Thread01 : public CAbstractThread
   {
   public:
      Thread01( int timeOut ) : CAbstractThread( true, timeOut ), m_count( 0 ) {}

      int CallbackFunction()
      {
         m_count ++;
         cout << "Running " << m_count << endl;
         return 0;
      }
      int GetCount() const { return m_count; }

   private:
      ~Thread01() {}

   private:
      int m_count;
   };


   bool  RunCounterThread()
   {
      cout << "Count test start" << endl;
      Thread01* thread = new Thread01( 100 );
      Sleep(20000);
      cout << "Count test end" << endl;

      int num = thread->GetCount();
      thread->Cleanup();

      if( num > 195 ) return true;// it is threaded but should count at least to 195
      return false;
   }
*/
/////////////////////////////////////////////////////////////////////////////

class CAbstractThread
{
public:
   enum ePriority
   {
      ePriorityLow, 
      ePriorityNormal, 
      ePriorityHigh
   };

public:
	CAbstractThread( bool needsThreadProtection = false, int sleepTime = 0, bool paused = false );
   void  Cleanup(); // invoke this instead of the d'tor

   void              SetPriority( ePriority );
   bool              IsRunning() const { return m_running; }
   bool              IsThreadLocked() const { return m_mutex.IsLocked(); }
   int				 GetSleepTime() const { return m_sleepTime; }

   void              Pause() { m_isPaused = true; }
   void              Resume() { m_isPaused = false; }

   // inherited classes must provide ths function
   virtual int       CallbackFunction() = 0;

   //------------------------------------------------------
protected:
   // allowing constness to be maintained with mutable
   mutable Mutex     m_mutex;
   void LockMutex() const { m_mutex.lock(); }
   void UnlockMutex() const { m_mutex.unlock(); }
   bool              m_isPaused;

protected:

   // prevent people from destroying directly. Call Cleanup instead.
   virtual ~CAbstractThread();
   virtual void      CallbackOnCleanup() {}

private:

   bool              m_running;
   bool              m_markedForCleanup;
   bool              m_needsThreadProtection;
   int               m_sleepTime;

   ThreadId          m_thread;
   
   //---------------------------

   int               CreateThread();
   int               DestroyThread();

#ifdef _WIN32
   DWORD             m_threadId;
   static DWORD  WINAPI  ThreadFunction( void* data );
#else
   pthread_attr_t    m_attr;
   static void*      ThreadFunction( void* data );
#endif
};

/////////////////////////////////////////////////////////////////////////////

   // this is a strange feature, but while rare, the importance of providing a clean
   // mechanism for allowing threads to chain and unchain correctly became obvious during test
template <typename Type> 
class ChainedInterface
{
public:
   void  AddInputChain( ChainedInterface*, bool recurse = true );
   void  RemoveInputChain( ChainedInterface*, bool recurse = true );
   void  AddOutputChain( ChainedInterface*, bool recurse = true );
   void  RemoveOutputChain( ChainedInterface*, bool recurse = true );

   // TODO: convert this to a const reference instead
   virtual bool AcceptChainData( Type t ) { return false; }// a false value means that the data was rejected

protected:
	struct OutputChain
	{
		OutputChain() : m_interface( NULL ) {}
		OutputChain( ChainedInterface* obj ) : m_interface( obj ) {}

		void	AddData( Type t) { m_data.push_back( t ); }
		Type	RemoveData() { Type t = m_data.front(); m_data.pop_front(); return t; }

		ChainedInterface*	m_interface;
		std::deque<Type>	m_data;
	};
protected:
   Mutex       m_inputChainListMutex;
   Mutex       m_outputChainListMutex;

   std::list< ChainedInterface* >	m_listOfInputs;// threads
   std::list< OutputChain >			m_listOfOutputs;// threads

   void  CleanupAllChainDependencies();
   	
   typedef std::list< ChainedInterface* >		base_container;
   typedef typename base_container::iterator	ChainedIteratorType;

   typedef std::list< OutputChain >						base_output_container;
   typedef typename base_output_container::iterator		ChainedOutputIteratorType;
};

/////////////////////////////////////////////////////////////////////////////

template <typename Type> 
class CChainedThread : public CAbstractThread, public ChainedInterface <Type>
{
public:
   // consider leaving the need protection false. This should be managed in the 
	CChainedThread( bool needsThreadProtection = false, int sleepTime = 0, bool paused = false );


   // inherited classes must provide ths function. Mutexes will be locked
   virtual int       ProcessInputFunction() { return 0; };
   virtual int       ProcessOutputFunction() { return 0; };
   

   //------------------------------------------------------
protected:

   virtual void      CallbackOnCleanup();

   // consider not overriding this function. You should achieve the results you need by 
   // overriding the ProcessInputFunction and ProcessOutputFunction functions
   virtual int       CallbackFunction();
   
};

/////////////////////////////////////////////////////////////////////////////
#include "Thread.inl"
/////////////////////////////////////////////////////////////////////////////
