# mariocSO
"Juego" escrito en C basado en el universo de Mario Bros. Realizado con fines didácticos.

Realizado como TP de la asignatura Sistemas Operativos de la UTN FRBA en el año 2013.

El objetivo del TP es llevar a la práctica los conceptos de sistemas operativos vistos en la asignatura: semáforos, planificación, comunicación y sincronización entre procesos, sockets, gestión de memoria, sistemas de archivos, señales, hilos (threads), inanición, deadlocks, forks, etc.

Resumen de la "mecánica" del "juego":

El juego se compone básicamente de los procesos: Personaje, Nivel, Plataforma (orquestador) y un File System (FUSE).
Cada persona tiene una lista de niveles a completar dada por un archivo de configuración y por cada nivel tiene una lista de objetivos. Los objetivos refieren a cantidad de unidades de determinados recursos que el personaje debe recolectar. El personaje no puede terminar un nivel hasta no haber recolectado de cada recurso, las cantidades que indique su lista de objetivos. En el nivel existen enemigos que al "pisar" al personaje le quitan una vida, lo cual genera que el personaje libere los recursos que tenía en posesión y arranque de 0 sus objetivos.
Los enemigos siempre se mueven en dirección al personaje más cercano. Cuando no hay personajes en el nivel, los enemigos se mueven como los caballos del ajedrez. En un mismo Nivel puede haber N personajes, cada uno hace un movimiento por turno (quorum). El personaje siempre se mueve en dirección a un recurso. Los recursos son limitados, por lo que los personajes pueden entrar en deadlock. Los enemigos no matan a personajes posicionados sobre recursos por lo cual el deadlock debe ser resuelto con una señal ingresada por el usuario por teclado.
Todos los niveles existentes (archivo de configuración) se levantan en simultaneo y un personaje juega todos los niveles que le correspondan en paralelo.
Cuando (y si) todos los personajes terminan todos sus niveles, se muestra una animación de Koopa.

