
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/sogt/mensajeria.h>
#include <commons/sogt/sockets.h>
#include <pthread.h>
#include <stdint.h>
#include "commons/string.h"
#include "commons/config.h"
#include "commons/log.h"
#include <stdarg.h>
#include "commons/error.h"
#include "../colas.h"
#include "auxiliar.h"
#include "variablesGlobales.h"
#include "plataforma.h"
#include <sys/inotify.h>




void* Orquestador() {


	MSJ* mensaje=(MSJ*)malloc(sizeof(MSJ));
	char* msj;

	int idSocket;
	char* ip;
	int puerto;
	int i;
	int error;
	int cantPersonajesFinalizados=0;
	int cantPersonajesEnJuego=0;
	int desconectar=0;

	tipoNiveles* unNivel=NULL;
	tipoNiveles* elementoNivel=NULL;

	pthread_t planif; //= NULL;

	char* personajes=string_new(); //chequear

	char** vector; // la uso para guardar los mensajes que recibo dsp de esplitearlos
	char* message = string_new();


	t_log* logOrquestador=(t_log*)malloc(sizeof(t_log));
	logOrquestador = log_create("logOrquestador.txt","Orquestador",true,LOG_LEVEL_DEBUG);

	configPlataforma=config_create(direccion);
	//quantum = config_get_int_value(configPlataforma,"quantum");
	ip=config_get_string_value(configPlataforma, "IP");
	puerto=atoi(config_get_string_value(configPlataforma, "PUERTO"));


	log_debug(logOrquestador, "Creé el archivo de configuración.");

	t_socket_servidor* orquestador=crearServidor(ip,puerto,logOrquestador);//crear socket orquestador y poner en escucha
	log_debug(logOrquestador, "Creé el servidor y espero conexión.");

	niveles=(UnNivel*)malloc(sizeof(UnNivel));
	niveles->nivelABuscar="";
	niveles->colaNiveles=crear_cola();

	while(1) {
		idSocket = multiplexarClientes(orquestador,logOrquestador);

		if(idSocket>0){
			mensaje=recibirMensaje(idSocket,logOrquestador);
			switch(mensaje->tipoMensaje){
				case  e_handshake: //handshake nivel
					//msj q envia nivel="nivel14:127.0.0.1:5001";
					// HAY COSAS DE MAS
					elementoNivel=crearElemento(); //malloquea
					//vector=string_split(mensaje->mensaje, ":"); //espliteo el mensaje que recibi
					message = mensaje->mensaje;

					log_debug(logOrquestador, "Se registró un nivel nuevo: %s",message);

					niveles->nivelABuscar=message;
					elementoNivel->nivel=message;
					//elementoNivel->ipPuertoNivel=concat(3,vector[1],":",vector[2]);
					puerto++;
					elementoNivel->ipPuertoPlan=concat(3,ip,":",itoa(puerto));
					elementoNivel->seCreoSockPlan=0;
					elementoNivel->colaBloqueados=crear_cola(); //ya no se usa
					elementoNivel->colaFinalizados=crear_cola(); //ya no se usa
					elementoNivel->colaListos=crear_cola(); //ya no se usa
					elementoNivel->idSock=idSocket;
					//elementoNivel->pjEjec=NULL; //ya no se usa
					pushear_cola(niveles->colaNiveles, elementoNivel); //aca agrego el nivel a la cola de niveles

					if (pthread_create(&planif, NULL, planificador,NULL) != 0){
						log_error(logOrquestador, "No se pudo crear el hilo planificador.");
					}else{
						log_debug(logOrquestador, "Se creó el hilo planificador.");
					}

					enviarMensaje(idSocket,e_ok,itoa(puerto),logOrquestador);
					log_debug(logOrquestador, "Le envie el puerto: %d del planificador al nivel",puerto);
					break;

				case e_ipPuerto:	//HANDSHAKE DEL PERSONAJE --> RECIBO UN MSJ TIPO "Nivel11:@"

					vector=string_split(mensaje->mensaje,":");
					niveles->nivelABuscar=vector[0];
					unNivel=encontrarUnNivel(niveles); // busco el nivel en la cola de niveles

					if(unNivel!=NULL) {
						if(unNivel->seCreoSockPlan) {
							//msj=concat(3, unNivel->ipPuertoNivel, ":", unNivel->ipPuertoPlan);

							msj = strdup(unNivel->ipPuertoPlan);
							agregarPersonaje(personajes, vector[1]);
				    		enviarMensaje(idSocket,e_ipPuerto,msj,logOrquestador);
				    		log_debug(logOrquestador, "Se envió ip y puerto del plani al personaje.");
				       	}
					}else{
						enviarMensaje(idSocket,e_error,"",logOrquestador);//ESTE MSJ SE ENVIA SI NO FUE CREADO EL SOCKET TODAVIA
						log_debug(logOrquestador, "El %s no se creó hasta el momento.",vector[0]);
					}
				    break;
				case e_finalizoPlanNiveles:
					log_debug(logOrquestador,"Un personaje finalizó el plan d niveles.");
					cantPersonajesFinalizados++;
					cantPersonajesEnJuego=tamanio_string(personajes,":");
					desecharSocket(idSocket,orquestador);
					if(cantPersonajesFinalizados==cantPersonajesEnJuego){
						desconectar=1;
						log_debug(logOrquestador,"Finalizaron todos los personajes; accediendo a koopa.");
					}else{
						cantPersonajesEnJuego=cantPersonajesEnJuego-cantPersonajesFinalizados;
						log_debug(logOrquestador,"Quedan %d personajes por finalizar.",cantPersonajesEnJuego);
					}
					break;
				case e_desconexion: //desconexion del nivel
					for (i = 0; i < (tamanio_cola(niveles->colaNiveles)); ++i) {
						unNivel=obtener_contenido_pos_determinada(niveles->colaNiveles, i);
						if(idSocket==unNivel->idSock){
							log_debug(logOrquestador, "Se desconectó un nivel.");
							error=pthread_cancel(unNivel->planificador);
							if(error){
								log_error(logOrquestador, "No se pudo cancelar el hilo planificador.");
							}else{
								log_debug(logOrquestador, "El hilo planificador se canceló satisfactoriamente.");
								//cerrarSockets(unNivel);
								unNivel=remover_cola_pos_determinada(niveles->colaNiveles , i);
							}
							break;
						}
					}
					desecharSocket(idSocket, orquestador);
					break;
				default: break;
				}
		}
		if(desconectar) {
			log_debug(logOrquestador, "Empieza Koopa");
			log_destroy(logOrquestador);
			break;
		}
	}

	return NULL;
}
