// test_thread01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "thread.h"
#include <conio.h>
#include <iostream>
#include <list>
using namespace std;

#include "MultipleFeedsIntoOneThread.h"
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
      return m_listOfInts.size();
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
      int numToAdd = rand() % 22 + 2;

      int num = m_listOfInts.size();
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
      int numToCull = rand() % 22 + 2;

      int num = m_listOfInts.size();
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
      cout<< "· ChainViewThread ·"  << endl;
      m_items.clear();
      return 1;
   }
   void AcceptChainData( int value )
   {
      LockMutex();
      m_items.push_back( value );
      UnlockMutex();
   }

   int GetNumItems() const
   {
      LockMutex();
      return m_items.size();
      UnlockMutex();
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
         std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
         while( itOutputs != m_listOfOutputs.end() )
         {
            ChainedInterface* chainedInterface = *itOutputs++;
            std::list< int > ::iterator it = m_items.begin();
            while( it != m_items.end() )
            {
               chainedInterface->AcceptChainData( *it++ );
            }
         }
         m_items.clear();

      UnlockMutex();
      return 0; 
   }

   void AcceptChainData( int value )
   {
      LockMutex();
      m_items.push_back( value );
      UnlockMutex();
   }
   int GetNumItems() const
   {
      LockMutex();
      return m_items.size();
      UnlockMutex();
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
      std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* chainedInterface = *itOutputs++;
         for( int i=0; i< 10; i++ )
         {
            chainedInterface->AcceptChainData( rand() % 25 + 1 );
         }
      }
      return 0; 
   }

private:
   ~ChainSupplyThread() {}
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

int main()
{
   cout << "Beginning all tests" << endl;
  /* if( RunCounterThread() == false ) cout << "Counter failed" << endl;
   if( RunInsertThread() == false ) cout << "Insertion failed" << endl;

   
   if( RunChainedThreads01Test() == false ) cout << "Chained 01 failed" << endl;*/

   //if( RunChainedThreads02Test() == false ) cout << "Chained 02 failed" << endl;

   //if( RunChainedThreads03Test() == false ) cout << "Chained 03 failed" << endl;

   if( RunChainedThreads_300_Test() == false ) cout << "RunChainedThreads_300_Test failed" << endl;

   cout << "all tests complete" << endl;
   cout << "press any key to exit" << endl;
   getch();
	return 0;
}

//-----------------------------------------------
//-----------------------------------------------
