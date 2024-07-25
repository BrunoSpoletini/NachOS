/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "system.hh"


unsigned amount = 0;
char *buffer[MAX_AMOUNT];
Lock *lock = new Lock("Lock");
Condition *conditionProd = new Condition("Condition Prod", lock);
Condition *conditionCons = new Condition("Condition Cons", lock);
static bool done[PRODS + CONS];

static void
Consumer(void *n_) 
{
    unsigned *n = (unsigned *) n_;
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        lock->Acquire();

        if (amount == 0)
            printf("Consumidor %d esperando (buffer vacio)\n", *n);

        while (amount == 0)
            conditionCons->Wait();

        amount -= 1;
        char* produce = buffer[amount];

        conditionProd->Signal();

        printf("Consumidor %d consume: %s en %d\n", *n, produce, amount);

        lock->Release();
    }

    printf("Consumidor %d termina\n", *n);
    done[*n] = true;
    delete n;
}

static void
Producer(void *n_)
{
    unsigned *n = (unsigned *) n_;
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        lock->Acquire();

        if (amount == MAX_AMOUNT)
            printf("Productor %d esperando (buffer lleno)\n", *n);

        while (amount == MAX_AMOUNT)
            conditionProd->Wait();

        char* produce = new char[64];
        sprintf(produce, "%d_%d", *n, i);
        buffer[amount] = produce;

        conditionCons->Signal();

        printf("Productor %d produce: %s en %d\n", *n, produce, amount);
        amount += 1;

        lock->Release();
    }

    printf("Productor %d termina\n", *n);
    done[CONS + (*n)] = true;
    delete n;
}

void
ThreadTestProdCons()
{
    for (unsigned i = 0; i < PRODS; i++) {
        done[i] = false;
        unsigned *pos = new unsigned();
        *pos = i;
        char *name = new char [64];
        sprintf(name, "Producer %d", i + 1);
        Thread *newThread = new Thread(name);
        newThread->Fork(Producer, pos);
    }
    
    for (unsigned i = 0; i < CONS; i++) {
        done[i + PRODS] = false;
        unsigned *pos = new unsigned();
        *pos = i;
        char *name = new char [64];
        sprintf(name, "Consumer %d", i + 1);
        Thread *newThread = new Thread(name);
        newThread->Fork(Consumer, pos);
    }

    for (unsigned i = 0; i < PRODS + CONS; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
}
