
#include <string.h>
#include "commons/string.h"
#include <stdio.h>
#include <string.h>    //strlen
#include <stdarg.h>
#include "commons/error.h"
#include <stdlib.h>
#include "commons/sogt/sockets.h"
#include "../colas.h"
#include "commons/sogt/mensajeria.h"
#include "auxiliar.h"
#include "variablesGlobales.h"
#include <sys/inotify.h>
#include "plataforma.h"
#include "commons/config.h"



void close(int);

void loggeoPersonajes(t_log* logPlan, cola_t* cola, char* tipoCola){
	int i;
	TprocesoPersonaje* elem=NULL;
	char* personajes=string_new();

	//personajes=concat(2,tipoCola,":");
	string_append(&personajes, tipoCola);
	string_append(&personajes, ":");

	for(i=0;i < cola->cola->elements->elements_count;i++){
		elem=obtener_contenido_pos_determinada(cola, i);
		string_append(&personajes, elem->simbolo);
		string_append(&personajes, "-");
		//personajes=concat(3,personajes,elem->simbolo,"-");
	}
	log_debug(logPlan, personajes);
	free(personajes);
}


tipoNiveles* crearElemento(){
	tipoNiveles* elemento=(tipoNiveles*)malloc(sizeof(tipoNiveles));
	return elemento;
}



TprocesoPersonaje* crearNodoPersonaje(){
	TprocesoPersonaje* procesoPersonaje = (TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje));
	return procesoPersonaje;
}

tipoNiveles* encontrarUnNivel(UnNivel* Nivel){

	int i=0;
	//tipoNiveles* elemento=(tipoNiveles*)malloc(sizeof(tipoNiveles));
	tipoNiveles* elemento=NULL;

	for (i = 0; i < (tamanio_cola(Nivel->colaNiveles)); ++i) {
		elemento=obtener_contenido_pos_determinada(Nivel->colaNiveles, i);
		if(string_equals_ignore_case(Nivel->nivelABuscar, elemento->nivel)){
			break;
			}
		}
	if (i==(tamanio_cola(Nivel->colaNiveles))) {
		elemento=NULL;
	}

	return elemento;
}



char* retornarRecursos(char* mensaje){

	int i;
	char* recursos;
	char* unRecurso;
	char** vector=string_split(mensaje, ":");
	recursos=string_new();
	recursos=vector[2];

	for(i=3;vector[i]!=NULL;i++){
		unRecurso=concat(2,":",vector[i]);
		string_append(&recursos, unRecurso);
	}
	return recursos;
}


char** reemplazarUnRecurso(char** vector, int pos){

	vector[pos]="0";

	return vector;
}

t_desbloqueo* asignarRecursosApersonajes(char** recursos, cola_t* colaBloqueados){


	int i;
	int j;
	char** aux=NULL;
	char* simbolos=string_new();
	char* recAux=string_new();

	TprocesoPersonaje* personaje=NULL;
	t_desbloqueo* auxiliar=(t_desbloqueo*)malloc(sizeof(t_desbloqueo)); //ANDAAA

	for(j=0;j<tamanio_cola(colaBloqueados);j++){
		personaje=obtener_contenido_pos_determinada(colaBloqueados, j);
		for(i=0;recursos[i]!=NULL;i++){
			if(string_equals_ignore_case(personaje->recursoEnEspera, recursos[i])){

				string_append(&simbolos, personaje->simbolo);
				string_append(&simbolos, ":");

				string_append(&recAux, personaje->simbolo);
				string_append(&recAux, ":");
				string_append(&recAux, recursos[i]);
				string_append(&recAux, ":");

				aux=reemplazarUnRecurso(recursos, i);
				recursos=aux;


				printf("inserte los recursos:  %s simbolo:  %s",recAux,simbolos);
				break;
			}
		}
	}
	auxiliar->simboloPersonajeAdesbloq=simbolos;
	auxiliar->recursosAsignados=recAux;


	return auxiliar;
}

//MANDAR RECURSOS CON EL IDENTIFICADOR DEL PERSONAJE "@:F:#:M"


