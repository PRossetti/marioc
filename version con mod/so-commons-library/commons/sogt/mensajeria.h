#ifndef MENSAJERIA_H_
#define MENSAJERIA_H_

#include <stdint.h>

/*Listar los distintos tipos de mensaje que vamos a necesitar*/
typedef enum tipoMensaje
{
	e_desconexion, e_handshake, e_ipPuerto, e_recurso, e_coordenadas,
	e_nivel, e_movimiento, e_finalizoTurno, e_finalizoNivel, e_finalizoPlanNiveles,
	e_muerte, e_procesosInterBlockeados, e_recursosLiberados, e_ok, e_error,
	e_algoritmo, e_matar_interbloq, e_RR, e_SRDF

}TipoMensaje;

typedef struct mensaje
{
	TipoMensaje tipoMensaje;
	int longitudMensaje;
	char* mensaje;
} __attribute__((packed)) MSJ;

typedef struct {
	uint32_t tipo;
	uint32_t length;
} __attribute__((packed)) t_header ;

char* itoa(int);
MSJ* crearMensaje();
void liberarMensaje(MSJ* msj);


#endif /* MENSAJERIA_H_ */
