// test_thread01.cpp : Defines the entry point for the console application.
//

#include "StdAfx.h"
#if defined(_WIN32)
	#include <conio.h>
#else
	#include <curses.h>
#endif

#include <iostream>
#include <list>
#include <deque>
using namespace std;

#include "thread.h"
#include "MultipleFeedsIntoOneThread.h"
using namespace Threading;

#pragma warning ( disable: 4996 )

//-----------------------------------------------
//-----------------------------------------------

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

//-----------------------------------------------

class ThreadSharedLinkedList : public CAbstractThread
{
public:
   ThreadSharedLinkedList( int timeOut ) : CAbstractThread( true, timeOut ) {}

   int GetSize() 
   {
      m_mutex.lock();
      return static_cast<int>( m_listOfInts.size() );
      m_mutex.unlock();
   }
protected:
   static list< int > m_listOfInts;

protected:
   ~ThreadSharedLinkedList() {}
};

list< int > ThreadSharedLinkedList::m_listOfInts; // static member variable

class ThreadInsert : public ThreadSharedLinkedList
{
public:
   ThreadInsert () : ThreadSharedLinkedList( 100 )
   {
   }

   int CallbackFunction()
   {
      //int numToAdd = rand() % 22 + 2;

      int num = static_cast<int>( m_listOfInts.size() );
      for( int i=0; i < num; i++ )
      {
         m_listOfInts.push_back( rand() %10 + 1);
      }
      
      return 0;
   }
};

class ThreadRemove : public ThreadSharedLinkedList
{
public:
   ThreadRemove () : ThreadSharedLinkedList( 150 )
   {
   }

   int CallbackFunction()
   {
      //int numToCull = rand() % 22 + 2;

      int num = static_cast<int>( m_listOfInts.size() );
      for( int i=0; i < num; i++ )
      {
         m_listOfInts.pop_front();
      }
      
      return 0;
   }
};

//-----------------------------------------------

// note, we are not hooking backwards
class ChainViewThread : public CChainedThread< int >
{
public:
   ChainViewThread( int timeOut ) : CChainedThread( true, timeOut ){}

   int CallbackFunction()
   {
      cout<< " ChainViewThread "  << endl;
      m_items.clear();
      return 1;
   }
   bool AddInputChainData( int value )
   {
	  if( m_mutex.IsLocked() == true ) 
		  return false;

      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

	  return true;
   }

   int GetNumItems() const
   {
      int numItems = 0;
      LockMutex();
      numItems = static_cast<int>( m_items.size() );
      UnlockMutex();

      return numItems;
   }

private:
   ~ChainViewThread() {}

private:
   list< int > m_items;
};

class ChainMiddleSupplyThread : public CChainedThread< int >
{
public:
   ChainMiddleSupplyThread( int timeOut ) : CChainedThread( false, timeOut ){}

   int       ProcessInputFunction() 
   { 
    /*  LockMutex();
      std::list< int > ::iterator it = m_items.begin();
      while( it != m_items.end() )
      {
      }
      m_items.clear();
      UnlockMutex();*/
      return 0; 
   }

   int       ProcessOutputFunction() 
   { 
     // cout<< "· ChainMiddleSupplyThread::ProcessOutputFunction ·"  << endl;
      LockMutex();

         //int num = m_item.size();
         ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
         while( itOutputs != m_listOfOutputs.end() )
         {
            ChainLink& chainedOutput = *itOutputs++;
		      ChainedInterface* chainedInterface = chainedOutput.m_interface;
            std::list< int > ::iterator it = m_items.begin();
            while( it != m_items.end() )
            {
               chainedInterface->AddInputChainData( *it++ );
            }
         }
         m_items.clear();

      UnlockMutex();
      return 0; 
   }

   bool AddInputChainData( int value )
   {
      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

	  return true;
   }
   int GetNumItems() const
   {
      int num = 0;
      LockMutex();
      num = static_cast<int>( m_items.size() );
      UnlockMutex();

      return num;
   }

private:
   ~ChainMiddleSupplyThread() {}

private:
   list< int > m_items;
};


class ChainSupplyThread : public CChainedThread< int >
{
public:
   ChainSupplyThread( int timeOut ) : CChainedThread( false, timeOut ){}


   int       ProcessOutputFunction() 
   { 
      //cout<< "· ChainSupplyThread::ProcessOutputFunction ·"  << endl;
      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs++;
		   ChainedInterface* chainedInterface = chainedOutput.m_interface;
         for( int i=0; i< 10; i++ )
         {
            chainedInterface->AddInputChainData( rand() % 25 + 1 );
         }
      }
      return 0; 
   }

