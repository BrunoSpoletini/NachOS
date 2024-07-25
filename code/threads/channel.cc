#include "channel.hh"
#include <cstdio>

Channel::Channel(const char *debugName)
{
    name = debugName;
    lock = new Lock("ChannelLock");
    EsperaRecibido = new Condition("EsperaRecibido", lock);
    mensajeListo = new Condition("mensajeListo", lock);
    finalTransaccion = new Condition("finalTransaccion", lock);
    mensajePuesto = false; 
    recibido = true; 

}

Channel::~Channel()
{
    
    delete EsperaRecibido;
    delete mensajeListo;
    delete finalTransaccion;
    delete lock;
 
}

const char *
Channel::GetName() const
{
    return name;
}

void
Channel::Send(int message) /// hay que esperar a que alguno tome el mensaje para poder seguir
{
        lock->Acquire();
        while( mensajePuesto ) 
            finalTransaccion->Wait();

        recibido = false;
        buffer = message;
        mensajePuesto = true;
        DEBUG('t', "Mensaje enviado: %d\n", message);
        
        mensajeListo->Signal();
        
        while( !recibido )
            EsperaRecibido->Wait();
        
        mensajePuesto = false;
    
        finalTransaccion->Signal();

        lock->Release();
}

void
Channel::Receive(int *message)
{   

    lock->Acquire();
    while( !mensajePuesto || recibido )
        mensajeListo->Wait();

    *message = buffer; 
    recibido = true;
    DEBUG('t', "Mensaje recibido: %d\n", *message);
    
    EsperaRecibido->Signal();
    lock->Release();
}