#include "synch_console.hh"
#include <stdio.h>

static void
readHandler(void *arg)
{
    ASSERT(arg != nullptr);
    ((SynchConsole *)arg)->ReadAvail();
}

static void
writeHandler(void *arg)
{
    ASSERT(arg != nullptr);
    ((SynchConsole *)arg)->WriteDone();
}

void
SynchConsole::ReadAvail()
{
    readAvail->V();
}

void
SynchConsole::WriteDone()
{
    writeDone->V();
}

SynchConsole::SynchConsole(const char *in, const char *out) {
    console = new Console(in, out, readHandler, writeHandler, this);
    writeDone = new Semaphore("write done", 0);
    readAvail = new Semaphore("read avail", 0);
    lockWrite = new Lock("write console");
    lockRead = new Lock("read console");
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete writeDone;
    delete readAvail;
    delete lockWrite;
    delete lockRead;
}

void
SynchConsole::Write(char *buffer, unsigned size)
{
    lockWrite->Acquire();

    for (unsigned i = 0; i < size; ++i) {
        console->PutChar(buffer[i]);
        writeDone->P();
    }

    lockWrite->Release();
}

void
SynchConsole::Read(char *buffer, unsigned size)
{
    lockRead->Acquire();
    
    if (size == 0)  {
        lockRead->Release();
        return;
    }

    readAvail->P();
    buffer[0] = console->GetChar();

    for (unsigned i = 0; i < (size - 1); ++i) {
        char nextChar = console->GetChar();

        if (nextChar != EOF) {
            buffer[i] = nextChar;
            readAvail->P();
        } else {
            buffer[i] = '\0';
            break;
        }
    }

    lockRead->Release();
}
