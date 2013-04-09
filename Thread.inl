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
      
      ChainLinkIteratorType itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainLink& chainedInput = *itInputs++;
		   ChainedInterface* interfacePtr = chainedInput.m_interface;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            break;
         }
      }
      if( found == false )
      {
         m_listOfInputs.push_back( ChainLink( chainedInterfaceObject ) );
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddOutputChain( this, false ); // note, output
   }
   NotifyFinishedAdding();
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::RemoveInputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_inputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      ChainLinkIteratorType itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainLink& chainedInput = *itInputs;
		   ChainedInterface* interfacePtr = chainedInput.m_interface;
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
   NotifyFinishedRemoving();
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::AddOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         const ChainLink& chain = *itOutputs++;
         ChainedInterface* interfacePtr = chain.m_interface;
         if( interfacePtr == chainedInterfaceObject )
         {
            found = true;
            break;
         }
      }

      if( found == false )// insert into the list
      {
         m_listOfOutputs.push_back( ChainLink( chainedInterfaceObject ) );
      }

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputChainListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddInputChain( this, false ); // note, input
   }
   NotifyFinishedAdding();
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::RemoveOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs;
		   ChainedInterface* interfacePtr = chainedOutput.m_interface;
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
   NotifyFinishedRemoving();
}

//----------------------------------------------------------------

template <typename Type> 
void  ChainedInterface<Type>::CleanupAllChainDependencies()
{
   m_inputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      ChainLinkIteratorType itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainLink& chainedInput = *itInputs++;
		   ChainedInterface* interfacePtr = chainedInput.m_interface;
         interfacePtr->RemoveOutputChain( this );// note the output
      }
      m_listOfInputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputChainListMutex.unlock();
   NotifyFinishedAdding();

   //-------------------------------

   m_outputChainListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      ChainLinkIteratorType itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainLink& chainedOutput = *itOutputs++;
         ChainedInterface* interfacePtr = chainedOutput.m_interface;
         interfacePtr->RemoveInputChain( this );// note the input
      }
      m_listOfOutputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputChainListMutex.unlock();

   NotifyFinishedRemoving();
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