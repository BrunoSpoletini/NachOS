/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "args.hh"

#include <stdio.h>
#include <string.h>


static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

void startProcess(void *args)
{
    currentThread->space->InitRegisters(); //Initialize registers
    currentThread->space->RestoreState();  //Copy the father's state

    if (args != nullptr) {
        int argC = WriteArgs((char **)args);
        int argV = machine->ReadRegister(STACK_REG);

        machine->WriteRegister(4, argC);
        machine->WriteRegister(5, argV);
        machine->WriteRegister(STACK_REG, argV - 24);
    }

    machine->Run(); //Run the program
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT: {
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

        case SC_EXIT: {
            int status = machine->ReadRegister(4);
            DEBUG('e', "Thread %s exited with value %d.\n", currentThread->GetName(), status);
            currentThread->Finish(status);
            break;
        }

        case SC_EXEC: {
            DEBUG('e', "Exec requested.\n");
            int filenameAddr = machine->ReadRegister(4);

            if (!filenameAddr) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char *filename = new char[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof(char)*(FILE_NAME_MAX_LEN+1))) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n", FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *executable = fileSystem->Open(filename);
            if (!executable) {
                DEBUG('e', "Unable to open file %s.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            Thread *thread = new Thread(filename);

            DEBUG('e', "Creating Adress Space %s.\n", filename);
#ifdef SWAP
            AddressSpace *space = new AddressSpace(executable, thread->GetSpaceId());
#else 
            AddressSpace *space = new AddressSpace(executable);
#endif
            DEBUG('e', "Created Adress Space %s.\n", filename);
            
            thread->space = space;
            
            DEBUG('e', "Forking Thread %s.\n", filename);
            thread->Fork(startProcess, nullptr);
            
            SpaceId executed = thread->GetSpaceId();
            DEBUG('e', "Executed %s with spaceId %d.\n", thread->GetName(), executed);
            machine->WriteRegister(2, executed);
#ifndef DEMAND_LOADING
            delete executable;
#endif
            break;
        }

        case SC_EXEC2: {
            DEBUG('e', "Exec2 requested.\n");
            int filenameAddr = machine->ReadRegister(4);
            int argsAddr = machine->ReadRegister(5);
            int joinable = machine->ReadRegister(6);

            if (!filenameAddr) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char *filename = new char[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof(char)*(FILE_NAME_MAX_LEN+1))) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n", FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *executable = fileSystem->Open(filename);
            if (!executable) {
                DEBUG('e', "Unable to open file %s.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }
            
            Thread *thread = new Thread(filename, joinable);
            
            DEBUG('e', "Creating Adress Space %s.\n", filename);
#ifdef SWAP
            AddressSpace *space = new AddressSpace(executable, thread->GetSpaceId());
#else 
            AddressSpace *space = new AddressSpace(executable);
#endif
            DEBUG('e', "Created Adress Space %s.\n", filename);

            thread->space = space;
            
            DEBUG('e', "Forking Thread %s.\n", filename);
            if(!argsAddr) {
                DEBUG('e', "Forking without arguments.\n");
                thread->Fork(startProcess, nullptr);
            } else {
                DEBUG('e', "Forking with arguments.\n");
                thread->Fork(startProcess, SaveArgs(argsAddr));
            }
            
            SpaceId executed = thread->GetSpaceId();
            DEBUG('e', "Executed %s with spaceId %d.\n", thread->GetName(), executed);
            machine->WriteRegister(2, executed);
#ifndef DEMAND_LOADING
            delete executable;
#endif
            break;
        }

        case SC_JOIN: {
            SpaceId id = machine->ReadRegister(4);
            DEBUG('e', "Request to join %d.\n", id);
            Thread *thread = activeThreads->Get(id);
            if (!thread) {
                DEBUG('e', "Thread doesn't exits %d.\n", id);
                machine->WriteRegister(2, -1);
                break;
            }
            int status = thread->Join();
            DEBUG('e', "Thread %d joined.\n", id);
            machine->WriteRegister(2, status);
            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            if (!fileSystem->Create(filename, 0)) {
                DEBUG('e', "Error: filename %s failed creation.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, 0);
            DEBUG('e', "Success: filename %s created.\n", filename);
            break;
        }

        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            
            DEBUG('e', "`Remove` requested for file `%s`.\n", filename);
            if (!fileSystem->Remove(filename)) {
                DEBUG('e', "Error: filename %s failed removal.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, 0);
            DEBUG('e', "Success: filename %s removed.\n", filename);
            break;
        }

        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }
            
            DEBUG('e', "`Open` requested for file `%s`.\n", filename);
            OpenFile *file = fileSystem->Open(filename);
            if (file == nullptr) {
                DEBUG('e', "Error: filename %s failed to be opened.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            int fid = currentThread->FileOpen(file);
            if (fid == -1) {
                DEBUG('e', "Error: thread %s doesn't have filename %s.");
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, fid);
            DEBUG('e', "Success: filename %s opened.\n", filename);
            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);
            OpenFile* file = currentThread->FileClose(fid);
            if(!file) {
                DEBUG('e', "Error closing file %d.\n", fid);
                machine->WriteRegister(2, -1);
                break;
            }
            delete file;
            DEBUG('e', "Success: file with id %d closed.\n", fid);
            machine->WriteRegister(2, 0);
            break;
        }

        case SC_READ: {
            int bufferUsr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            OpenFileId id = machine->ReadRegister(6);

            if (bufferUsr == 0) {
                DEBUG('e', "Error: buffer is null.\n");
                machine->WriteRegister(2, 0);
                break;
            }

            if (id < 0) {
                DEBUG('e', "File id doesn't exists.\n");
                machine->WriteRegister(2, 0);
                break;
            }

            if (size <= 0) {
                DEBUG('e', "Read size incorrect.\n");
                machine->WriteRegister(2, 0);
                break;
            }

            char bufferSys[size+1];
            int bytesRead;
            switch (id)
            {
                case CONSOLE_INPUT:
                {
                    DEBUG('e', "Request to read from console.\n");
                    synchconsole->Read(bufferSys, size);
                    bufferSys[size] = '\0';
                    bytesRead = strlen(bufferSys);
                    DEBUG('e', "Read from console: %s.\n", bufferSys);
                    WriteBufferToUser(bufferSys, bufferUsr, bytesRead);
                    break;
                }
                case CONSOLE_OUTPUT:
                {
                    DEBUG('e', "Can't read from console output.\n");
                    bytesRead = 0;
                    break;
                }
                default:
                {
                    DEBUG('e', "Requested to read from file %d.\n", id);
                    OpenFile *file = currentThread->FileGet(id);
                    if (!file) {
                        DEBUG('e', "File with id %d doesn't exist.\n", id);
                        bytesRead = 0;
                        break;
                    }
                    bytesRead = file->Read(bufferSys, size);
                    bufferSys[bytesRead] = '\0';
                    if (bytesRead != 0)
                        WriteBufferToUser(bufferSys, bufferUsr, bytesRead);
                    DEBUG('e', "Read from file: %s.\n", bufferSys);
                }
            }
            
            machine->WriteRegister(2, bytesRead);
            break;
        }

        case SC_WRITE: {
            int bufferUsr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            OpenFileId id = machine->ReadRegister(6);

            if (bufferUsr == 0) {
                DEBUG('e', "Error: buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (id < 0) {
                DEBUG('e', "File id doesn't exists.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (size <= 0) {
                DEBUG('e', "Read size incorrect.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char bufferSys[size + 1];
            ReadBufferFromUser(bufferUsr, bufferSys, size);
            switch (id)
            {
                case CONSOLE_INPUT:
                {
                    DEBUG('e', "Can't write to console input.\n");
                    machine->WriteRegister(2, -1);
                    break;
                }
                case CONSOLE_OUTPUT:
                {
                    DEBUG('e', "Request to write to console.\n");
                    synchconsole->Write(bufferSys, size);
                    bufferSys[size] = '\0';
                    DEBUG('e', "Wrote to console output: %s.\n", bufferSys);
                    break;
                }
                default:
                {
                    DEBUG('e', "Requested to write to file %d.\n", id);
                    OpenFile *file = currentThread->FileGet(id);
                    if (!file) {
                        DEBUG('e', "File with id %d doesn't exist.\n", id);
                        break;
                    }
                    int bytesWritten = file->Write(bufferSys, size);
                    bufferSys[bytesWritten] = '\0';
                    if (bytesWritten != size)
                        DEBUG('e', "Bytes actually written differ from intended to write.\n", bufferSys);
                    DEBUG('e', "Wrote to file: %s.\n", bufferSys);
                }
            }
            
            machine->WriteRegister(2, 0);
            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}

#ifdef SWAP
void ReplaceTlbEntry(unsigned index, AddressSpace* space, TranslationEntry entry)
{
    TranslationEntry* tlb = machine->GetMMU()->tlb;

    for (unsigned i=0;i<TLB_SIZE;++i) {
        if (!tlb[i].valid) {
            tlb[i] = entry;
            return;
        }
    }

    space->SyncTlbEntry(index);
    tlb[index] = entry;
}
#endif

static void
PageFaultHandler(ExceptionType et)
{
#ifdef USE_TLB
    AddressSpace* space = currentThread->space;

    unsigned vpn = machine->ReadRegister(BAD_VADDR_REG) / PAGE_SIZE;

    static unsigned tlbSelection = 0;
    unsigned index = tlbSelection++%TLB_SIZE;

#ifdef DEMAND_LOADING
#ifdef SWAP
    if(!space->GetPageTableEntry(vpn).valid) {
        unsigned frame = coreMap->ReplacePage(space);
        DEBUG('v', "Loading %lu %lu \n", vpn, frame);
        if(space->GetPageTableEntry(vpn).isInSwap) {
            DEBUG('v', "Swap Loading %lu %lu \n", vpn, frame);
            ReplaceTlbEntry(index, space, space->LoadFromSwap(vpn, frame));
        } else {
            ReplaceTlbEntry(index, space, space->LoadPage(vpn, frame));
        }
        DEBUG('v', "Loaded page for address %lu \n", vpn);
    } else {
        ReplaceTlbEntry(index, space, space->GetPageTableEntry(vpn));
    }
#else
    TranslationEntry* tableE = machine->GetMMU()->tlb + index;
    if(!space->GetPageTableEntry(vpn).valid) {
        DEBUG('v', "Demand loading for address %lu \n", vpn);
        *tableE = space->LoadPage(vpn, pages->Find());
        DEBUG('v', "Loaded page for address %lu \n", vpn);
    } else {
        *tableE = space->GetPageTableEntry(vpn);
    }
#endif
#else
    TranslationEntry* tableE = machine->GetMMU()->tlb + index;
    *tableE = space->GetPageTableEntry(vpn);
#endif
    DEBUG('v', "Virtual page %lu is loaded in the tlb entry %lu\n", vpn, index); 
#else
    DefaultHandler(et);
#endif
}

static void
ReadOnlyHandler(ExceptionType et)
{
#ifdef USE_TLB
    currentThread->Finish(et);
#else
    DefaultHandler(et);
#endif
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &ReadOnlyHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
