#include "nivel.h"
#include "Gui/tad_items.h"
#include <stdlib.h>
#include <curses.h>
#include <time.h> // para time()
#include <string.h>
#include <math.h> // para sqrt

#define EVENT_SIZE  (sizeof (struct inotify_event))
#define BUF_LEN        (16 * (EVENT_SIZE + 16))

//--- Inicio: Declaracion variables globales -----------

/*
typedef struct {
	pthread_mutex_t mutex;
	char* muertos;
}t_pjMuertos;

t_pjMuertos pjMuertos;
*/

// NUEVO PABLO
typedef struct {
	t_list* lista;
	pthread_mutex_t mutex;
}t_muertos;

t_muertos* listaMuertos;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_conectados = PTHREAD_MUTEX_INITIALIZER;

t_list* items = NULL;
t_list* listaDeConectados = NULL;

char* nombre;
int uSleep_Enemigos;
short int personajesConectados = 0;
int rows,cols;
int cantidadCajas = 0;

int FD; // para monitorizar archivo
t_socket_cliente* Socket_planif;
char** string_split(char*,char*);

char* ip_orquestador; //global
int puertoPlanif; //global
int puerto_nivel_cliente; //glogal
char* ip_nivel;

//---------- Fin: Declaracion variables globales -----------

t_recursosDelPersonaje* crearPersonaje(){
	t_recursosDelPersonaje* elemento=(t_recursosDelPersonaje*)malloc(sizeof(t_recursosDelPersonaje));
	return elemento;
}

t_recursosDelPersonaje* buscoPersonajePorSimbolo(t_list* unaLista,char* simbolo){
	int i=0;
	t_recursosDelPersonaje* elemento=(t_recursosDelPersonaje*)malloc(sizeof(t_recursosDelPersonaje));

	for (i = 0; i < (list_size(unaLista)); ++i) {
		elemento=list_get(unaLista, i);
		if(simbolo[0]==elemento->simbolo[0]){
			//log_debug(logger,"encontre el personaje: %s\n",elemento->simbolo);
			break;
			}
		}

	return elemento;
}

t_recursosDelPersonaje* encontrarUnPersonaje(t_list* unaLista, int socket){

	int i=0;
	t_recursosDelPersonaje* elemento=(t_recursosDelPersonaje*)malloc(sizeof(t_recursosDelPersonaje));

	for (i = 0; i < (list_size(unaLista)); ++i) {
		elemento=list_get(unaLista, i);
		if(socket==elemento->socket){
			//log_debug(logger,"encontre el personaje: %s\n",elemento->simbolo);
			break;
			}
		}

	return elemento;
}

int buscarIndiceDePersonaje(t_list* unaLista,t_recursosDelPersonaje* unPersonaje){
	int indice;
	t_recursosDelPersonaje* elemento=(t_recursosDelPersonaje*)malloc(sizeof(t_recursosDelPersonaje));

	for (indice = 0; indice < (list_size(unaLista)); ++indice) {
		elemento=list_get(unaLista, indice);		//Busco personaje
		if(elemento->simbolo[0]==unPersonaje->simbolo[0]){
			break;
		}
	}

	return indice;
}

