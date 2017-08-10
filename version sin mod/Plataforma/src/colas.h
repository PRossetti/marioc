
#ifndef COLAS_H_
#define COLAS_H_

#include <commons/collections/queue.h>
#include <pthread.h>
//#include "plataforma/auxiliar.h"

typedef struct {
	t_queue* cola; //cola de personajes bloqueados
	//sem_t elementos_disponibles;
	pthread_mutex_t acceso;
} cola_t;



cola_t* crear_cola();
void pushear_cola(cola_t *, void *);
void *remover_de_cola(cola_t *);
int tamanio_cola(cola_t *);
void *apuntar_siguiente(cola_t *);
void *obtener_contenido_primer_elemento(cola_t *);
int cola_vacia(cola_t *);
void pushear_cola_pos_determinada(cola_t *, void *, int);
void *obtener_contenido_pos_determinada(cola_t *, int);
void *remover_cola_pos_determinada(cola_t* , int);
void* intentar_remover_de_cola(cola_t* cola);




#endif /* COLAS_H_ */
