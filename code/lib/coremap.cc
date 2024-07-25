#include "coremap.hh"
#include "threads/system.hh"
#include <stdio.h>
#include <stdlib.h>

Coremap::Coremap(unsigned physPages)
{
    ASSERT(physPages > 0);
    numPhysPages = physPages;
    coreMapAdd = new AddressSpace*[numPhysPages];
    timers = new unsigned[numPhysPages];
    pages = new Bitmap(numPhysPages);
}

Coremap::~Coremap()
{
    delete coreMapAdd;
    delete timers;
    delete pages;
}

#ifdef SWAP
unsigned
Coremap::ReplacePage(AddressSpace* newSpace)
{
    int physIndex = pages->Find();

    if (physIndex == -1) {
        unsigned victim = GetVictim();
        AddressSpace* space = coreMapAdd[victim];
        space->SwapPage(space->GetPhysicalPageIndex(victim));
        physIndex = pages->Find();
        DEBUG('v', "Succesfully swapped, newP: %d\n", physIndex);
    }
    
    coreMapAdd[physIndex] = newSpace;
    return (unsigned) physIndex;
}
#endif

void
Coremap::Clear(AddressSpace* space)
{
    for (unsigned i=0;i<numPhysPages;++i) {
        if (coreMapAdd[i] == space)
            pages->Clear(i);
    }
}

unsigned
Coremap::GetVictim()
{
    DEBUG('v', "Getting Victims\n");
#ifdef PRPOLICY_CLOCK
    for (unsigned i = 0; i < numPhysPages; i++) {
        victimIndex++;
        victimIndex = victimIndex % numPhysPages;
        AddressSpace* space = coreMapAdd[victimIndex];
        if (space == nullptr)
            return victimIndex;
        int vpn = space->GetPhysicalPageIndex(victimIndex);
        TranslationEntry transEntry = space->GetPageTableEntry(vpn);
        
        if (!transEntry.use && !transEntry.dirty)
                return victimIndex;
    }
    
    for (unsigned i = 0; i < numPhysPages; i++) {
        victimIndex++;
        victimIndex = victimIndex % numPhysPages;
        AddressSpace* space = coreMapAdd[victimIndex];
        if (space == nullptr)
            return victimIndex;
        int vpn = space->GetPhysicalPageIndex(victimIndex);
        TranslationEntry transEntry = space->GetPageTableEntry(vpn);

        if (!transEntry.use && transEntry.dirty) {
            return victimIndex;
        } else {
            if (transEntry.use) {
                space->SetNotUsed(vpn);
            }
        }
    }

    for (unsigned i = 0; i < numPhysPages; i++) {
        victimIndex++;
        victimIndex = victimIndex % numPhysPages;
        AddressSpace* space = coreMapAdd[victimIndex];
        if (space == nullptr)
            return victimIndex;
        int vpn = space->GetPhysicalPageIndex(victimIndex);
        TranslationEntry transEntry = space->GetPageTableEntry(vpn);

        if (!transEntry.dirty)
                return victimIndex;
    }
    
    for (unsigned i = 0; i < numPhysPages; i++) {
        victimIndex++;
        victimIndex = victimIndex % numPhysPages;
        AddressSpace* space = coreMapAdd[victimIndex];
        if (space == nullptr)
            return victimIndex;
        int vpn = space->GetPhysicalPageIndex(victimIndex);
        TranslationEntry transEntry = space->GetPageTableEntry(vpn);

        if (transEntry.dirty)
            return victimIndex;
    }

    return victimIndex;
#else
#ifdef PRPOLICY_FIFO
    return victimIndex++ % numPhysPages;
#else
#ifdef PRPOLICY_LRU    
    unsigned victim, m = 0;
    for (unsigned i = 0; i < numPhysPages; ++i)
        if (timers[i] > m){
            victim = i;
            m = timers[i];
        }

    return victim;
#else
    return rand() % numPhysPages;
#endif
#endif
#endif
}

#ifdef PRPOLICY_LRU
void
Coremap::UpdateTimers(unsigned pageUsed)
{
    DEBUG('v', "Updating Timers\n");
    for (unsigned i=0; i < numPhysPages; ++i)
        timers[i]++;
    timers[pageUsed] = 0;
}
#endif

void
Coremap::ClearPageIndex(unsigned page)
{
    pages->Clear(page);
}