// MultipleFeedsIntoOneThread.cpp

#include "stdafx.h"

#include "Thread.h"
#include <conio.h>
#include <iostream>
#include <list>
using namespace std;

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

   int CallbackFunction()
   {
      //cout<< "· ChainViewThread300 ·"  << endl;
      m_items.clear();
      return 1;
   }
   void AcceptChainData( int value )
   {
      LockMutex();
      m_items.push_back( value );
      UnlockMutex();

      m_countIn++;
   }

   int GetNumItems() const
   {
      LockMutex();
      return m_items.size();
      UnlockMutex();
   }

   void  ClearCountIn() { m_countIn = 0; }
   int GetCountIn() const { return m_countIn; }


private:
   ~ChainViewThread300() {}

private:
   list< int > m_items;
   int m_countIn;
};

class ChainMiddleSupplyThread300 : public CChainedThread< int >
{
public:
   ChainMiddleSupplyThread300( int timeOut ) : CChainedThread( false, timeOut ), m_countIn( 0 ), m_countOut( 0 ) {}

   int       ProcessOutputFunction() 
   { 
      LockMutex();

         int num = m_items.size();
         m_countOut += num;
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
      m_countIn++;
   }
   int GetNumItems() const
   {
      LockMutex();
      return m_items.size();
      UnlockMutex();
   }

   void  ClearCountIn() { m_countIn = 0; }
   void  ClearCountOut() { m_countOut = 0; }

   int GetCountIn() const { return m_countIn; }
   int GetCountOut() const { return m_countOut; }

private:
   ~ChainMiddleSupplyThread300() {}

private:
   list< int > m_items;
   int m_countIn;
   int m_countOut;
};

//-----------------------------------------------

class ChainSupplyThread300 : public CChainedThread< int >
{
public:
   ChainSupplyThread300( int timeOut, int id ) : CChainedThread( false, timeOut, false ), m_count( 0 ), m_id( id )
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
            chainedInterface->AcceptChainData( rand() % 25 + 1 );
            m_count++;
         }
      }
      return 0; 
   }

   void  ClearCount() { m_count = 0; }
   int GetCount() const { return m_count; }

private:
   ~ChainSupplyThread300() {}
private:
   int m_count;
   int m_id;
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
            m_numRemoved( 0 )
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
              //m_thread[i]->Resume();
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

   viewSupply->ClearCountIn();
}

int  RunChainedThreads_300_Test()
{
   int success = true;
   try
   {
      cout << "Multiple suppliers test start.. this test takes about 1 minute" << endl;

      const int numSuppliers = 300;
      ChainSupplyThread300* arrayOfSuppliers[ numSuppliers ];
      memset( arrayOfSuppliers, 0, sizeof( arrayOfSuppliers ) );

      ChainMiddleSupplyThread300* middleSupply = new ChainMiddleSupplyThread300( 300 );
      ChainViewThread300* viewSupply = new ChainViewThread300( 100 );
      viewSupply->AddInputChain( middleSupply );

      for( int i=0; i<numSuppliers; i++ )
      {
         arrayOfSuppliers[i] = new ChainSupplyThread300( ChooseRandomTimeout( 100, 3000 ), i );
         arrayOfSuppliers[i]->AddOutputChain( middleSupply );
      }

      cout << "Waiting a bit to make sure that quiescence works" << endl;
      Sleep( 2000 );
      cout << "Items sent during quiescence = " << GetTotalSent( arrayOfSuppliers, numSuppliers ) << endl;

      cout << "Testing only 10 suppliers start" << endl;
         for( int i=0; i<10; i++ )
         {
            arrayOfSuppliers[i]->Resume();
         }
         Sleep( 2000 );
         for( int i=0; i<10; i++ )
         {
            arrayOfSuppliers[i]->Pause();
         }
      cout << "Testing only 10 suppliers end" << endl;

      DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
      ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );

      int time = 12000;
      cout << "Testing 300 suppliers start for " << time/1000 << " seconds" << endl;
         for( int i=0; i<numSuppliers; i++ )
         {
            arrayOfSuppliers[i]->Resume();
         }
         Sleep( time );
         for( int i=0; i<numSuppliers; i++ )
         {
            arrayOfSuppliers[i]->Pause();
         }
      cout << "Testing 300 suppliers end" << endl;

      DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
      ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );


      cout << "Testing 300 suppliers, random deletion start for " << time/1000 << " seconds" << endl;
         for( int i=0; i<numSuppliers; i++ )
         {
            arrayOfSuppliers[i]->Resume();
         }
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
         for( int i=0; i<numSuppliers; i++ )
         {
            if( arrayOfSuppliers[i] != NULL )
            {
               arrayOfSuppliers[i]->Pause();
            }
         }
         cout<< "Sleep after pausing remaining suppliers" << endl;
         Sleep( time - tempPause );
      cout << "Testing 300 suppliers, random deletion" << endl;

      DisplayCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
      ClearAllCounts( arrayOfSuppliers, numSuppliers, middleSupply, viewSupply );
      
      cout << "Prep for creating randomly coming and going clients 1 " << endl;
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

         for( int i=0; i<numSuppliers; i++ )
         {
            if( arrayOfSuppliers[i] != NULL )
            {
               arrayOfSuppliers[i]->Pause();
            }
         }

      cout << "Creating randomly coming and going clients end" << endl;
      

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