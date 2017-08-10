#ifndef PERSONAJE_H_
#define PERSONAJE_H_


#include <stdlib.h>
#include <stdio.h> //printf
#include <string.h>    //strlen
#include <signal.h>    //signal
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <commons/config.h>
#include <commons/log.h>
#include <stdarg.h>
#include "commons/sogt/sockets.h"
#include <errno.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/sogt/auxiliares.h>


//Structuras de mutex

typedef struct {
	pthread_mutex_t mutex;
	int puerto;
}t_puerto;

typedef struct{
	pthread_mutex_t mutex;
	int vidas;
}t_vidas;

typedef struct{
	pthread_mutex_t mutex;
	int hilos_corriendo;
}t_hilos_corriendo;

typedef struct{
	t_socket_cliente* socketPlanificador;
	pthread_t idHilo;
}t_puertosPlanif;

//Fin structuras de mutex

//Variables Globales

// falta semaforear esta variable
t_list* l_puertosPlanif; //lista de socket de planificadores por hilo
pthread_mutex_t m_puertos = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
t_config* Personaje;
t_vidas Vidas;
t_puerto miPuerto;
t_hilos_corriendo hilosCorriendo; //variable global de hilos para supervisar la finalizacion de los hilos
int tieneQueMorirsePorUnaSenial=0;//global
int ganoUnaVidaPorUnaSenial=0;//global
char* miIp; //no se pone mutex porque solo se lee
char* Orq;
char** ippuertoOrq;
char* ipOrq;
int portOrq;
char** Plan;

//t_socket_cliente* s_niv;

//Encabezados de funciones

char* string_duplicate(char*);
char** string_split(char*,char*);
void* sleep(int);
int close(int);
void rutina (int n);
void* procPersonaje( void* ptr );

//Fin de Encabezados de funciones
//Fin Variables Globales



#endif /* PERSONAJE_H_ */
