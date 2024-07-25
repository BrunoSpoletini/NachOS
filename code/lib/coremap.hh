#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH

#include "userprog/address_space.hh"
#include "lib/bitmap.hh"

class Coremap {
public:
    Coremap(unsigned numPhysPages);

    ~Coremap();

    unsigned ReplacePage(AddressSpace* newSpace);

    void Clear(AddressSpace* space);

    unsigned GetVictim();
    
    void UpdateTimers(unsigned pageUsed);

    void ClearPageIndex(unsigned pageUsed);

private:
    AddressSpace** coreMapAdd;
    unsigned numPhysPages;
    unsigned victimIndex = 0;
    unsigned* timers;
    Bitmap *pages;
};


#endif
