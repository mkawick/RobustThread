// MultipleFeedsIntoOneThread.cpp


#include "StdAfx.h"
#if defined(_WIN32)

#include <conio.h>

#else 

#include <curses.h>

#endif

#include <list>
#include <deque>
#include <iostream>
#include <algorithm>
using namespace std;

#include "Thread.h"
#include "MultipleFeedsIntoOneThread.h"


int   ChooseRandomTimeout( int low, int high )
{
   int range = high-low + 1;// [0,n) -> [0,n]...accounting for the modulus range being 0-n with n exclusive and we want n inclusive
   int value = rand() % range + low;
   return value;
}


//-----------------------------------------------

// note, we are not hooking backwards
class ChainViewThread300 : public CChainedThread< int >
{
public:
   ChainViewThread300( int timeOut ) : CChainedThread( true, timeOut ), m_countIn( 0 ){}

   int		CallbackFunction()
   {
      //cout<< "· ChainViewThread300 ·"  << endl;
	  int num = m_items.size();
      //while( num -- )
	  {
		  m_items.clear();
	  }
      return 1;
   }
   bool		AcceptChainData( int value )
   {
	  if( m_mutex.IsLocked() )
		  return false;

      LockMutex();
      m_items.push_back( value );
      UnlockMutex();
      m_countIn++;

	  return true;
   }

   int		GetNumItems() const
   {
	  int count;
      LockMutex();
      count = static_cast<int>( m_items.size() );
      UnlockMutex();

	  return count;
   }

   void		ClearCountIn() { m_countIn = 0; }
   void		ClearPending() 
   {
	  LockMutex();
      m_items.clear();
      UnlockMutex();
   }
   int		GetCountIn() const { return m_countIn; }


private:
   ~ChainViewThread300() {}

private:
   deque< int > m_items;
   int m_countIn;
};

class ChainMiddleSupplyThread300 : public CChainedThread< int >
{
public:
   
   ChainMiddleSupplyThread300( int timeOut ) :
      CChainedThread( false, timeOut ),
      m_countIn( 0 ),
      m_countOut( 0 ),
      m_numTimeItemsMovedIntoOut( 0 ),
      m_numTimesThatQueueHadItemsInIt( 0 )
   {}

   int		 ProcessInputFunction()
   {
	   m_inputMutex.lock(); LockMutex();
	   std::copy( m_itemsIn.begin(), m_itemsIn.end(), std::back_insert_iterator<std::deque< int > >( m_itemsOut ));
	   m_itemsIn.clear();
	   UnlockMutex(); m_inputMutex.unlock();

	   return 0;
   }
   int       ProcessOutputFunction() 
   {
	  MutexLock lock( m_mutex );
         
         std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
         //while( itOutputs != m_listOfOutputs.end() )
         {
            ChainedInterface* chainedInterface = *itOutputs++;
			int num = static_cast<int>( m_itemsOut.size() );
			int numAcceptedItems = 0;
			if( num > 0 )
				m_numTimesThatQueueHadItemsInIt ++;

			for( int i=0; i<num; i++ )
			{
				int val = m_itemsOut.front();
				if( chainedInterface->AcceptChainData( val ) == false )
				{
					break;
				}
				numAcceptedItems ++;
				m_itemsOut.pop_front();
			}
			m_countOut += numAcceptedItems;
			if( numAcceptedItems > 0 )
				m_numTimeItemsMovedIntoOut++;
         }
         m_itemsOut.clear();

      return 0; 
   }

   bool  AcceptChainData( int value )
   {
	  if( m_inputMutex.IsLocked() == true )
		  return false;
	  m_inputMutex.lock();
      m_itemsIn.push_back( value );
	  m_inputMutex.unlock();
      m_countIn++;
	  return true;
   }
   int	 GetNumItems() const
   {
	  int count;
	  m_inputMutex.lock();
      count = static_cast<int>( m_itemsIn.size() );
	  m_inputMutex.unlock();

	  LockMutex();
	  count += static_cast<int>( m_itemsOut.size() );
      UnlockMutex();

	  return count;

   }

   int	 GetNumIn() const
   {
	  int count;
	  m_inputMutex.lock();
      count = static_cast<int>( m_itemsIn.size() );
	  m_inputMutex.unlock();
	  return count;

   }
   int	GetNumOut() const 
   {
      int count;
	  LockMutex();
	  count = static_cast<int>( m_itemsOut.size() );
      UnlockMutex();
	  return count;
   }

   void  ClearCountIn() { m_countIn = 0; }
   void  ClearCountOut() { m_countOut = 0; }
   void  ClearPending() 
   { 
	  m_inputMutex.lock();
      m_itemsIn.clear();
	  m_inputMutex.unlock();

	  LockMutex();
	  m_itemsOut.clear();
      UnlockMutex();
   }

