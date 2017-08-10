
#include "colas.h"
#include <stdlib.h>
#include <pthread.h>
#include "plataforma/auxiliar.h"

cola_t *crear_cola() {
	cola_t * s_queue = calloc(1, sizeof(cola_t));
	s_queue->cola = queue_create();
	//sem_init(&(s_queue->elementos_disponibles), 0, 0);
	pthread_mutex_init((&(s_queue->acceso)), NULL);
	return s_queue;
}

void pushear_cola(cola_t *cola, void *elemento) {
	pthread_mutex_lock(&(cola->acceso));
	queue_push((cola->cola), elemento);
	//sem_post(&(cola->elementos_disponibles));
	pthread_mutex_unlock(&(cola->acceso));
}

void *remover_de_cola(cola_t *cola) {
	//sem_wait(&(cola->elementos_disponibles));
	pthread_mutex_lock(&(cola->acceso));
	void* elemento = queue_pop(cola->cola);
	pthread_mutex_unlock(&(cola->acceso));
	return elemento;
}

int tamanio_cola(cola_t *cola){
	return queue_size(cola->cola);
}

void *apuntar_siguiente(cola_t *cola){
	return cola->cola->elements->head=cola->cola->elements->head->next;
}

void *obtener_contenido_primer_elemento(cola_t *cola){
	return cola->cola->elements->head->data;
}

int cola_vacia(cola_t *cola){
	return queue_is_empty(cola->cola);
}

void pushear_cola_pos_determinada(cola_t *cola, void *elemento, int indice) {
	pthread_mutex_lock(&(cola->acceso));
	list_add_in_index((cola->cola->elements), indice, elemento);
	//sem_post(&(cola->elementos_disponibles));
	pthread_mutex_unlock(&(cola->acceso));
}

void *obtener_contenido_pos_determinada(cola_t* cola, int indice){
	return list_get((cola->cola->elements), indice);
}

void *remover_cola_pos_determinada(cola_t* cola, int indice){
	//sem_wait(&(cola->elementos_disponibles));
	pthread_mutex_lock(&(cola->acceso));
	void* elemento = list_remove((cola->cola->elements), indice);
	pthread_mutex_unlock(&(cola->acceso));
	return elemento;
}


void* intentar_remover_de_cola(cola_t* cola){
	int var = 0;
	pthread_mutex_lock(&(cola->acceso));
	//sem_getvalue(&(cola->elementos_disponibles),&var);
	if(var > 0){
		//sem_wait(&(cola->elementos_disponibles));
		void* elemento = queue_pop(cola->cola);
		pthread_mutex_unlock(&(cola->acceso));
		return elemento;
	}else{
		pthread_mutex_unlock(&(cola->acceso));
		return NULL;
	}
}


