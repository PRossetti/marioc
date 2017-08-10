#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_


#include "variablesGlobales.h"
#include "commons/sogt/sockets.h"
#include "../colas.h"
#include <stdlib.h>
#include "commons/sogt/mensajeria.h"
#include "commons/sogt/auxiliares.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/config.h"
#include <sys/time.h>
#include "auxiliar.h"
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

//Structuras

typedef struct {
	int hayNuevosValores;
	int mseg;
	int useg;
	char *algoritmo;
	char *quantum;
	char *retardo;
} t_nuevosValores;

//Fin estructuras

// Encabezados de funciones

int evaluarMensaje (int socket_Nivel, MSJ* msj, int* q, t_nuevosValores** valoresNuevos, t_log* logger);
int desbloquearPersonajes(int socket_Nivel,TprocesoPersonaje* personaje,tipoNiveles* elem,int tipo,t_log* logPlanificador);
//Fin Encabezados de funciones


#endif /* PLANIFICADOR_H_ */