private:
   ~ChainSupplyThread() {}
};

//-----------------------------------------------

// note, we are not hooking backwards
class ChainViewCircularThread : public CChainedThread< int >
{
public:
   ChainViewCircularThread( int timeOut ) : CChainedThread( true, timeOut ){}

   int       ProcessOutputFunction() 
   { 
      if( m_items.size() )
      {
         LockMutex();
            ChainLinkIteratorType itInputs = m_listOfInputs.begin();
            while( itInputs != m_listOfInputs.end() )
            {
               ChainLink& chainedOutput = *itInputs++;
               ChainedInterface* chainedInterface = chainedOutput.m_interface;
               std::list< int > ::iterator it = m_items.begin();
               while( it != m_items.end() )
               {
                  chainedInterface->AddOutputChainData( *it++ );
               }
            }
            m_items.clear();
         UnlockMutex();
      }
      return 1;
   }

   bool AddInputChainData( int value )
   {
	  if( m_mutex.IsLocked() == true ) 
		  return false;

      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

	  return true;
   }

   int GetNumItems() const
   {
      int num = 0;
      LockMutex();
      num = static_cast<int>( m_items.size() );
      UnlockMutex();
      return num;
   }

private:
   ~ChainViewCircularThread() {}

private:
   list< int > m_items;
};

class ChainMiddleSupplyCircularThread : public CChainedThread< int >
{
public:
   ChainMiddleSupplyCircularThread( int timeOut ) : CChainedThread( false, timeOut ){}

   int       ProcessInputFunction() 
   { 
      return 0; 
   }

   int       ProcessOutputFunction() 
   { 
      LockMutex();

         if( m_items.size() )
         {
            ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
            while( itOutputs != m_listOfOutputs.end() )
            {
               ChainLink& chainedOutput = *itOutputs++;
		         ChainedInterface* chainedInterface = chainedOutput.m_interface;
               std::list< int > ::iterator it = m_items.begin();
               while( it != m_items.end() )
               {
                  chainedInterface->AddInputChainData( *it++ );
               }
            }
            m_items.clear();
         }
         if( m_itemsOut.size() )
         {
            ChainLinkIteratorType itInputs = m_listOfInputs.begin();
            while( itInputs != m_listOfInputs.end() )
            {
               ChainLink& chainedOutput = *itInputs++;
		         ChainedInterface* chainedInterface = chainedOutput.m_interface;
               std::list< int > ::iterator it = m_itemsOut.begin();
               while( it != m_itemsOut.end() )
               {
                  chainedInterface->AddOutputChainData( *it++ );
               }
            }
            m_itemsOut.clear();
         }

      UnlockMutex();
      return 0; 
   }

   bool AddInputChainData( int value )
   {
      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

	  return true;
   }

   bool AddOutputChainData( int t )
   {
      LockMutex();
      m_itemsOut.push_back( t );
      UnlockMutex();

	   return true;
   }

   int GetNumItems() const
   {
      int num = 0;
      LockMutex();
      num = static_cast<int>( m_items.size() );
      UnlockMutex();
      return num;
   }

private:
   ~ChainMiddleSupplyCircularThread() {}

private:
   list< int > m_items;
   list< int > m_itemsOut;
};


class ChainSupplyCircularThread : public CChainedThread< int >
{
public:
   ChainSupplyCircularThread( int timeOut, int numToSupply ) : 
      CChainedThread( false, timeOut ), 
      m_numToSupply( numToSupply ), 
      m_hasSent( false ){}

   int       ProcessOutputFunction() 
   {
      if( m_hasSent == true )
         return 0;

      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs++;
		   ChainedInterface* chainedInterface = chainedOutput.m_interface;
         for( int i=0; i< m_numToSupply; i++ )
         {
            int value = rand() % 25 + 1;
            chainedInterface->AddInputChainData( value );
            m_itemsSent.push_back( value );
         }
      }
      m_hasSent = true;
      return 0; 
   }

   bool AddOutputChainData( int t )
   {
      LockMutex();
      m_itemsReturned.push_back( t );
      UnlockMutex();
      return true;
   }

   bool  AreQueuesEqual() const 
   {
      int num = m_itemsReturned.size();// guaranteed to be smaller than the items sent
      for( int i=0; i< num; i++ )
      {
         if( m_itemsReturned[i] != m_itemsSent[i] )
            return false;
      }
      return true;
   }

private:
   ~ChainSupplyCircularThread() {}

   bool  m_hasSent;
   int   m_numToSupply;
   deque< int > m_itemsSent;
   deque< int > m_itemsReturned;
};
//-----------------------------------------------