void* chequeoInterbloqueo(){

	t_recursosDelPersonaje* pj,*pjAux;
	t_list* listaInterbloqueados,* listaAuxiliar, *potencialInterbloqueados;
	int i,j,indice;
	char* rr;
	t_config* configThread=config_create(path_config);
	int tiempoChequeoDeadlock=config_get_int_value(configThread, "TiempoChequeoDeadlock");
	int recovery=config_get_int_value(configThread, "Recovery");

	t_socket_cliente* s_planif;
	puerto_nivel_cliente= puerto_nivel_cliente+2;
	s_planif = crearCliente(ip_nivel,puerto_nivel_cliente,logger); // hace falta inicializar aca?
	conectarCliente(s_planif,ip_orquestador,puertoPlanif,logger);

	while(1){


		sleep(tiempoChequeoDeadlock);

		if(list_size(personajesBloqueados->personajesBloqueados)>1){

			log_debug(logger,"(h_Ibq) Hay personajes bloqueados");

			listaAuxiliar=list_create();
			listaInterbloqueados=list_create();
			potencialInterbloqueados=list_create();

			pj=crearPersonaje();
			pjAux=crearPersonaje();


			//Armo una nueva lista auxiliar con personajesBloqueados
			pthread_mutex_lock(&(personajesBloqueados->acceso));
			listaAuxiliar=list_take(personajesBloqueados->personajesBloqueados, personajesBloqueados->personajesBloqueados->elements_count);
			pthread_mutex_unlock(&(personajesBloqueados->acceso));

			log_debug(logger,"(h_Ibq) Creo la cola auxiliar con %i personajes\n",list_size(listaAuxiliar));
			log_debug(logger,"(h_Ibq) Asigno 1 a los procesos que tengan RecursosAsignados en NULL");

			indice=0;


			while(indice<list_size(listaAuxiliar)){		//Asigno 0 a los procesos que tengan RecursosAsignados en NULL
				pj=list_get(listaAuxiliar,indice);
				if(pj->recursosAsignados->elements_count==0){ //Si no tiene recursos asignados, no puede estar interbloqueado
					pj->Interbloqueo=0;
				}
				else {
					pj->Interbloqueo=1;
				}
				indice++;
			}


			while(1){

				for(indice=0;indice<list_size(listaAuxiliar);indice++){ //Busco al personaje que tenga interbloqueo = 1
					pj=list_get(listaAuxiliar,indice);
					if(pj->Interbloqueo==1){
						list_add(potencialInterbloqueados,pj);
					}
				}

				// ------------------------- PRIMER CHEQUEO -------------------------
				for(indice=0;indice<list_size(potencialInterbloqueados);indice++){
					pj=list_get(potencialInterbloqueados,indice);

					for(i=0;i<list_size(potencialInterbloqueados);i++) {
						pjAux=list_get(potencialInterbloqueados,i);

						if( pjAux->simbolo[0] != pj->simbolo[0] ) {
							for( j=0;j<list_size(pjAux->recursosAsignados);j++ ) {
								rr = list_get(pjAux->recursosAsignados,j);
								if( pj->recursoSolicitado[0] == rr[0] ) {
									break;
								}

							}
							if (j == list_size(pjAux->recursosAsignados)) {
								pj->Interbloqueo = 0;
							} else {
								pj->Interbloqueo = 1;
								break;
							}
						}
					}
					if (pj->Interbloqueo == 0) {
						list_remove(potencialInterbloqueados,indice);
						indice = 0;
					}
				}

				// ------------------------- SEGUNDO CHEQUEO -------------------------
				for(indice=0;indice<list_size(potencialInterbloqueados);indice++){ //Busco al personaje que tenga interbloqueo = 1
					pj=list_get(potencialInterbloqueados,indice);
					for(i=0;i<list_size(pj->recursosAsignados);i++) {
						rr=list_get(pj->recursosAsignados,i);
						for(j=0;j<list_size(potencialInterbloqueados);j++) {
							pjAux = list_get(potencialInterbloqueados,j);
							if (pjAux->simbolo[0] != pj->simbolo[0] && rr[0] == pjAux->recursoSolicitado[0] ) {
								break;
							}
						}
						if ( j == list_size(potencialInterbloqueados) ) {
							pj->Interbloqueo = 0;
						} else {
							pj->Interbloqueo = 1;
							break;
						}
					}
					if (pj->Interbloqueo == 0) {
						list_remove(potencialInterbloqueados,indice);
						indice = 0;
					}
				}
				break;

			} // fin while(1)


			log_debug(logger,"(h_Ibq) Sali del algoritmo");
			indice=0;
			while(indice<list_size(potencialInterbloqueados)){	//Si el personaje tiene interbloqueo 1, lo meto en la cola de interbloqueados
				pj=list_get(potencialInterbloqueados,indice);
				list_add(listaInterbloqueados,pj); //aca lo agrega a la lista de interbloqueados
				indice++;
			}

			if(recovery==1){			//Si el recovery esta activado(=1) entonces debo avisar del interbloqueo
										//si el recovery esta activado, tengo que matar un personaje
				log_debug(logger,"(h_Ibq) Esta encendido el recovery");
				// LOGUEO DE PRUEBA
				if(list_size(listaInterbloqueados)>1){		//Quiere decir que hay pjs interbloqueados
					log_debug(logger,"(h_Ibq) Hay personajes interbloqueados");

					for (i=0; i < list_size(listaInterbloqueados); i++ ) {
						pjAux = list_get(listaInterbloqueados,i);
						log_debug(logger,"(h_Ibq) pj: %s recurso en espera: %s", pjAux->simbolo,pjAux->recursoSolicitado);
						int j;
						log_debug(logger,"(h_Ibq) Los recursos asignados del pj son");
						for (j=0; j < list_size(pjAux->recursosAsignados); j++) {
							rr = list_get(pjAux->recursosAsignados,j);
							log_debug(logger,"(h_Ibq) recurso: %s",rr);
						}

					}
				// FIN LOGUEO DE PRUEBA
					log_debug(logger,"(h_Ibq) ******LOGGEO PERSONAJES INTERBLOQUEADOS******");
					for (i=0; i < list_size(listaInterbloqueados) ;i++) {
						pjAux = list_get(listaInterbloqueados,i);
						log_debug(logger, "Interbloqueado %d %s",i,pjAux->simbolo);
					}
					log_debug(logger,"(h_Ibq) ****FIN LOGGEO PERSONAJES INTERBLOQUEADOS****");
					// Logica de seleccion de victima

					int ordenLlegada = 99;
					int indice;
					i = 0;
					while ( i < list_size(listaInterbloqueados) ) {
						pjAux = list_get(listaInterbloqueados,i);
						if ( pjAux->orden < ordenLlegada ) {
							ordenLlegada = pjAux->orden;
							pj = pjAux;
							indice = i;
						}
						i++;
					}
					list_remove(listaInterbloqueados,indice);
					// Fin logica de seleccion de vicitima

					t_recursosDelPersonaje* personaje = crearPersonaje();

					pthread_mutex_lock(&personajesBloqueados->acceso);
					personaje = buscoPersonajePorSimbolo(personajesBloqueados->personajesBloqueados, pj->simbolo);
					indice = buscarIndiceDePersonaje(personajesBloqueados->personajesBloqueados, personaje);
					list_remove(personajesBloqueados->personajesBloqueados,indice);
					pthread_mutex_unlock(&personajesBloqueados->acceso);


					pthread_mutex_lock(&m_conectados);
					for (i=0; i<list_size(listaDeConectados);i++ ) {
						personaje = list_get(listaDeConectados,i);
						if ( personaje->simbolo[0] == pj->simbolo[0] ) {
							break;
						}
					}
					list_remove(listaDeConectados,i);//mata al pj
					personajesConectados--;
					pthread_mutex_unlock(&m_conectados);

					pthread_mutex_lock( &mutex1 );
						BorrarItem(items, pj->simbolo[0]);
						nivel_gui_dibujar(items, nombre);
					pthread_mutex_unlock( &mutex1 );

					// Aviso al planificador la victima

					log_debug(logger,"(h_Ibq) Voy a enviar la victima al plani");
					enviarMensaje(s_planif->sock->idSocket,e_matar_interbloq, pj->simbolo,logger);
					log_debug(logger,"(h_Ibq) Le avisé la victima: %s al plani", pj->simbolo);
					log_debug(logger,"(h_Ibq) Empieza la logica de liberacion");

				} else log_debug(logger,"(h_Ibq) Pero no hay personajes interbloqueados");

			} // Fin if recovery



		}//Fin if personajesBloqueados>0

	}//Fin while

		list_destroy_and_destroy_elements(listaAuxiliar,free);
		list_destroy_and_destroy_elements(listaInterbloqueados,free);
		list_destroy_and_destroy_elements(potencialInterbloqueados,free);
		config_destroy(configThread);

	return 0;
}//Fin thread

int dentroDeLosLimites(int x, int y) {
	//devuelve 1 si esta dentro de los limites, 0 si no lo esta

	if ( (x <= cols) && (x > 0) && (y <= rows) && (y > 0) ) {
		return 1;
	}

	return 0;
}

int hayCaja(unsigned int x, unsigned int y) {
	//devuelve 1 si hay caja, 0 si no hay
	int i;

	for (i = 0; i < cantidadCajas; i++) {
		if ( (x == contenidoNivel[i].posX) && (y == contenidoNivel[i].posY) ) {
			return 1;
		}
	}

	return 0;
}

