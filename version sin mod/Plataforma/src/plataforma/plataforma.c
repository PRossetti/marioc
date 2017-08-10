
#include <stdlib.h>
#include "pthread.h"
#include <stdint.h>
#include "auxiliar.h"
#include <stdio.h>
#include <stdarg.h>
#include "plataforma.h"
#include "commons/log.h"
#include "variablesGlobales.h"
#include "commons/log.h"
#include <signal.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>



int main(int argc, char* argv[]){
//char* dir_koopa;
	pthread_t thread_orquestador;
	direccion=string_duplicate((char*)argv[1]);
	//dir_koopa=string_duplicate((char*)argv[2]);
	pthread_create(&thread_orquestador, NULL, Orquestador,NULL);

	pthread_join(thread_orquestador,NULL);

	//LLAMAR A KOOPA
	printf("VA A COMENZAR KOOPA\n");

	char* argumentos[2]; // con [3] no funciona ...

	argumentos[0] = string_duplicate("koopa");
	argumentos[1] = string_duplicate("/home/utnso/workspace/grasa_Aux/Debug/tmp/");
	argumentos[2] = string_duplicate("/home/utnso/workspace/grasa_Aux/Debug/script1.sh");

	if(execv("/home/utnso/workspace/grasa_Aux/Debug/koopa",argumentos) == -1)
		perror("Error en el execv");


	return EXIT_SUCCESS;
}