   int	 GetCountIn() const { return m_countIn; }
   int   GetCountOut() const { return m_countOut; }

private:
   ~ChainMiddleSupplyThread300() {}

private:
   deque< int >		m_itemsIn;
   deque< int >		m_itemsOut;
   mutable Mutex	m_inputMutex;
   int				m_countIn;
   int				m_countOut;
   int m_numTimeItemsMovedIntoOut;
   int m_numTimesThatQueueHadItemsInIt;
};

//-----------------------------------------------

class ChainSupplyThread300 : public CChainedThread< int >
{
public:
   ChainSupplyThread300( int timeOut, int id ) :
      CChainedThread( false, timeOut, true ),
      m_count( 0 ),
      m_id( id ),
      m_startedAdd( false )
   { 
   }

   int       ProcessOutputFunction() 
   {
      std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* chainedInterface = *itOutputs++;
         int num = rand() % 10 + 1;
         for( int i=0; i< num; i++ )
         {
            m_startedAdd = true;
			int value = m_count % 25 + 1;
			bool result = chainedInterface->AcceptChainData( value );
            m_startedAdd = false;
			if( result == false )
				break;
            m_count++;
         }
      }
      return 0; 
   }

   void		ClearCount() { m_count = 0; }
   int		GetCount() const { return m_count; }
   bool		IsStuckAdding() const 
   { 
	   if( m_startedAdd == true ) 
		   return true; 
	   return false; 
   }


private:
   ~ChainSupplyThread300() {}
private:
   int	m_count;
   int	m_id;
   bool m_startedAdd;
};

//-----------------------------------------------
//-----------------------------------------------

class AddingSupplyThread : public CAbstractThread
{
public:
   AddingSupplyThread( int timeOut, ChainSupplyThread300** thread, int num, ChainMiddleSupplyThread300* middleSupply ) : 
            CAbstractThread( true, timeOut ), 
            m_thread( thread ), 
            m_arraySize( num ), 
            m_middleSupply( middleSupply ),
            m_numAdded( 0 ),
            m_numRemoved( 0 ),
            m_startedAdd( false )
            {}

   int CallbackFunction()
   {
      if( rand() % 2 != 0 )// true false
      {
         int numToAdd = rand() % 3 + 1;
         int numAdded = 0;
         for( int i=0; i<m_arraySize; i ++ )
         {
            if( m_thread[i] == NULL )
            {
               m_thread[i] = new ChainSupplyThread300( ChooseRandomTimeout( 100, 3000 ), i );
               m_thread[i]->AddOutputChain( m_middleSupply );
               if( ++numAdded >= numToAdd )
                  break;
            }
         }
         m_numAdded += numAdded;
      }
      else
      {
         int numToRemove = rand() % 3 + 1;
         int numRemoved = 0;
         for( int i=0; i<m_arraySize; i ++ )
         {
            if( m_thread[i] != NULL )
            {
               m_thread[i]->Cleanup();
               m_thread[i] = NULL;
               if( ++numRemoved >= numToRemove )
                  break;
            }
         }

         m_numRemoved += numRemoved;
      }
      return 0;
   }

   int GetNumAdded() const { return m_numAdded; }
   int GetNumRemoved() const { return m_numRemoved; }

private:
   ~AddingSupplyThread() {}

private:
   ChainSupplyThread300** m_thread;
   ChainMiddleSupplyThread300* m_middleSupply;
   int m_arraySize;

   int m_numAdded, m_numRemoved;
   bool  m_startedAdd;
};

//-----------------------------------------------


int GetTotalSent( ChainSupplyThread300** thread, int num )
{
   int total = 0;
   for( int i=0; i< num; i++ )
   {
      if( thread[i] != NULL )
      {
         total += thread[i]->GetCount();
      }
   }
   return total;
}

void  DisplayCounts( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
   cout << "Final numbers: total supplied=" << GetTotalSent( arrayOfSuppliers, numSuppliers ) ;
   cout << ", middleIn=" << middleSupply->GetCountIn() << ", middleOut=" << middleSupply->GetCountOut();
   cout << ", middleInProcess=" << middleSupply->GetNumItems();
   cout << ", viewIn=" << viewSupply->GetCountIn() << endl;
}

void  ClearAllCounts( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
   for( int i=0; i<numSuppliers; i++ )
   {
      if( arrayOfSuppliers[i] )
      {
         arrayOfSuppliers[i]->ClearCount();
      }
   }
   middleSupply->ClearCountIn();
   middleSupply->ClearCountOut();
   middleSupply->ClearPending();

   viewSupply->ClearCountIn();
   viewSupply->ClearPending();
}