class ChainObserverThread : public CChainedThread< int >
{
public:
   ChainObserverThread( int timeOut ) : 
      CChainedThread( true, timeOut ), 
      m_isDataAvailable( false ), 
      m_numItemsProcessed( 0 ){}


   bool  PushInputEvent( Threading::ThreadEvent* te )
   {
      switch( te->type )
      {
      case Threading::ThreadEvent_DataAvailable:
         int id = te->identifier;
         m_isDataAvailable = true;
         delete te;
         return true;
      }

      return false;
   }
   void  ProcessEvents()
   {
      if( m_isDataAvailable == false )
         return;

     /* ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs++;
		   ChainedInterface* chainedInterface = chainedOutput.m_interface;
      }*/
      LockMutex();
      m_numItemsProcessed = m_items.size();
      UnlockMutex();
      m_isDataAvailable = false;

      CChainedThread::ProcessEvents();
   }

   bool  AddInputChainData( int value )
   {
	  if( m_mutex.IsLocked() == true ) 
		  return false;

      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

	  return true;
   }

   int GetNumItemsProcessed() const
   {
      return m_numItemsProcessed;
   }

private:
   ~ChainObserverThread() {}

private:
   list< int > m_items;
   int   m_numItemsProcessed;
   bool  m_isDataAvailable;
};

class ChainControllerThread : public CChainedThread< int >
{
public:
   ChainControllerThread( int timeOut ) : 
      CChainedThread( false, timeOut ), 
      m_needsToSend( true ){}


   int       ProcessOutputFunction() 
   { 
      //cout<< "· ChainSupplyThread::ProcessOutputFunction ·"  << endl;
      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs++;
		   ChainedInterface* chainedInterface = chainedOutput.m_interface;
         bool sent = false;
         for( int i=0; i< 10; i++ )
         {
            sent |= chainedInterface->AddInputChainData( rand() % 25 + 1 );
         }

         if( sent && m_needsToSend == true )
         {
            ThreadEvent* event = new ThreadEvent();
            event->type = ThreadEvent_DataAvailable;
            event->identifier = 0xff;
            chainedInterface->PushInputEvent( event );
            m_needsToSend = false;
         }
      }
      return 0; 
   }

private:
   ~ChainControllerThread() {}

   bool  m_needsToSend;
};

//-----------------------------------------------
//-----------------------------------------------

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

bool  RunInsertThread()
{
   cout << "Insert test start" << endl;
   ThreadInsert* threadInsert = NULL;
   ThreadRemove* threadRemove = NULL;
   bool errorState = false;

   try 
   {
      threadInsert = new ThreadInsert();
      threadRemove = new ThreadRemove();
      Sleep(10000);
      cout << "Num in list = " << threadInsert->GetSize() << endl;
      Sleep(5000);
      cout << "Num in list = " << threadInsert->GetSize() << endl;
      threadInsert->Cleanup();
      threadRemove->Cleanup();
      if( threadInsert->GetSize() > 0 )
         errorState = false;
      else
         errorState = true;
   } 
   catch( ... )
   {
      cout << "error thrown" << endl;
      errorState = false;
   }

   cout << "Insert test end" << endl;
   return errorState;
}

int  RunChainedThreads01Test()
{
   cout << "Chained thread test start" << endl;
   ChainViewThread* threadView = new ChainViewThread( 50 );
   ChainSupplyThread* threadSupply = new ChainSupplyThread( 10 );

   threadSupply->AddOutputChain( threadView );
   
   Sleep( 12000 );
   threadSupply->Pause();
   threadView->Pause();

   cout << "End number of items = " << threadView->GetNumItems() << endl;

   threadSupply->Cleanup();
   threadView->Cleanup();

   cout << "Chained thread test end" << endl;
   return true;
}

int  RunChainedThreads02Test()// simple chain test
{
   cout << "Chained thread 02 test start" << endl;
   ChainViewThread* threadView = new ChainViewThread( 50 );
   ChainMiddleSupplyThread* threadMiddleSupply = new ChainMiddleSupplyThread( 20 );
   ChainSupplyThread* threadSupply = new ChainSupplyThread( 10 );

   threadView->AddInputChain( threadMiddleSupply );
   threadSupply->AddOutputChain( threadMiddleSupply );
   
   Sleep( 12000 );
   threadSupply->Pause();
   threadMiddleSupply->Pause();
   threadView->Pause();

   cout << "End number of items in middle = " << threadMiddleSupply->GetNumItems() << endl;
   cout << "End number of items at end = " << threadView->GetNumItems() << endl;

   //Sleep( 2000 );
   threadSupply->Cleanup();
   threadMiddleSupply->Cleanup();
   threadView->Cleanup();

   cout << "Chained thread 02 test end" << endl;
   return true;
}

