// Thread.inl

#pragma once

//#include <list>
//using namespace std;
//#include "thread.h"
//////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::AddInputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_inputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks
      
      chainedIterator itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainedInterface* interfacePtr = *itInputs++;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            break;
         }
      }
      if( found == false )
      {
         m_listOfInputs.push_back( chainedInterfaceObject );
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddOutputChain( this, false ); // note, output
   }
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::RemoveInputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_inputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      chainedIterator itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainedInterface* interfacePtr = *itInputs;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            m_listOfInputs.erase( itInputs );
            break;
         }

         // increment here so that erase works
         itInputs++;
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == true && recurse == true )
   {
      chainedInterfaceObject->RemoveOutputChain( this, false ); // note, output
   }
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::AddOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      chainedIterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* interfacePtr = *itOutputs++;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            break;
         }
      }

      if( found == false )// insert into the list
      {
         m_listOfOutputs.push_back( chainedInterfaceObject );
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddInputChain( this, false ); // note, input
   }
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::RemoveOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      chainedIterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* interfacePtr = *itOutputs;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            m_listOfOutputs.erase( itOutputs );
            break;
         }

         // increment here so that erase works
         itOutputs++;
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == true && recurse == true )
   {
      chainedInterfaceObject->RemoveInputChain( this, false );// note, input
   }
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::CleanupAllChainDependencies()
{
   m_inputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      chainedIterator itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainedInterface* interfacePtr = *itInputs++;
         interfacePtr->RemoveOutputChain( this );// note the output
      }
      m_listOfInputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputChainListMutex.unlock();

   //-------------------------------

   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      chainedIterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* interfacePtr = *itOutputs++;
         interfacePtr->RemoveInputChain( this );// note the input
      }
      m_listOfOutputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputChainListMutex.unlock();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type> 
CChainedThread<Type>::CChainedThread( bool needsThreadProtection, int sleepTime, bool paused ) : CAbstractThread( needsThreadProtection, sleepTime, paused )
{
}

//----------------------------------------------------------------

template <typename Type> 
void  CChainedThread<Type>::CallbackOnCleanup()
{
   CChainedThread<Type>::CleanupAllChainDependencies();
   CAbstractThread::CallbackOnCleanup();
}

//----------------------------------------------------------------

template <typename Type> 
int       CChainedThread<Type>::CallbackFunction()
{
   CChainedThread<Type>::m_inputChainListMutex.lock();
   ProcessInputFunction();
   CChainedThread<Type>::m_inputChainListMutex.unlock();

   //-------------------------------

   CChainedThread<Type>::m_outputChainListMutex.lock();
   ProcessOutputFunction();
   CChainedThread<Type>::m_outputChainListMutex.unlock();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////