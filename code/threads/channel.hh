#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "condition.hh"
#include "system.hh"

class Channel {
public:

    Channel(const char *debugName);

    ~Channel();

    /// For debugging.
    const char *GetName() const;

    void Send(int message);
    void Receive(int *message);

private:
    int buffer;
    const char *name;
    Lock *lock;
    bool mensajePuesto;
    bool recibido;
    Condition *finalTransaccion, *mensajeListo, *EsperaRecibido;
};

#endif