int  RunChainedThreads03Test()// cleanup test
{
   cout << "Chained thread 03 test start" << endl;
   ChainViewThread* threadView = new ChainViewThread( 50 );
   ChainMiddleSupplyThread* threadMiddleSupply = new ChainMiddleSupplyThread( 20 );
   ChainSupplyThread* threadSupply = new ChainSupplyThread( 100 );

   threadView->AddInputChain( threadMiddleSupply );
   threadSupply->AddOutputChain( threadMiddleSupply );
   
   Sleep( 10000 );
   threadMiddleSupply->Cleanup();
   Sleep( 2000 );

   threadSupply->Pause();
   threadView->Pause();

   //cout << "End number of items in middle = " << threadMiddleSupply->GetNumItems() << endl;
   cout << "End number of items at end = " << threadView->GetNumItems() << endl;

   threadSupply->Cleanup();
   threadView->Cleanup();

   cout << "Chained thread 03 test end" << endl;
   return true;
}

// chaining one thread feeding another.
// chaining one thread feeding another feeding to another.
// making sure that cleanup is out of order
// setup 3-chain, run for a while, then remove middle node
// multiple feed. create 300 children feeding into one parent. That parent should feed into another chained thread class.
//     Children should have random periodicity of 100-3000 ms. at the end, we should have a count of 1000's of messages.
//     be sure to cleanup a few children and make sure all runs well for a while. measure wait times per child. 
//     Clean up final parent first and see how it goes.

//-----------------------------------------------

bool RunCircularChainedThreadsTest()
{
   int numItemsToSend = 10;
   cout << "Circular thread test start" << endl;
   ChainViewCircularThread* threadView = new ChainViewCircularThread( 50 );
   ChainMiddleSupplyCircularThread* threadMiddleSupply = new ChainMiddleSupplyCircularThread( 20 );
   ChainSupplyCircularThread* threadSupply = new ChainSupplyCircularThread( 100, numItemsToSend );

   threadView->AddInputChain( threadMiddleSupply );
   threadSupply->AddOutputChain( threadMiddleSupply );
   
   Sleep( 1000 );

   threadSupply->Pause();
   threadView->Pause();
   threadMiddleSupply->Pause();

   bool isEqual = threadSupply->AreQueuesEqual();
   cout << "Number of items supplied were " << numItemsToSend << endl;
   cout << " Did we receive the same data back " << isEqual << endl;

   threadSupply->Cleanup();
   threadView->Cleanup();
   threadMiddleSupply->Cleanup();

   cout << "Circular thread test end" << endl;

   return isEqual;
}

//-----------------------------------------------

bool RunEventTest()
{
   cout << "Event thread test start" << endl;

   ChainObserverThread* observer = new ChainObserverThread ( 50 );
   ChainControllerThread* controller = new ChainControllerThread( 200 );

   cout << "here we go" << endl;

   controller->Pause();
   controller->AddOutputChain( observer );
   controller->Resume();
   
   Sleep( 500 );

   bool result = true;
   if( observer->GetNumItemsProcessed() < 10 )
   {
      cout << "Event failed" << endl;
      result = false;
   }
   else
   {
      cout << "Event worked" << endl;
   }
   cout << "Event thread test end" << endl;
   return result;
}

//-----------------------------------------------

int __main()
{
   cout << "Beginning all tests" << endl;

   if( RunEventTest() == false ) cout << "Event chain failed" << endl;
   //if( RunCircularChainedThreadsTest() == false ) cout << "Circular chain failed" << endl;
 /*  if( RunCounterThread() == false ) cout << "Counter failed" << endl;
   if( RunInsertThread() == false ) cout << "Insertion failed" << endl;

   
   if( RunChainedThreads01Test() == false ) cout << "Chained 01 failed" << endl;

   if( RunChainedThreads02Test() == false ) cout << "Chained 02 failed" << endl;

   if( RunChainedThreads03Test() == false ) cout << "Chained 03 failed" << endl;*/

   //if( RunChainedThreads_300_Test() == false ) cout << "RunChainedThreads_300_Test failed" << endl;

   cout << "all tests complete" << endl;
   cout << "press any key to exit" << endl;
   getch();
	
   return 0;
}

//-----------------------------------------------
//-----------------------------------------------
