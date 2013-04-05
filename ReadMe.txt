// This threading lib is meant to encapsulate a signalled approach to chaining threads together.
// Signals from other threads/processes should be able to create a new thread and easily be able 
// to link a new HW thread or open file or socket to other threads to do processing.

Look at the testing code in MultipleFeedsIntoOneThread for lots of examples and test_thread01 
has a few minor examples of simple threads and chaining threads.

Ultimately, this lib will be used in a high-speed networking capacity.