void* logicaEnemigo(char* ptr) {

	t_log* log_enemigo;
	log_enemigo = log_create("logEnemigo","main.c",0,LOG_LEVEL_DEBUG);
	float distPE;
	char eje = 'x';

	typedef struct posiciones {
		int x;
		int y;
	} t_posiciones;

	t_posiciones arr[8];

	char idEnemigo = *ptr;
	int x, y, xAux, yAux;
	int pisaCaja = 1;

	char* cPJ;
	int xPJ, yPJ;

	int indice;
	t_recursosDelPersonaje* pjXY; //tengo que hacer mallocs para estos dos?
	pjXY = crearPersonaje();
	t_recursosDelPersonaje* pjAuxXY;
	pjAuxXY = crearPersonaje();

	float distAux;
	int aux;
	int i;
	char* caracter;

	//printf("cajas: %d caracter: %s",cantidadCajas, idEnemigo);
	while ( pisaCaja ) {
		x = rand() % 78 + 1;
		y = rand() % 19 + 1;
		pisaCaja = hayCaja(x,y);
	}
	pthread_mutex_lock( &mutex1 );
	CrearEnemigo(items, idEnemigo, x, y);
	nivel_gui_dibujar(items, nombre);
	pthread_mutex_unlock( &mutex1 );
	// cree un enemigo y lo dibuje

	arr[0].x = 1;
	arr[0].y = 2;
	arr[1].x = 1;
	arr[1].y = -2;
	arr[2].x = -1;
	arr[2].y = 2;
	arr[3].x = -1;
	arr[3].y = -2;
	arr[4].x = 2;
	arr[4].y = 1;
	arr[5].x = 2;
	arr[5].y = -1;
	arr[6].x = -2;
	arr[6].y = 1;
	arr[7].x = -2;
	arr[7].y = -1;

	int xDestino, yDestino;

	while (1) {
		while (personajesConectados <= 1) {
			// si no hay personajes conectados
			int posFinal = 1, buscarMovimiento = 1;
			while ( buscarMovimiento == 1 ) {

				xAux = x;
				yAux = y;

				while (posFinal == 1) {

					aux = rand() % 8;
					xDestino = x + arr[aux].x;
					yDestino = y + arr[aux].y;

					posFinal = hayCaja(xDestino, yDestino);

					if ( dentroDeLosLimites(xDestino,yDestino) == 0 ) //si no esta dentro de los limites
						posFinal = 1; // posFinal = 1 significa que la pos es invalida

					if (posFinal)
						log_debug(log_enemigo, "Pos FINAL INVALIDA");

				}

				log_debug(log_enemigo, "Encontre POS FINAL VALIDA");


				while ( (xAux != xDestino) && (hayCaja(xAux,y)==0) ) {
					if (xDestino > xAux) {
						xAux++;
						log_debug(log_enemigo, "1 Mov Der Valido");
					} else {
						xAux--;
						log_debug(log_enemigo, "1 Mov Izq Valido");
					}
				}

				if ( hayCaja(xAux,y)==0 ) {
					while ( (yAux != yDestino) && (hayCaja(xAux,yAux)==0) ) {
						if (yDestino > y) {
							yAux++;
							log_debug(log_enemigo, "1 Mov DOWN Valido");
						} else {
							yAux--;
							log_debug(log_enemigo, "1 Mov UP Valido");
						}
					}
				} //cierro el if

				//buscarMovimiento = hayCaja(xAux,yAux);
				/*
				if ( hayCaja(xAux,yAux) ) { // si no sirvio empezando por x ahora prueba empezando por y
					xAux = x;
					yAux = y;
					while ( (yAux != yDestino) && (hayCaja(x,yAux) == 0) ) {
						if (yDestino > y) {
							yAux++;
							log_debug(log_enemigo, "1 Mov DOWN Valido");
						} else {
							yAux--;
							log_debug(log_enemigo, "1 Mov UP Valido");
						}
					}
					if ( hayCaja(x,yAux) == 0 ) {
						while ( (xAux != xDestino) && (hayCaja(xAux,yAux) == 0) ) {
							if (xDestino > xAux) {
								xAux++;
								log_debug(log_enemigo, "1 Mov Der Valido");
							} else {
								xAux--;
								log_debug(log_enemigo, "1 Mov Izq Valido");
							}
						}
					}
				}
				*/
				buscarMovimiento = hayCaja(xAux,yAux);
				if (buscarMovimiento) {
					log_debug(log_enemigo, "Movimientos intermedios invalidos");
					posFinal = 1;
				}

			} // fin while buscarMovimiento

			log_debug(log_enemigo, "Todo valido");


			while ( (x != xDestino) && (personajesConectados <= 1) ) {
				if (xDestino > x) {
					x++;
					log_debug(log_enemigo, "Se mueve uno a la derecha");
				} else {
					x--;
					log_debug(log_enemigo, "Se mueve uno a la izquierda");
				}
				usleep(uSleep_Enemigos); //sleepEnemigos

				pthread_mutex_lock( &mutex1 );
				MoverEnemigo(items, idEnemigo, x, y);
				nivel_gui_dibujar(items, nombre);
				pthread_mutex_unlock( &mutex1 );
			}

			while ( (y != yDestino) && (personajesConectados <= 1) ) {
				if (yDestino > y) {
					y++;
					log_debug(log_enemigo, "Se mueve uno para abajo");
				} else {
					y--;
					log_debug(log_enemigo, "Se mueve uno para arriba");
				}
				usleep(uSleep_Enemigos); //sleepEnemigos

				pthread_mutex_lock( &mutex1 );
				MoverEnemigo(items, idEnemigo, x, y);
				nivel_gui_dibujar(items, nombre);
				pthread_mutex_unlock( &mutex1 );
			}


		} // chierro el while( personajesConectados==0)


		while ( personajesConectados > 1 ) {

			log_debug(log_enemigo, "Hay personajes conectados");

			distPE = 9999;
			pthread_mutex_lock(&m_conectados);
			for (indice = 0; indice < (list_size(listaDeConectados)); indice++) {
				//log_debug(log_enemigo, "Voy a calcular la dist. Tamanio lista %d",list_size(listaDeConectados));
				pjAuxXY = list_get(listaDeConectados,indice);
				if (pjAuxXY->bloqueado == 0) { // si no esta bloqueado
					distAux = sqrt( elevarALa2(pjAuxXY->x - x ) + elevarALa2(pjAuxXY->y - y) );
					if( distAux < distPE ){
						distPE = distAux;
						pjXY = pjAuxXY;
					}
				}
			}
			pthread_mutex_unlock(&m_conectados);
			// deberia poner un semaforo para que el pj hacia el que voy, no se mueva hasta que el enemigo evalue
			//pthread_mutex_lock(&pjXY->mutex);
			cPJ = strdup(pjXY->simbolo);
			xPJ = pjXY->x;
			yPJ = pjXY->y;
			log_debug(log_enemigo,"Soy el hilo %c y voy al PJ %s x:%d y:%d dist: %f\n",idEnemigo, cPJ, xPJ, yPJ, distPE);

			//PRUEBA -- Voy a ver si la lista esta bien para los hilos

			log_debug(log_enemigo,"VERIFICAR ABAJO LA LISTA");
			for (indice = 0; indice < (list_size(listaDeConectados)); indice++) {
				pjAuxXY = list_get(listaDeConectados,indice);
				log_debug(log_enemigo,"Elemento %d de la ListaPosConectados es %s %d %d bloqueado:%d",indice,
						pjAuxXY->simbolo,pjAuxXY->x,pjAuxXY->y,pjAuxXY->bloqueado);
			}

			// FIN PRUEBA

			switch( eje ) {
				case 'x' :
					//if (personajesConectados <= 1) break;
					moverEnX:
					if ( ( xPJ > x ) && ((x+1) <= cols) && (hayCaja(x+1,y)==0) ) {
						x++; //se mueve a la derecha
						//if (personajesConectados <= 1) break;
						log_debug(log_enemigo, "Se mueve a la derecha");
					} else if ( ( xPJ < x ) && ((x-1) > 0) && (hayCaja(x-1,y)==0) ) {
						x--;//se mueve a la izquierda
						//if (personajesConectados <= 1) break;
						log_debug(log_enemigo, "Se mueve a la izquierda");
					} else { // aca viene si xPJ = x o si se fue de margen
						if ( (x==xPJ) && (y==yPJ) ) {
							log_debug(log_enemigo, "Me comi a: %s", cPJ);

							pthread_mutex_lock(&listaMuertos->mutex);

							for (i=0; i < list_size(listaMuertos->lista); i++) {
								caracter = list_get(listaMuertos->lista,i);
								if (cPJ[0] == caracter[0]) {
									break;
								}
							}
							if ( i == list_size(listaMuertos->lista) ) { // si esto se cumple, el pj no estaba en la lista
								list_add(listaMuertos->lista, cPJ);		 // y lo voy a agregar
							}
							pthread_mutex_unlock(&listaMuertos->mutex);


							//Notificar al planificador que me comi al personaje
						} else if ( (y==yPJ) && ( (hayCaja(x+1,y)==0) || (hayCaja(x-1,y)==0) ) ){
							// mover aleatorio en y
							if ( hayCaja(x,y+1) || dentroDeLosLimites(x,y+1)==0 ) { // chequear si hay caja en y++ si hay moverme en y--
								if ( (hayCaja(x,y-1) == 0) && dentroDeLosLimites(x,y-1) ) {
									y--;
								} else {
									break; // no tengo para donde moverme, agregar eje = 'y'; ?
								}
							} else y++;
							eje = 'x';
							break;
						} else {
							log_debug(log_enemigo, "Estoy en la misma X que el pj asi que me voy a mover en Y acercandome");
							goto moverEnY;
						}
					}
					eje = 'y';
					break;
				case 'y' :
					//if (personajesConectados <= 1) break;
					moverEnY:
					if ( (yPJ > y) && ((y+1) <= rows) && (hayCaja(x,y+1)==0) )  {
						y++; //se mueve hacia abajo
						//if (personajesConectados <= 1) break;
						log_debug(log_enemigo, "Se mueve para abajo");
					} else if ( (yPJ < y) && ((y-1) > 0) && (hayCaja(x,y-1)==0) ) {
						y--; //se mueve hacia arriba
						//if (personajesConectados <= 1) break;
						log_debug(log_enemigo, "Se mueve para arriba");
					} else { //aca viene si yPJ = y o si se fue de margen
						if ( (y==yPJ) && (x==xPJ) ) {
							log_debug(log_enemigo, "Me comi a: %s", cPJ);

							pthread_mutex_lock(&listaMuertos->mutex);
							char* caracter;
							for (i=0; i < list_size(listaMuertos->lista); i++) {
								caracter = list_get(listaMuertos->lista,i);
								if (cPJ[0] == caracter[0]) {
									break;
								}
							}
							if ( i == list_size(listaMuertos->lista) ) { // si esto se cumple, el pj no estaba en la lista
								list_add(listaMuertos->lista, cPJ);		 // y lo voy a agregar
							}
							pthread_mutex_unlock(&listaMuertos->mutex);

							//Notificar al planificador que me comi al personaje
						} else if ( (x==xPJ) && ( (hayCaja(x,y+1)==0) || (hayCaja(x,y-1)==0) ) ) {
							//mover aleatorio en X
							if ( hayCaja(x+1,y) || dentroDeLosLimites(x+1,y)==0 ) { // chequear si hay caja en x++ si hay moverme en x--
								if ( (hayCaja(x-1,y) == 0) && dentroDeLosLimites(x-1,y) ) {
									x--;
								} else {
									break; // no tengo para donde moverme, agregar eje = 'x'; ?
								}
							} else x++;
							eje = 'y';
							break;
						} else { // seguramente sobre el mismo eje Y que el enemigo
							log_debug(log_enemigo, "Estoy en el mismo Y que el pj asi que me voy a mover en X acercandome");
							goto moverEnX;
						}
					}
					eje = 'x';
					/* borrar desde aca
					for (indice = 0; indice < (list_size(listaDeConectados)); indice++) {
						log_debug(log_enemigo, "Voy a recorrer lista de pos PJS para verificar si lo piso. Tamanio lista %d",list_size(listaDeConectados));
						pjAuxXY = list_get(listaDeConectados,indice);

						if ( (x==pjAuxXY->x) && (y==pjAuxXY->y) ) {
							log_debug(log_enemigo, "Pise a: %s", pjAuxXY->simbolo);
							//enviarMensaje(Socket_planif->sock->idSocket,e_muerte,pjAuxXY->simbolo,logger);
						}

					}
					hasta aca */
					break;
			} // fin switch eje


			pthread_mutex_lock( &mutex1 );
			MoverEnemigo(items, idEnemigo, x, y );
			nivel_gui_dibujar(items, nombre);
			pthread_mutex_unlock( &mutex1 );
			//pthread_mutex_unlock(&pjXY->mutex); // fin semaforo pj

			log_debug(log_enemigo,"Me acabo de mover y ahora voy a volver al ppio del while (personajesConectados > 1)");
			usleep(uSleep_Enemigos); // sleep enemigos
		} // fin while ( personajesConectados > 1 )

	} //cierro el while (1)
	pthread_exit(NULL);
}

