
#include "auxiliares.h"


int tamanio_matriz(char** array){
	int i=0;
	while(array[i]!=NULL){
		i++;
	}
	return i;
}


int tamanio_array(char* cadena){
	int i=0;
	char** array;

	array=(char**)string_split(cadena, ",");
	while(array[i]!=NULL){
		i++;
	}
	return i;
}


int elevarALa2(int valor) {
	return (valor*valor);
}


