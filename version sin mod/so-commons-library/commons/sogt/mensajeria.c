#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mensajeria.h"
#include "../string.h"
//#include <stdint.h>

char* itoa(int valor)
{
	char* string = malloc(sizeof(valor));
	int valorRetorno;

	valorRetorno = snprintf(string,32,"%d",valor);

	if(valorRetorno == -1) return("-1");

	return(string);
}


MSJ* crearMensaje(){
	MSJ* msj = (MSJ*)malloc(sizeof(struct mensaje));
	msj->longitudMensaje=0;
	msj->tipoMensaje=-1;
	msj->mensaje = string_new();

	return msj;
}

void liberarMensaje(MSJ* msj){
	free(msj->mensaje);
	free(msj);
}
