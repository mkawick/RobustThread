

//////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------

template <class Type> 
void  ChainedInterface<Type>::AddInputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_inputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itInputs = m_listOfInputs.begin();
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
   m_inputListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddOutputChain( this, false ); // note, output
   }
}

//----------------------------------------------------------------

template <class Type> 
void  ChainedInterface<Type>::RemoveInputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_inputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itInputs = m_listOfInputs.begin();
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
   m_inputListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == true && recurse == true )
   {
      chainedInterfaceObject->RemoveOutputChain( this, false ); // note, output
   }
}

//----------------------------------------------------------------

template <class Type> 
void  ChainedInterface<Type>::AddOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
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
   m_outputListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == false && recurse == true )
   {
      chainedInterfaceObject->AddInputChain( this, false ); // note, input
   }
}

//----------------------------------------------------------------

template <class Type> 
void  ChainedInterface<Type>::RemoveOutputChain( ChainedInterface<Type>* chainedInterfaceObject, bool recurse )
{
   bool found = false;
   m_outputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
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
   m_outputListMutex.unlock();

   // notice that we are outside of the locks now.
   if( found == true && recurse == true )
   {
      chainedInterfaceObject->RemoveInputChain( this, false );// note, input
   }
}

//----------------------------------------------------------------

template <class Type> 
void  ChainedInterface<Type>::CleanupAllChainDependencies()
{
   m_inputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itInputs = m_listOfInputs.begin();
      while( itInputs != m_listOfInputs.end() )
      {
         ChainedInterface* interfacePtr = *itInputs++;
         interfacePtr->RemoveOutputChain( this );// note the output
      }
      m_listOfInputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_inputListMutex.unlock();

   //-------------------------------

   m_outputListMutex.lock();
   {
      // the indentation here is to show that we are in the 'scope' of the locks

      std::list< ChainedInterface* > ::iterator itOutputs = m_listOfOutputs.begin();
      while( itOutputs != m_listOfOutputs.end() )
      {
         ChainedInterface* interfacePtr = *itOutputs++;
         interfacePtr->RemoveInputChain( this );// note the input
      }
      m_listOfOutputs.clear();

      // the indentation here is to show that we are in the 'scope' of the locks
   }
   m_outputListMutex.unlock();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

template <class Type> 
CChainedThread<Type>::CChainedThread( bool needsThreadProtection, int sleepTime, bool paused ) : CAbstractThread( needsThreadProtection, sleepTime, paused )
{
}

//----------------------------------------------------------------

template <class Type> 
void  CChainedThread<Type>::CallbackOnCleanup()
{
   CleanupAllChainDependencies();
   CAbstractThread::CallbackOnCleanup();
}

//----------------------------------------------------------------

template <class Type> 
int       CChainedThread<Type>::CallbackFunction()
{
   m_inputListMutex.lock();
   ProcessInputFunction();
   m_inputListMutex.unlock();

   //-------------------------------

   m_outputListMutex.lock();
   ProcessOutputFunction();
   m_outputListMutex.unlock();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////