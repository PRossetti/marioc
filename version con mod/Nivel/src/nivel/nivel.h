
#ifndef NIVEL_H_
#define NIVEL_H_


#include <commons/sogt/sockets.h>
#include <commons/sogt/mensajeria.h>
#include <stdio.h>
#include "commons/config.h"
#include <stdlib.h>	/*por el malloc*/
#include <string.h> /*por el strcmp*/
#include "commons/collections/list.h"
#include <unistd.h>
#include <pthread.h>
#include <commons/log.h>
#include <sys/inotify.h>
#include <commons/sogt/auxiliares.h>




#define MAX_X 1000
#define MAX_Y 1000

typedef struct{
	t_list* personajesBloqueados;
	pthread_mutex_t acceso;
}tipoLista;

typedef struct {				//Vector del contenido del nivel
	char* inicial;
	int instancias;
	unsigned int posX;
	unsigned int posY;
}t_contenidoNivel;

typedef struct {			//nodo de personaje(colaDeConectados)
	char* simbolo;
	int socket;
	int Interbloqueo;	//1 ESTÁ interbloqueado, 0 NO
	t_list* recursosAsignados;
	char* recursoSolicitado;
	int bloqueado; //1 ESTÁ bloqueado, 0 NO
	unsigned int x;
	unsigned int y;
	unsigned int orden;
}t_recursosDelPersonaje;

typedef struct {
	char* algoritmo;
	int quantum;
	int retardo;
	char* dir;
	int ret;
} t_datosNivel;


t_recursosDelPersonaje* crearPersonaje();
t_recursosDelPersonaje* buscoPersonajePorSimbolo(t_list*,char*);
t_recursosDelPersonaje* encontrarUnPersonaje(t_list*, int);
int buscarIndiceDePersonaje(t_list* unaLista,t_recursosDelPersonaje* unPersonaje);
void* chequeoInterbloqueo();
int dentroDeLosLimites(int x, int y);
int hayCaja(unsigned int x, unsigned int y);
void* logicaEnemigo(char* ptr);
int monitorizarArchivo(t_log* logito);

t_contenidoNivel contenidoNivel[15];
tipoLista* personajesBloqueados;
int idSocketThread;
char * path_config;
t_log* logger;

#endif