int monitorizarArchivo(t_log* logito) {
	fd_set descriptores;
	int actividad;
    char buf[BUF_LEN];
    int len;
    struct timeval time;
	// timeout after five seconds
	time.tv_sec = 0.1;
	time.tv_usec = 0;
	//zero-out the fd_set
	FD_ZERO (&descriptores);

	FD_SET (FD, &descriptores);

	actividad = select (FD + 1, &descriptores, NULL, NULL, &time);

	if (actividad < 0) {
		log_debug(logito,"Error en el select del archivo");
		return 0;
		//perror("Error en select");
	} else if ( !actividad ) { //fin tiempo de espera
		//printf("Fin tiempo de espera\n"); //return -2;
		return 0;
	} else if ( FD_ISSET(FD, &descriptores) ) { //evento de inotify
		//printf("Hubo un evento\n");
		len = read(FD, buf, BUF_LEN);

		if (len > 0)
		{
			int i = 0;
			while (i < len)
			{
				struct inotify_event *event;
				event = (struct inotify_event *) &buf[i];

				//printf("wd=%d mask=%x cookie=%u len=%u name=%s\n",
				//	event->wd, event->mask,
				//	event->cookie, event->len, "test");


				if (event->mask & IN_MODIFY) {
					log_debug(logito,"Se modifico el archivo, voy a return 1");
					printf("Se modifico el archivo");
					//printf("file modified %s", "test");
					return 1;
				}

				if (event->len) {
					//printf("name=%s\n", event->name);

				}
				i += EVENT_SIZE + event->len;
			}
		} // fin if len>0
	} //fin if FD_ISSET
	return 0;
}