//------------------------------------------------------------------------------------------------------

void  OutputStuckThreads( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers )
{
   int numStuck = 0;
   int minimumSchedule = 1000000;
   int maximumSchedule = 0;
   for( int i=0; i<numSuppliers; i++ )
   {
      if( arrayOfSuppliers[i] && 
		  arrayOfSuppliers[i]->IsStuckAdding() == true )
      {
		 int val = arrayOfSuppliers[i]->GetSleepTime();
		 if( val < minimumSchedule )
			 minimumSchedule = val;
		 if( val > maximumSchedule )
			 maximumSchedule = val;
         numStuck++;
      }
   }
   
   cout << "Num stuck threads = " << numStuck << endl;
   if( numStuck != 0 )
   {
	  cout << "Schedule[ " << minimumSchedule << ", " << maximumSchedule << " ]" << endl;
   }
}

//------------------------------------------------------------------------------------------------------

void	PauseSuppliers( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers )
{
   for( int i=0; i<numSuppliers; i++ )
   {
      if( arrayOfSuppliers[i] )
	  {
	     arrayOfSuppliers[i]->Pause();
	  }
   }
}

//------------------------------------------------------------------------------------------------------

void	ResumeSuppliers( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers )
{
   for( int i=0; i<numSuppliers; i++ )
   {
	  if( arrayOfSuppliers[i] )
	  {
		 arrayOfSuppliers[i]->Resume();
	  }
   }
}
//------------------------------------------------------------------------------------------------------

void	SimpleChainTest( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
   cout << "-------------------------------------------------------" << endl;
   cout << "Testing SimpleChain start" << endl;

   PauseSuppliers( arrayOfSuppliers, numSuppliers );
   middleSupply->Pause();
   int numToTest = 1666;

   for( int i=0; i<numToTest; i++ )
   {
	  middleSupply->AcceptChainData( i% 10 );
   }

   cout << "Num put in middle = " << numToTest << " and number stored in middle: received = " << middleSupply->GetCountIn();
   cout << ", sent = " << middleSupply->GetCountOut() << ", enqueue in = " << middleSupply->GetNumIn() << ", enqueue out = ";
   cout << middleSupply->GetNumOut() << endl;

   cout << "Now let it push it's data to the view " << endl;
   middleSupply->Resume();
   Sleep( 5000 );

   cout << "Num put in middle = " << numToTest << " and number stored in middle: received = " << middleSupply->GetCountIn();
   cout << ", sent = " << middleSupply->GetCountOut() << ", enqueue in = " << middleSupply->GetNumIn() << ", enqueue out = ";
   cout << middleSupply->GetNumOut() << endl;
   DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

   OutputStuckThreads( arrayOfSuppliers, numSuppliers );

   middleSupply->ClearCountIn();
   middleSupply->ClearCountOut();
   middleSupply->ClearPending();

   DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
   cout << "Testing SimpleChain start" << endl;
}

//------------------------------------------------------------------------------------------------------

void	TestOnly10Suppliers( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
   cout << "Testing only 10 suppliers start" << endl;

   ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

     PauseSuppliers( arrayOfSuppliers, numSuppliers );
     Sleep( 2000 );
     ResumeSuppliers( arrayOfSuppliers, numSuppliers );

  OutputStuckThreads( arrayOfSuppliers, numSuppliers );

  DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
  cout << "Testing only 10 suppliers end" << endl;
}

//------------------------------------------------------------------------------------------------------

void	Test300Suppliers( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
   int time = 12000;
   cout << "Testing 300 suppliers start for " << time/1000 << " seconds" << endl;
   ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
   
 ResumeSuppliers( arrayOfSuppliers, numSuppliers );
 Sleep( time );
 PauseSuppliers( arrayOfSuppliers, numSuppliers );

 OutputStuckThreads( arrayOfSuppliers, numSuppliers );
cout << "Testing 300 suppliers end" << endl;

middleSupply->Pause();
viewSupply->Pause();
DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

middleSupply->Resume();
viewSupply->Resume();
}

//------------------------------------------------------------------------------------------------------

void Run300WithRandomDeletion( ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
	int time = 12000;
	cout << "Testing 300 suppliers, random deletion start for " << time/1000 << " seconds" << endl;

	ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

     ResumeSuppliers( arrayOfSuppliers, numSuppliers );

     int tempPause = 2000;
     if( tempPause > time )
        tempPause = time;
     Sleep( tempPause );
     for( int i=0; i<30; i++ )
     {
        int which = rand() % numSuppliers;
        if( arrayOfSuppliers[which] != NULL )
        {
           arrayOfSuppliers[which]->Cleanup();
           arrayOfSuppliers[which] = NULL;
        }
     }
     PauseSuppliers( arrayOfSuppliers, numSuppliers );

     cout<< "Sleep after pausing remaining suppliers" << endl;
     Sleep( time - tempPause );
  OutputStuckThreads( arrayOfSuppliers, numSuppliers );
  DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
  cout << "Testing 300 suppliers, random deletion" << endl;
}

