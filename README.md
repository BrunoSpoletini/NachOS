# NachOS
Not Another Completely Heuristic Operating System o NachOS es un Sistema Operativo educativo para los estudiantes de cursos de Sistemas Operativos.

Escrito originalmente en C++ para MIPS, NachOS se ejecuta como un proceso de usuario en el sistema operativo anfitrión. 

Un simulador de MIPS ejecuta el código para cualquier programa de usuario que se ejecute sobre el sistema operativo NachOS.


## Componentes del NachOS
### Máquina NachOS
NachOS simula una máquina similar a la arquitectura MIPS con registros, memoria y CPU. El objeto Machine, creado al iniciar NachOS, maneja métodos como Run, ReadRegister y WriteRegister, y también las interrupciones, el temporizador y las estadísticas.

### Hilos NachOS
NachOS define una clase de hilos con estados como listo, en ejecución, bloqueado o recién creado. Los métodos incluyen PutThreadToSleep, YieldCPU, ThreadFork y ThreadStackAllocate, y cada hilo se ejecuta en un espacio de direcciones virtuales.

### Programas de usuario NachOS
NachOS ejecuta programas de usuario en espacios de direcciones privados y puede ejecutar binarios MIPS, siempre que solo usen llamadas al sistema que NachOS entienda.