int main(int argc, char* argv[]){



	//pthread_mutex_init(&pjMuertos.mutex,NULL);
	//pjMuertos.muertos= strdup("");

	listaMuertos = (t_muertos*)malloc(sizeof(t_muertos));
	listaMuertos->lista = list_create();
	pthread_mutex_init(&listaMuertos->mutex,NULL);


	t_socket_cliente* nivel_cliente;
	MSJ* mensajeRecibido, *msjPlanif;//, *mensajeOrquestador;
	t_recursosDelPersonaje* personaje,*personajeBloqueado;

	path_config=argv[1];
	t_config* configNivel=config_create(path_config);
	nombre=config_get_string_value(configNivel, "Nombre");
	char* orquestador=config_get_string_value(configNivel,"orquestador");
	char** ipYpuertoOrquestador=string_split(orquestador,":");

	ip_orquestador= ipYpuertoOrquestador[0]; //global

	int puerto_orquestador=atoi(ipYpuertoOrquestador[1]);
	cantidadCajas=config_get_int_value(configNivel, "cantidadCajas");
	ip_nivel=config_get_string_value(configNivel,"ipNivel");
	puerto_nivel_cliente=config_get_int_value(configNivel, "puertoCliente");
	char** movimiento=string_split("@,XX,YY",",");
	int cantEnemigos = config_get_int_value(configNivel, "Enemigos");
	int sleep_enemigos = config_get_int_value(configNivel, "Sleep_Enemigos"); // en milisegundos
	char* algoritmo = config_get_string_value(configNivel,"algoritmo");
	int quantum = config_get_int_value(configNivel,"quantum");
	int retardo = config_get_int_value(configNivel,"retardo");

	char** recursoNivel;
	short int conexion;
	int actividadSocket=-1;
	char* cajax="";
	char* recurso="";
	int i,j;
	int posicionX,posicionY;
	char* posXY="";
	int indice;
	char* num="";
	int	numero;
	char** elMensaje=NULL;
	char* nombrelog;
	char* str_victimas = strdup("");

	pthread_t thread_chequeoInterbloqueo;
	char** recursosLiberados;
	char** personajesLiberados;
	t_recursosDelPersonaje* pjInterbloq;
	pjInterbloq = crearPersonaje();
	t_recursosDelPersonaje* pjXY;
	char* msj;

	pthread_t t_enemigo[cantEnemigos];
	char enemySymbol = 49;
	char symbols[cantEnemigos];
	uSleep_Enemigos = sleep_enemigos*1000; // uSleep_Enemigos es GLOBAL

	t_list* victimas;

	srand(time(NULL));

	//---------- Inicio: Creacion del log -----------

	nombrelog=concat(3,"log",nombre,".txt");
	logger = log_create(nombrelog,nombre,false,LOG_LEVEL_DEBUG);


	//---------- Inicio: GUI del Nivel -----------

	items = list_create();
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

	//---------- Inicio: Vector con datos de los recursos del nivel -----------

	for(i=0;i<cantidadCajas;i++){
		contenidoNivel[i].inicial="";
		contenidoNivel[i].instancias=0;
		contenidoNivel[i].posX=0;
		contenidoNivel[i].posY=0;
	}

	//---------- Inicio: Vector con datos del archivo de config -----------

	for(i=0;i<cantidadCajas;i++){
		num=itoa(i+1);
		cajax= concat(2,"Caja",(char*)num);
		recursoNivel=config_get_array_value(configNivel,cajax);
		contenidoNivel[i].inicial=recursoNivel[1];
		numero=atoi(recursoNivel[2]);
		contenidoNivel[i].instancias=numero;
		contenidoNivel[i].posX=atoi(recursoNivel[3]);
		contenidoNivel[i].posY=atoi(recursoNivel[4]);
		CrearCaja(items, contenidoNivel[i].inicial[0], contenidoNivel[i].posX, contenidoNivel[i].posY, numero);
	}

	for(i=0;i<cantidadCajas;i++){
		log_debug(logger,"Recurso: %s instancias: %d PosX: %d PosY: %d\n",contenidoNivel[i].inicial,contenidoNivel[i].instancias,contenidoNivel[i].posX,contenidoNivel[i].posY);
	}
	log_debug(logger,"algoritmo: %s quantum: %d retardo: %d \n",algoritmo,quantum,retardo);

	pthread_mutex_lock( &mutex1 );
	nivel_gui_dibujar(items, nombre);
	pthread_mutex_unlock( &mutex1 );

	//---------- Inicio: Inotify -----------

    FD = inotify_init();
    if (FD < 0)
        perror("inotify_init()");

    int wd;
    wd = inotify_add_watch(FD, path_config, IN_MODIFY);
    if (wd < 0)
        perror("inotify_add_watch");
    int act;
	//---------- Fin: Inotify -----------


	//---------- Inicio: Hilos enemigos -----------

	int e;
	usleep(300000);

	for (e = 0;  e < cantEnemigos; e++) {
		symbols[e] = enemySymbol;
		pthread_create( &t_enemigo[e], NULL, (void*)logicaEnemigo, &symbols[e]);
		enemySymbol++;
	}


	//---------- Inicio: Socket cliente y conexion con el Orquestador -----------

	nivel_cliente=crearCliente(ip_nivel,puerto_nivel_cliente,logger);
	idSocketThread=nivel_cliente->sock->idSocket;
	log_debug(logger,ip_orquestador);
	log_debug(logger,"%d\n",puerto_orquestador);
	conexion=1;
	//Se va a conecta al orquestador
	if(conexion==1){
		if(conectarCliente(nivel_cliente,ip_orquestador,puerto_orquestador,logger)==-1){
				log_debug(logger,"No me conecte\n");
		}
		else 	log_debug(logger,"Me conecte");
	}

	// HANDSHAKE con Orquestador (aviso mi nombre)
	if(enviarMensaje(nivel_cliente->sock->idSocket,e_handshake,nombre,logger)==-1){
		log_debug(logger,"Fallo envio.");

	}
	else {
		log_debug(logger,"Le mande:%s\n",nombre);
		mensajeRecibido=recibirMensaje(nivel_cliente->sock->idSocket,logger);

		puertoPlanif = atoi(mensajeRecibido->mensaje); //no pido la ip porque tiene que ser la misma que el orquestador
		log_debug(logger,"El puerto del planificador es: %d",puertoPlanif);

	}
	free(mensajeRecibido);
	// cierro la conexion con un close ?

	//---------- Inicio: Conexion con planificador -----------

	int meConecte = 0;
	while ( meConecte != 1 ) {
		Socket_planif = crearCliente(ip_nivel,puerto_nivel_cliente+1,logger);
		if(conexion==1) {
			if(conectarCliente(Socket_planif, ip_orquestador, puertoPlanif, logger)==-1) {
				log_debug(logger,"No me pude conectar al planificador");
				meConecte = 0;
			} else {
				log_debug(logger,"Me conecte al planificador");
				meConecte = 1;
			}
		}
	}

	//---------- Inicio: Handshake con planificador -----------

	if ( strcmp(algoritmo,"RR") == 0 ) {

		msj = concat(5,algoritmo, "," ,itoa(quantum),",",itoa(retardo));
		enviarMensaje(Socket_planif->sock->idSocket, e_RR, msj,logger);
		log_debug(logger,"Envie msj al planif y el algoritmo es :%s",algoritmo);

	} else if (strcmp(algoritmo,"SRDF") == 0 ) {

		msj = concat(5,algoritmo, "," , "1",",",itoa(retardo)); // coloco quamtum 1 si el algoritmo es SRDF
		enviarMensaje(Socket_planif->sock->idSocket, e_SRDF, msj,logger);
		log_debug(logger,"Envie msj al planif y el algoritmo es :%s",algoritmo);
	}

	msjPlanif = recibirMensaje(Socket_planif->sock->idSocket,logger); //se va a bloquear hasta recibir el ok
	log_debug(logger,"Se recibio msj del planif: %s de tipo: %d",msjPlanif->mensaje, msjPlanif->tipoMensaje);
	free(msjPlanif);
	free(msj);

	//---------- Inicio: Logica conexion con personaje -----------

	listaDeConectados=list_create();
	pjXY = crearPersonaje();
	personajesBloqueados=(tipoLista*)malloc(sizeof(tipoLista));
	personajesBloqueados->personajesBloqueados=list_create();

	pthread_mutex_init(&(personajesBloqueados->acceso),NULL);
	pthread_create(&thread_chequeoInterbloqueo, NULL, chequeoInterbloqueo,NULL);	//Lanzo el hilo

	personajesConectados=1;

	while(personajesConectados!=-1){

		actividadSocket = Socket_planif->sock->idSocket;

			log_debug(logger,"Espero mensaje del planificador");
			pthread_mutex_lock( &mutex1 );
			nivel_gui_dibujar(items,nombre);
			pthread_mutex_unlock( &mutex1 );
			mensajeRecibido=recibirMensaje(Socket_planif->sock->idSocket,logger); //se bloquea

			log_debug(logger,"Recibi el mensaje: %s de tipo: %d",mensajeRecibido->mensaje, mensajeRecibido->tipoMensaje);

			//---------- Inicio: Analisis de Mensajes -----------

			switch(mensajeRecibido->tipoMensaje){

			//---------- Caso: Peticion de coordenadas  -----------

			case e_coordenadas:

				log_debug(logger,"Me piden coordenadas");

				i=0;
				while(mensajeRecibido->mensaje[0]!=contenidoNivel[i].inicial[0]){		//Busco las coordenadas en el array
					i++;
				}

				posicionX= contenidoNivel[i].posX;
				posicionY= contenidoNivel[i].posY;
				if(posicionX<=cols && posicionY<= rows){			//Me fijo si las coordenadas no superan los limites
					posXY= concat(3,itoa(posicionX),":",itoa(posicionY));
					log_debug(logger,posXY);
					enviarMensaje(actividadSocket, e_coordenadas,posXY,logger);	//Mando mensaje con las coordenadas al personaje
				}

			break;

			//---------- Caso: Peticion de recurso -----------

			case e_recurso:		//El personaje me pide instancia de recurso
				log_debug(logger,"Me pide una instancia de recurso");
				log_debug(logger,"En el array HAY:");
				for(i=0;i<cantidadCajas;i++){
					log_debug(logger,"Recurso:%s instancias:%d\n",contenidoNivel[i].inicial,contenidoNivel[i].instancias);
				}

				elMensaje = string_split(mensajeRecibido->mensaje,",");
				recurso=elMensaje[1];

				i=0;
				while(recurso[0]!= contenidoNivel[i].inicial[0]){	//Recorro el array hasta que caigo en el recurso solicitado
					i++;
				}
				pthread_mutex_lock(&m_conectados);
					personaje = buscoPersonajePorSimbolo(listaDeConectados,elMensaje[0]);
					personaje->recursoSolicitado=elMensaje[1];
				pthread_mutex_unlock(&m_conectados);

				if(contenidoNivel[i].instancias!=0){

					contenidoNivel[i].instancias=contenidoNivel[i].instancias-1; //Si hay instancias doy el okey y saco una instancia de ese recurso

					pthread_mutex_lock( &mutex1 );
					restarRecurso(items, contenidoNivel[i].inicial[0]);
					nivel_gui_dibujar(items, nombre);
					pthread_mutex_unlock( &mutex1 );

					list_add(personaje->recursosAsignados,recurso);	//pongo el recurso en la cola de recursos asignados al personaje
					personaje->recursoSolicitado="";		//ahora desconozco cual es el proximo recurso que el personaje necesita
					//AGREGO ESTO PARA DESBLOQUEARLO
					if (personaje->bloqueado == 1 ) {
						personaje->bloqueado = 0;
					}
					enviarMensaje(actividadSocket,e_ok,"",logger);
				}
				else {
					log_debug(logger,"Hay un bloqueo");
					personajeBloqueado=crearPersonaje();
					personajeBloqueado=personaje;
					personaje->bloqueado=1;
					personajeBloqueado->bloqueado=1;
					enviarMensaje(actividadSocket,e_error,"",logger);	//le digo que hay un bloqueo con este personaje

					pthread_mutex_lock(&(personajesBloqueados->acceso));
					list_add(personajesBloqueados->personajesBloqueados,personajeBloqueado);		//mando a la cola de personajes bloqueados esperando ese recurso
					pthread_mutex_unlock(&(personajesBloqueados->acceso));


					log_debug(logger,"Hay %i personajes bloqueados\n",list_size(personajesBloqueados->personajesBloqueados));

				}

				log_debug(logger,"En el array quedan:");
				for(i=0;i<cantidadCajas;i++){
					log_debug(logger,"Recurso:%s instancias:%d\n",contenidoNivel[i].inicial,contenidoNivel[i].instancias);
				}
				free(mensajeRecibido);
			break;

			//---------- Caso: Handshake -----------

			case e_handshake: //voy a crear el nodo del personaje

				log_debug(logger,"Llego un handshake de PJ");

				pthread_mutex_lock(&m_conectados);
				personajesConectados++;
				pthread_mutex_unlock(&m_conectados);

				personaje=crearPersonaje();
				elMensaje=string_split(mensajeRecibido->mensaje,":"); //Aca me separa el Mario:@:1
				personaje->simbolo=elMensaje[1];
				personaje->Interbloqueo=0;
				personaje->bloqueado=0;
				personaje->socket=actividadSocket;
				personaje->recursoSolicitado="";
				personaje->recursosAsignados=list_create();
				personaje->x = 1;
				personaje->y = 1;
				personaje->orden = atoi(elMensaje[2]);

				log_debug(logger,personaje->simbolo); //le agrego el simbolo del personaje

				pthread_mutex_lock(&m_conectados);
				list_add(listaDeConectados,personaje); //lo agrego a la lisa de personajes conectados
				pthread_mutex_unlock(&m_conectados);

				pthread_mutex_lock( &mutex1 );
				CrearPersonaje(items, personaje->simbolo[0], 1, 1);
				nivel_gui_dibujar(items, nombre);
				pthread_mutex_unlock( &mutex1 );

				enviarMensaje(Socket_planif->sock->idSocket,e_ok,"Respondo handshake",logger); // enviar okey


			break;

			//---------- Caso: Desconexion -----------

			case e_desconexion: case e_finalizoNivel:		//cuando se desconecta un personaje

				if(mensajeRecibido->tipoMensaje==e_desconexion)
				{
					log_debug(logger,"Desconexion repentina");
				}
				else {
					log_debug(logger,"Finalizo el nivel un personaje");
				}


			// Lo encuentro y lo saco de la lista de conectados

					pthread_mutex_lock(&m_conectados);
					for (indice = 0; indice < (list_size(listaDeConectados)); ++indice) {
						personaje=list_get(listaDeConectados,indice);		//Busco personaje
						if(mensajeRecibido->mensaje[0]==personaje->simbolo[0]){
							break;
						}
					}
					personaje=list_remove(listaDeConectados,indice);
					pthread_mutex_unlock(&m_conectados);

					// BEGIN PRUEBA
					char *recu;
					log_debug(logger,"personaje: %s cant recursos asignados: %d",personaje->simbolo,list_size(personaje->recursosAsignados));
					log_debug(logger,"los recursos asignados son");
					for(i=0; i < list_size(personaje->recursosAsignados); i++) {
						recu = list_get(personaje->recursosAsignados,i);
						log_debug(logger,"recurso: %c",recu[0]);
					}
					// FIN PRUEBA

					pthread_mutex_lock( &mutex1 );
					BorrarItem(items, personaje->simbolo[0]);
					nivel_gui_dibujar(items, nombre);
					pthread_mutex_unlock( &mutex1 );

			// Si esta en la cola de bloqueados tambien lo saco de ahi
					if(personaje->bloqueado==1){
						for (indice = 0; indice < (list_size(personajesBloqueados->personajesBloqueados)); ++indice) {
							personaje=list_get(personajesBloqueados->personajesBloqueados,indice);	//Busco personaje
							if(mensajeRecibido->mensaje[0]==personaje->simbolo[0]){
								break;
							}
						}
						pthread_mutex_lock(&(personajesBloqueados->acceso));
							list_remove(personajesBloqueados->personajesBloqueados,indice);
						pthread_mutex_unlock(&(personajesBloqueados->acceso));
					}

					pthread_mutex_lock( &m_conectados);
					personajesConectados--;
					pthread_mutex_unlock( &m_conectados);

					pthread_mutex_lock( &mutex1 );
					nivel_gui_dibujar(items, nombre);
					pthread_mutex_unlock( &mutex1 );

					log_debug(logger,"En la lista de conectados quedan: %i personajes\n",listaDeConectados->elements_count);
					log_debug(logger,"Quedan %i personajes bloqueados\n",list_size(personajesBloqueados->personajesBloqueados));
					// mensaje para logica de liberar recursos
					enviarMensaje(Socket_planif->sock->idSocket,e_ok,"Libera todo",logger);
					// devuelvo los recursos al nivel

			break;


			//---------- Caso: Movimiento -----------

			case e_movimiento:
				log_debug(logger,"Entre al case e_movimiento: %d y el msj: %s",e_movimiento, mensajeRecibido->mensaje);


				movimiento=string_split(mensajeRecibido->mensaje,",");
				// por aca deberia chequear si no esta muerto con un if, si no esta muerto no hago nada, si esta muerto
				// ahi tengo que agregar algo de logica
				log_debug(logger,"Hice el split: %c %d %d", movimiento[0][0], atoi(movimiento[1]),atoi(movimiento[2]) );

				// -----------***PABLO*** -----------
				//pjXY = crearPersonaje();
				pthread_mutex_lock(&m_conectados);
				for (indice = 0; indice < (list_size(listaDeConectados)); indice++) {
					pjXY = list_get(listaDeConectados,indice); //***PABLO***
					if( movimiento[0][0] == pjXY->simbolo[0] ){
						break;
					}
				}
				pthread_mutex_unlock(&m_conectados);
				//pthread_mutex_lock(&pjXY->mutex);
				pjXY->x = atoi(movimiento[1]);
				pjXY->y = atoi(movimiento[2]);

				//pjXY = list_get(listaDeConectados,indice);

				//log_debug(logger,"clave %s %d %d",pjXY->simbolo,pjXY->x,pjXY->y);

				pthread_mutex_lock( &mutex1 );
				MoverPersonaje(items, movimiento[0][0], atoi(movimiento[1]), atoi(movimiento[2]));
				nivel_gui_dibujar(items, nombre);
				pthread_mutex_unlock( &mutex1 );
				//pthread_mutex_unlock(&pjXY->mutex);
				log_debug(logger,mensajeRecibido->mensaje);

				enviarMensaje(actividadSocket,e_ok,"",logger); //okey para el plani
				//free(pjXY); <--- por este free se rompia, aunque no entiendo por que

			break;

			case e_muerte:
				log_debug(logger,"\n");
				log_debug(logger,"************** ENTRE A LOGICA E_MUERTE **************");
				victimas = list_create();
				str_victimas = strdup("");
				pthread_mutex_lock(&listaMuertos->mutex);
				victimas = list_take(listaMuertos->lista,list_size(listaMuertos->lista)); // creo lista auxiliar
				list_clean(listaMuertos->lista);
				pthread_mutex_unlock(&listaMuertos->mutex);

				log_debug(logger,"Entre a e_muerte, cant muertos: %d ",list_size(victimas));
				//log_debug(logger,"Cantidad conectados: %d",list_size(listaDeConectados));

				//char** victimas = NULL;
				//int len;
				char* victima;


				if ( list_size(victimas) > 0 ) {	// si entra es porque murio uno o mas

					t_recursosDelPersonaje* elemento;
					elemento = crearPersonaje();

					log_debug(logger,"Cantidad conectados: %d personajesConectados: %d",list_size(listaDeConectados), personajesConectados);
					log_debug(logger,"Mensaje victimas: %s",str_victimas);

					log_debug(logger,"Voy a loguear los personajes");
					for(i=0;i<list_size(listaDeConectados);i++) {
						elemento = list_get(listaDeConectados,i);
						log_debug(logger,"Personaje %d es %s",i,elemento->simbolo);
					}

					log_debug(logger,"Voy a loguear las victimas");

					for(i=0;i<list_size(victimas);i++) {
						victima = list_get(victimas,i);
						log_debug(logger,"Victima %d es %s",i,victima);
					}

					log_debug(logger,"Voy a remover al (o a los) pj/s muerto/s de listaDeConectados");

					// remuevo los personajes muertos de la listaDeConectados y los borro del mapa

					pthread_mutex_lock(&m_conectados);
					for(i=0; i < list_size(victimas); i++) {

						victima = list_get(victimas,i);
						if ( strcmp(str_victimas,"") == 0 ) {
							str_victimas = strdup(victima);
						} else {
							str_victimas = concat(3,str_victimas,",",victima);
						}

						for(j=0; j<list_size(listaDeConectados); j++) {
							elemento = list_get(listaDeConectados,j);
							if ( elemento->simbolo[0] == victima[0] ) {
								break;
							}
						}

						// no pregunto por cual motivo salio del for ya que el pj deberia estar si o si en listaDeConectados
						if ( j<list_size(listaDeConectados) ) {
							list_remove(listaDeConectados,j);
							pthread_mutex_lock(&mutex1);
							BorrarItem(items,victima[0]);
							pthread_mutex_unlock(&mutex1);

							if (personajesConectados > 1) {
								personajesConectados--;
							}

						} else {
							log_debug(logger,"La victima no estaba en la lista de conectados");
							str_victimas = strdup("");
						}
					}

					log_debug(logger,"Cantidad conectados: %d personajesConectados: %d",list_size(listaDeConectados), personajesConectados);
					log_debug(logger,"Mensaje Victimas: %s",str_victimas);

					log_debug(logger,"Voy a loguear los personajes");
					for(i=0;i<list_size(listaDeConectados);i++) {
						elemento = list_get(listaDeConectados,i);
						log_debug(logger,"Personaje %d es %s",i,elemento->simbolo);
					}

					log_debug(logger,"Voy a loguear las victimas");

					for(i=0;i<list_size(victimas);i++) {
						victima = list_get(victimas,i);
						log_debug(logger,"Victima %d es %s",i,victima);
					}


					pthread_mutex_lock(&mutex1);
					nivel_gui_dibujar(items,nombre);
					pthread_mutex_unlock(&mutex1);

					pthread_mutex_unlock(&m_conectados);

					enviarMensaje(Socket_planif->sock->idSocket,e_muerte,str_victimas,logger);

				} else {
					enviarMensaje(Socket_planif->sock->idSocket,e_muerte,"",logger);
					log_debug(logger,"No murio nadie todavia, salgo de e_muerte");
				}
				list_destroy_and_destroy_elements(victimas,free); // borro la lista auxiliar

				log_debug(logger,"************** SALGO DE LOGICA E_MUERTE **************\n");

			break;

			case e_matar_interbloq:

				log_debug(logger,"Entre a e_matar_interbloq");

				log_debug(logger,"Tamanio de cadena: %d",strlen(mensajeRecibido->mensaje));

				if ( strlen(mensajeRecibido->mensaje) > 0 ) {
					recursosLiberados = string_split(mensajeRecibido->mensaje,",");
					log_debug(logger,"Tamaño recursosLiberados: %d",tamanio_matriz(recursosLiberados));
				} else {
					recursosLiberados = string_split(mensajeRecibido->mensaje,",");
					log_debug(logger,"Tamaño recursosLiberados: %d",tamanio_matriz(recursosLiberados));
					log_debug(logger,"No se devuelve ningun recurso al mapa");
				}

				enviarMensaje(Socket_planif->sock->idSocket,e_ok,"Recibi los recus a liberar",logger);
				//recibo los personajes desbloqueados para actualizarlo en mis listas

				mensajeRecibido = recibirMensaje(Socket_planif->sock->idSocket,logger);

				if(strcmp(mensajeRecibido->mensaje,"") != 0) {

					personajesLiberados = string_split(mensajeRecibido->mensaje,",");

					enviarMensaje(Socket_planif->sock->idSocket,e_ok,"Recibi los personajes desinterbloqueados",logger);

					log_debug(logger,"personajesLiberados: %s", personajesLiberados[0]);
					//voy a desbloquear a los personajes que corresponda segun el mensaje del plani

					log_debug(logger,"Tamaño personajesBloqueados: %d Tamaño personajesLiberados: %d",

					list_size(personajesBloqueados->personajesBloqueados),tamanio_matriz(personajesLiberados));

					// los saco de la lista de Interbloqueados y lo agrego a la de conectados

					pthread_mutex_lock(&personajesBloqueados->acceso);
					for (i=0; i<list_size(personajesBloqueados->personajesBloqueados);i++ ) {

						pjInterbloq = list_get(personajesBloqueados->personajesBloqueados,i);

						for ( j=0; j < tamanio_matriz(personajesLiberados); j++ ) {

							if ( pjInterbloq->simbolo[0] == personajesLiberados[j][0] ) {
								break;
							}
						}

						if ( j <  tamanio_matriz(personajesLiberados)  ) {
							list_remove(personajesBloqueados->personajesBloqueados,i);
							i--;

							pjInterbloq->Interbloqueo = 0;
							pjInterbloq->bloqueado = 0;
							list_add(pjInterbloq->recursosAsignados,pjInterbloq->recursoSolicitado); //chequear
							// tendria que agregarle un semaforo tambien a esta lista por ser global, la usan los enemigos
							// y el hilo de interbloqueo
							pthread_mutex_lock(&m_conectados);
								list_add(listaDeConectados,pjInterbloq);
							pthread_mutex_unlock(&m_conectados);
						}
					}
					pthread_mutex_unlock(&personajesBloqueados->acceso);

				} else {
					enviarMensaje(Socket_planif->sock->idSocket,e_ok,"No habia personajes para desbloquear",logger);
				}

				// devuelvo al mapa los recursos que se liberaron
				for ( i=0; i < tamanio_matriz(recursosLiberados); i++ )
				{
					j=0;
					//actualizo el vector
					log_debug(logger,"%c,%c", recursosLiberados[i][0],contenidoNivel[j].inicial[0] );
					while(recursosLiberados[i][0] != contenidoNivel[j].inicial[0]) {
						j++;
						log_debug(logger,"%c,%c", recursosLiberados[i][0],contenidoNivel[j].inicial[0] );
					}

						contenidoNivel[j].instancias++;
						pthread_mutex_lock( &mutex1 );
							sumarRecurso(items,recursosLiberados[i][0]);
						pthread_mutex_unlock( &mutex1 );
				}

				log_debug(logger,"Fin de logica de liberar recursos y pj");
				break;

			default:
				log_debug(logger,"Entre por el default, el mensaje fue %s, y el tipo: %d",mensajeRecibido->mensaje,mensajeRecibido->tipoMensaje);
			break;

		//Hasta aca son todos los mensajes con actividadSocket!=0

			} //Llave del switch

			//---------- Fin: Analisis de Mensajes -----------


			//---------- Inicio: Monitoreo y actividades correspondientes -----------

		    act = monitorizarArchivo(logger);

			if ( act == 1 ){
				log_debug(logger,"act == %d ",act);
				configNivel=config_create(path_config);

				char* algoritmoAnt = strdup(algoritmo);
				int quantumAnt = quantum;
				int retardoAnt = retardo;

				algoritmo = config_get_string_value(configNivel,"algoritmo");
				quantum = config_get_int_value(configNivel,"quantum");
				retardo = config_get_int_value(configNivel,"retardo");
				int sleep_enemigos_aux = config_get_int_value(configNivel, "Sleep_Enemigos"); // PABLO

				if ( strcmp(algoritmo,algoritmoAnt) != 0 ) {
					log_debug(logger,"Cambio el algoritmo: %s",algoritmo);
				}
				if ( quantum != quantumAnt ) {
					log_debug(logger,"Cambio el quantum: %d",quantum);
				}
				if ( retardo != retardoAnt ) {
					log_debug(logger,"Cambio el retardo: %d",retardo);
				}
				if ( (strcmp(algoritmo,algoritmoAnt) != 0) || (quantum != quantumAnt) || (retardo != retardoAnt) ) {
					if ( strcmp(algoritmo,"RR") == 0 ) {
						msj = concat(5,algoritmo, "," ,itoa(quantum),",",itoa(retardo));
						enviarMensaje(Socket_planif->sock->idSocket, e_RR, msj,logger);
						//recibirMensaje(Socket_planif->sock->idSocket,logger);
						log_debug(logger,"Envie msj al planif y el algoritmo es: %s",algoritmo);
					} else if (strcmp(algoritmo,"SRDF") == 0 ) {
						msj = concat(5,algoritmo, "," , "1",",",itoa(retardo)); // coloco quamtum 1 si el algoritmo es SRDF
						enviarMensaje(Socket_planif->sock->idSocket, e_SRDF, msj,logger);
						//recibirMensaje(Socket_planif->sock->idSocket,logger);
						log_debug(logger,"Envie msj al planif y el algoritmo es: %s",algoritmo);
					}
				}
				free(algoritmoAnt);
				// BEGIN PABLO
				if ( sleep_enemigos != sleep_enemigos_aux ) {
					log_debug(logger,"Cambio el Sleep Enemigos: %d",sleep_enemigos_aux);
					sleep_enemigos = sleep_enemigos_aux;
					uSleep_Enemigos = sleep_enemigos*1000;
				}
				// END PABLO
			} else {
				log_debug(logger,"No cambio el archivo");
			}

			//---------- Fin: Monitoreo y actividades correspondientes -----------

	}//Llave del while

	log_destroy(logger);
	nivel_gui_terminar();
	return 0;

}