//------------------------------------------------------------------------------------------------------

void	RandomlyArrivingAndLeavingConnections(ChainSupplyThread300** arrayOfSuppliers, int numSuppliers, ChainMiddleSupplyThread300* middleSupply, ChainViewThread300* viewSupply )
{
cout << "Prep for creating randomly coming and going clients 1 " << endl;
ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
for( int i=0; i<numSuppliers; i++ )
{
 if( arrayOfSuppliers[i] != NULL && rand()%2 )// 50/50 chance of deleting these
 {
    arrayOfSuppliers[i]->Cleanup();
    arrayOfSuppliers[i] = NULL;
 }
}

cout << "Prep for creating randomly coming and going clients 2 " << endl;
Sleep( 2000 );

int numRunningSuppliers = 0;
for( int i=0; i<numSuppliers; i++ )
{
 if( arrayOfSuppliers[i] != NULL )
 {
    arrayOfSuppliers[i]->Resume();
    numRunningSuppliers ++;
 }
}

cout << "-------------------------------------------------------" << endl;
cout << "Start number of suppliers = " << numRunningSuppliers << endl;
AddingSupplyThread* adder = new AddingSupplyThread( 180, arrayOfSuppliers, numSuppliers, middleSupply );

cout << "Creating randomly coming and going clients" << endl;
 Sleep( 12000 );

 adder->Pause();
 numRunningSuppliers = 0;
 for( int i=0; i<numSuppliers; i++ )
 {
    if( arrayOfSuppliers[i] != NULL )
    {
       arrayOfSuppliers[i]->Pause();
       numRunningSuppliers ++;
    }
 }

 cout << "Final number of suppliers = " << numRunningSuppliers << endl;
 cout << "num added = " << adder->GetNumAdded() << ", " << " num removed = " << adder->GetNumRemoved() << endl;
 
 adder->Cleanup();

 PauseSuppliers( arrayOfSuppliers, numSuppliers );

cout << "Creating randomly coming and going clients end" << endl;

OutputStuckThreads( arrayOfSuppliers, numSuppliers );
}

//------------------------------------------------------------------------------------------------------

int  RunChainedThreads_300_Test()
{
   int success = true;
   try
   {
      cout << "Multiple suppliers test start.. this test takes about 1 minute" << endl;

      const int numSuppliers = 300;
      ChainSupplyThread300* arrayOfSuppliers[ numSuppliers ];
      memset( arrayOfSuppliers, 0, sizeof( arrayOfSuppliers ) );

      ChainMiddleSupplyThread300* middleSupply = new ChainMiddleSupplyThread300( 200 );
      ChainViewThread300* viewSupply = new ChainViewThread300( 300 );
      viewSupply->AddInputChain( middleSupply );

      for( int i=0; i<numSuppliers; i++ )
      {
         arrayOfSuppliers[i] = new ChainSupplyThread300( ChooseRandomTimeout( 100, 3000 ), i );
         arrayOfSuppliers[i]->AddOutputChain( middleSupply );
      }
      
      OutputStuckThreads( arrayOfSuppliers, numSuppliers );

      cout << "Waiting a bit to make sure that quiescence works" << endl;
      Sleep( 2000 );
      cout << "Items sent during quiescence = " << GetTotalSent( arrayOfSuppliers, numSuppliers ) << endl;
	  
	  cout << "-------------------------------------------------------" << endl;
	  SimpleChainTest( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

	  cout << "-------------------------------------------------------" << endl;
	  TestOnly10Suppliers( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

	  cout << "-------------------------------------------------------" << endl;
      Test300Suppliers( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

	  cout << "-------------------------------------------------------" << endl;
      Run300WithRandomDeletion( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

	  cout << "-------------------------------------------------------" << endl;

      RandomlyArrivingAndLeavingConnections( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
      

	  cout << "-------------------------------------------------------" << endl;
      cout << "final cleanup" << endl;
      for( int i=0; i<numSuppliers; i++ )
      {
         if( arrayOfSuppliers[i] != NULL )
         {
            arrayOfSuppliers[i]->Cleanup();
            arrayOfSuppliers[i] = NULL;
         }
      }
      middleSupply->Cleanup();
      viewSupply->Cleanup();

      cout << "final cleanup complete" << endl;

      cout << "Multiple suppliers test end" << endl;
   }catch ( ... )
   {
      success = false;
   }

   return success;
}

//-----------------------------------------------