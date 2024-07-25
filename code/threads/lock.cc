/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "lock.hh"
#include "system.hh"


/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    name = debugName;
    semaphore = new Semaphore("Lock", 1);
}

Lock::~Lock()
{
    delete semaphore;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    DEBUG('s', "Thread \"%s\" is doing Acquire\n", currentThread->GetName());
    ASSERT(!IsHeldByCurrentThread());

    if (currentHolder != nullptr && currentThread->GetPriority() < currentHolder->GetPriority()) {
        previousPriority = currentHolder->GetPriority();
        scheduler->TransferPriority(currentHolder, currentThread->GetPriority());
        semaphore->PP();
    } else {
        semaphore->P();
    }
    
    currentHolder = currentThread;
}

void
Lock::Release()
{
    DEBUG('s', "Thread \"%s\" is doing Release\n", currentThread->GetName());
    ASSERT(IsHeldByCurrentThread());
    if (previousPriority != -1) {
        scheduler->TransferPriority(currentHolder, previousPriority);
        previousPriority = -1;
    }

    currentHolder = nullptr;
    semaphore->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return currentThread == currentHolder;
}