int tamanio_string(char* cadena,char* simbolo){
	int i=0;
	char** array;

	array=string_split(cadena, simbolo);
	while(array[i]!=NULL){
		i++;
	}
	return i;
}

void agregarPersonaje(char* personajes, char* simbolo){

	int i;
	int j=0;
	char** pers;



	if(string_is_empty(personajes)){
		string_append(&personajes, simbolo);
		string_append(&personajes, ":");
		j=-1;
	}else{
		pers=string_split(personajes, ":");
		for(i=0;pers[i]!=NULL;i++){
			if(string_equals_ignore_case(simbolo, pers[i])){
				j=-1;
				break;
			}
		}
	}

	if(j!=-1){
	string_append(&personajes, simbolo);
	string_append(&personajes, ":");
	}
}


char* resolverInterbloqueo(char** personajes, tipoNiveles* unNivel){

	TprocesoPersonaje* personaje=(TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje));
	interbloq* aux=(interbloq*)malloc(sizeof(interbloq));
	int i;
	int j;
	aux->ordenLlegada=-1;

	for(j=0;j<tamanio_cola(unNivel->colaBloqueados);j++){
		personaje=obtener_contenido_pos_determinada(unNivel->colaBloqueados,j);
		for(i=0;personajes[i]!=NULL;i++){
			if(string_equals_ignore_case(personaje->simbolo, personajes[i])){
				if((aux->ordenLlegada>personaje->ordenLlegada)||(aux->ordenLlegada==-1)){
					aux->personaje=personaje->simbolo;
					aux->ordenLlegada=personaje->ordenLlegada;

				}
			}
		}
	}
	return aux->personaje;
}

void matarPersonaje(char* simbolo, tipoNiveles* unNivel, t_log* logger){

	int j;
	TprocesoPersonaje* personaje;
	for(j=0;j<tamanio_cola(unNivel->colaBloqueados);j++){
		personaje=obtener_contenido_pos_determinada(unNivel->colaBloqueados,j);
		if(string_equals_ignore_case(personaje->simbolo, simbolo)){
			personaje=remover_cola_pos_determinada(unNivel->colaBloqueados,j);
			//log_debug(logger, "Elimine al personaje %s; su orden de llegada fue %d.",personaje->simbolo,personaje->ordenLlegada);
			//printf("Elimine al personaje %c, su orden de llegada fue %d.",personaje->simbolo[0],personaje->ordenLlegada);
			enviarMensaje(personaje->fd,e_muerte,"",logger);
			//log_debug(logger, "Se notificó al personaje de su muerte.");

			//free(personaje);
			break;
		}
	}
}

void eliminarPersonajeDesc(tipoNiveles* unNivel, char* simbolo){
	TprocesoPersonaje* personaje=NULL;
	int j;


	for(j=0;j<tamanio_cola(unNivel->colaBloqueados);j++){
		personaje=obtener_contenido_pos_determinada(unNivel->colaBloqueados,j);
		if(string_equals_ignore_case(personaje->simbolo, simbolo)){
			personaje=remover_cola_pos_determinada(unNivel->colaBloqueados , j);
			//log_debug(log->logOrquestador, "El personaje %s abandonó el juego.",personaje->simbolo);
			printf("El personaje %s finalizó el nivel %s; lo saque d la cola bloq.",personaje->simbolo, unNivel->nivel);
			break;
		}
	}

}

void cerrarSockets(tipoNiveles* unNivel){

int i;
TprocesoPersonaje* pj;

for (i=0; i<unNivel->colaListos->cola->elements->elements_count;i++){
	pj=obtener_contenido_pos_determinada(unNivel->colaListos, i);
	close(pj->fd);
}

for (i=0; i<unNivel->colaBloqueados->cola->elements->elements_count;i++){
	pj=obtener_contenido_pos_determinada(unNivel->colaBloqueados, i);
	close(pj->fd);
}
if (unNivel->pjEjec!=NULL) close(unNivel->pjEjec->fd);
}

