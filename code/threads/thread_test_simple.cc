/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.

bool threadsDone[4];

#ifdef SEMAPHORE_TEST
    Semaphore semaphore("Simple Test Semaphore", 3);
#endif

void
SimpleThread(void *name_)
{
    #ifdef SEMAPHORE_TEST
        semaphore.P();
        DEBUG('s',"Semaforo decrementado por hilo %s\n", currentThread->GetName());
    #endif
    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
        currentThread->Yield();
    }

    #ifdef SEMAPHORE_TEST
        semaphore.V();
        DEBUG('s',"Semaforo incrementado por hilo %s\n", currentThread->GetName());
    #endif
    
    unsigned threadNum = atoi(currentThread->GetName());

    if (threadNum > 1)
        threadsDone[threadNum - 2] = true;

    printf("!!! Thread `%s` has finished SimpleThread\n", currentThread->GetName());
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
    for (unsigned i = 0; i < NEW_THREADS; i++) {
        char *name = new char [64];
        sprintf(name, "%d", i + 2);
        threadsDone[i] = false;
        Thread *newThread = new Thread(name);
        newThread->Fork(SimpleThread, NULL);
    }

    //the "main" thread also executes the same function
    SimpleThread((void *) "1");

    //Wait for the threads to finish if needed
    for (unsigned i = 0; i < NEW_THREADS; i++)
        while (!threadsDone[i])
            currentThread->Yield();
    
    printf("Test finished\n");
}
