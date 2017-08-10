#include "Personaje.h"

int main(int argc, char* argv[]) {

	//Asignacion de rutinas para el manejo de señales
	signal(SIGTERM,rutina);
	signal(SIGUSR1,rutina);

	int i;
	int cantHilosMax = 0;
	char letra;

	char* path_config=argv[1]; //por que en NIVEl no la declara ? tengo que malloquearlo?
	Personaje=config_create(path_config);//leo la configuracion

	Orq=config_get_string_value(Personaje, "orquestador"); //GLOBAL
	ippuertoOrq = string_split(Orq,":"); // cambie esto
	ipOrq = ippuertoOrq[0]; // cambie esto
	portOrq = atoi(ippuertoOrq[1]);	//cambie esto
	Plan = config_get_array_value(Personaje, "planDeNiveles"); //vector de niveles

	//Inicializacion de semaforos
	pthread_mutex_init(&miPuerto.mutex,NULL);
	pthread_mutex_init(&Vidas.mutex,NULL);
	pthread_mutex_init(&hilosCorriendo.mutex,NULL);
	hilosCorriendo.hilos_corriendo = 0; // PABLO


	miIp = config_get_string_value(Personaje, "miIp");
	miPuerto.puerto = config_get_int_value(Personaje, "miPuerto");

	char* nom_log= concat(3, "log_", config_get_string_value(Personaje, "nombre"), ".txt");
	t_log* logger = log_create(nom_log, config_get_string_value(Personaje, "nombre"),true,LOG_LEVEL_DEBUG);

	int cantNiveles = 1; //la inicializo en 1 para que entre al while

	while(cantNiveles > 0)
	{
		pthread_mutex_lock(&Vidas.mutex);
			Vidas.vidas=config_get_int_value(Personaje, "vidas"); //TIENE QUE SER GLOBAL PARA LOS HILOS
		pthread_mutex_unlock(&Vidas.mutex);
		Plan = config_get_array_value(Personaje, "planDeNiveles"); //vector de niveles
		cantNiveles = tamanio_array(config_get_string_value(Personaje, "planDeNiveles"));
		pthread_t hilo_personaje[cantNiveles];
		cantHilosMax = 0; // ya lo inicialice arriba

		//Inicializo la list de puertos de planificadores
		l_puertosPlanif = list_create();

		//levanto un hilo de pj por cada nivel
		log_debug(logger,"Voy a empezar a levantar hilos de pj");

		for (i = 0; (i < cantNiveles); i++) {
			pthread_mutex_lock(&hilosCorriendo.mutex);
				hilosCorriendo.hilos_corriendo++;
				cantHilosMax++;
			pthread_mutex_unlock(&hilosCorriendo.mutex);
			pthread_create(&hilo_personaje[i], NULL, (void*)procPersonaje,(void*)Plan[i]);
			log_debug(logger,"Levante el hilo %d con el plan %s", i, Plan[i]);
		}
		log_debug(logger,"Voy a esperarlos");

		while(hilosCorriendo.hilos_corriendo > 0)
		{
			sleep(1); //sleep para que no ??

			if(cantNiveles != hilosCorriendo.hilos_corriendo || Vidas.vidas <= 0) {
				cantNiveles = hilosCorriendo.hilos_corriendo; // si cantNiveles != hilos_corriendo entonces algun hilo termino
															  // actualizo cantNiveles

				if ( Vidas.vidas <= 0 ) {
					log_debug(logger,"No quedan mas vidas por usar, voy a eliminar a los hilos");
					for (i = 0; (i < cantHilosMax); i++) {
						log_debug(logger, "Voy a cancelar un hilo y cerrar un socket");
						pthread_cancel(hilo_personaje[i]); // no hay problema aca si el hilo ya habia terminado?
						t_puertosPlanif* aux; // no esta muy lindo declararlo aca
						aux = list_get(l_puertosPlanif,i);
						close(aux->socketPlanificador->sock->idSocket);
						log_debug(logger, "Desconecto el personaje con id %d", i);

					}
					pthread_mutex_lock(&hilosCorriendo.mutex);
						hilosCorriendo.hilos_corriendo = 0;
					pthread_mutex_unlock(&hilosCorriendo.mutex);
					letra = '0';

					log_debug(logger,"¿Desea Reiniciar? (Y/N) ");

					while ( letra != 'N' &&
							letra != 'n' &&
							letra != 'Y' &&
							letra != 'y' )
					{
						scanf("%c", &letra);
					}
					if((letra == 'Y') || (letra == 'y') )
					{
						cantNiveles = 1;
						pthread_mutex_lock(&Vidas.mutex);
							Vidas.vidas = config_get_int_value(Personaje, "vidas");
						pthread_mutex_unlock(&Vidas.mutex);

					} else if (letra == 'N' || letra == 'n') {
						cantNiveles = -1;
						list_clean(l_puertosPlanif); //PABLO
					} else {
						log_debug(logger,"la letra que puso fue %c, la cual es erronea", letra);
					}
					// si las vidas llegan a 0 tengo que preguntar pantalla si sigo o no, si sigo puedo elegir entre reiniciar
					//el personaje o seguir el juego sin el, si no sigo, terminaria .tod.o y no se ganaria


				} else {
					log_debug(logger,"Finalizo un Hilo su nivel, faltan finalizar %d",hilosCorriendo.hilos_corriendo);
				}
			}

		}
	}

	log_debug(logger,"Terminaron todos los hilos");

	if(cantNiveles == 0 )
	{
		// BEGIN PABLO
		//t_socket_cliente* s_orq_main = crearCliente(miIp, miPuerto.puerto, logger);
		//conectarCliente(s_orq_main, ipOrq, portOrq, logger);//me conecto con el Orquestador
		//enviarMensaje(s_orq_main->sock->idSocket,e_finalizoPlanNiveles, "", logger);
		// END PABLO
		log_debug(logger, "------------------%s TERMINO PLAN DE NIVELES--------------",config_get_string_value(Personaje, "nombre"));
	}
	else
	{
		log_debug(logger, "------------------%s NO TERMINO PLAN DE NIVELES--------------",config_get_string_value(Personaje, "nombre"));
	}
	config_destroy(Personaje);
	log_destroy(logger);


	return 0;
}

//Rutina de señales

void rutina (int n) {
	switch (n) {
		case SIGTERM:
			pthread_mutex_lock(&Vidas.mutex);
				Vidas.vidas--;
			pthread_mutex_unlock(&Vidas.mutex);
			printf("Gano una vida por señal quedan %d", Vidas.vidas);

		break;

		case SIGUSR1:

			pthread_mutex_lock(&Vidas.mutex);
				Vidas.vidas++;
			pthread_mutex_unlock(&Vidas.mutex);
			printf("Gano una vida por señal quedan %d", Vidas.vidas);

		break;
	}
}

void* procPersonaje( void* ptr ) {

	t_socket_cliente* s_orq;
	t_socket_cliente* s_planif;

	t_puertosPlanif* puertoPlanif; //variable para la lista
	puertoPlanif = malloc(sizeof(t_puertosPlanif));

	char* miNivel;
	miNivel =(char*)ptr;
	tieneQueMorirsePorUnaSenial=0;
	ganoUnaVidaPorUnaSenial=0;
	int n = 1; //representa el nivel actual
	int r=0; //representa el recurso actual
	int tengoDestino=0; //para saber si se a donde voy
	int posX, posY, faltanEnX, faltanEnY; //posiciones del personaje + las que faltan para el prox recurso.
	int movimientoAlternado = 1; //variable para alternar el moviento en los ejes
	char* new_pos; //hacia donde me movere.
	int llegue=0; // indica si ya llegue al recurso que quiero
	int esperoUnRecurso=0; //indica si estoy esperando un recurso.
	char* key; //la uso para pasar parametros con concatenacion de strings
	char** mision=NULL; //vector de recursos a conseguir en el nivel
	char* misionAux;
	int cantRecursos;
	char* ipPlanif; //, *ipNiv;
	int portPlanif; //, portNiv;

	char* presentacion;
	char* nom_log= concat(5, "log_", config_get_string_value(Personaje, "nombre"),"_hilo_",miNivel, ".txt");
	t_log* logger = log_create(nom_log, config_get_string_value(Personaje, "nombre"),true,LOG_LEVEL_DEBUG);

	// INICIALIZO LOS PUERTOS DE CLIENTE
	log_debug(logger, "Valor de miPuerto.mutex: %d",miPuerto.puerto); // PABLO
	pthread_mutex_lock(&miPuerto.mutex);

	miPuerto.puerto++;

	int port_cliOrq = miPuerto.puerto;

	miPuerto.puerto++;

	int port_cliPlani = miPuerto.puerto;

	pthread_mutex_unlock(&miPuerto.mutex);
	log_debug(logger, "Valor de miPuerto.mutex: %d",miPuerto.puerto); // PABLO


	log_debug(logger,"Ip Orq: %s Puerto: %d",ipOrq,portOrq); //agregue esto pero dsp borrar

	char* auxPeticionOrq;
	char* simbolo = config_get_string_value(Personaje, "simbolo");
	char** datosDelPlanificador, **coordenadas; //datosDelNivel,

	MSJ* bufPlanif;
	MSJ* bufOrq;

	//inicializo sockets:

	s_orq = crearCliente(miIp, port_cliOrq, logger); // hace falta inicializar aca?
	s_planif= crearCliente(miIp, port_cliPlani, logger); // y aca?



	while ( n != -1 )
	{ //condicion adaptada a la original, es como un while 1, si N vale 0 sera porque termino el nivel:

		iniciarNivel://etiqueta
		tengoDestino=0; // igual a 0 porque no tengo destino
		// INICIAR_NIVEL

		s_orq = crearCliente(miIp, port_cliOrq, logger);
		conectarCliente(s_orq, ipOrq, portOrq, logger);//me conecto con el Orquestador
		//pthread_mutex_unlock(&miPuerto.mutex);

		repreguntarPorNivel://Etiqueta
		log_debug(logger, "Entre a repreguntarPorNivel");
		log_debug(logger, "Mi plan es %s",miNivel);

		auxPeticionOrq=concat(3, miNivel, ":", config_get_string_value(Personaje, "simbolo"));

		log_debug(logger, "asigne valor a auxPeticionOrq");
		enviarMensaje(s_orq->sock->idSocket,e_ipPuerto ,auxPeticionOrq, logger); //le pido los datos del plani del nivel

		free(auxPeticionOrq);
		log_debug(logger,"Pido datos del: %s",miNivel);

		bufOrq=recibirMensaje( s_orq->sock->idSocket,logger);//RECV blockeante esperando los datos del PLANI

		log_debug(logger,"Recibi del orquestador bufOrq= %s",bufOrq->mensaje);

		log_debug(logger,"IpPuertos del plani: %s Tipo Mensaje: %d",bufOrq->mensaje, bufOrq->tipoMensaje);

		if (bufOrq->tipoMensaje!=e_ipPuerto) {
			if (bufOrq->tipoMensaje==e_error) {
				log_debug(logger, "El nivel no esta, pruebo de nuevo en 10 segundos");
				sleep(10);
				goto repreguntarPorNivel;
			}else{
				log_debug(logger, "Llego un msj incorrecto. Pincho.");
				log_destroy(logger);
				//return 0;
				pthread_exit(NULL);
			}
		}//si fue otro error, pincho!

		close(s_orq->sock->idSocket);//desconecto del orquestador

		datosDelPlanificador=(char**)string_split(bufOrq->mensaje, ":");
		ipPlanif=(char*)string_duplicate(datosDelPlanificador[0]); //podia usar strdup...
		portPlanif=atoi(datosDelPlanificador[1]);
		free(datosDelPlanificador);

		// ME VOY A CONECTAR AL PLANIFICADOR DEL NIVEL
		s_planif = crearCliente(miIp, port_cliPlani, logger);
		conectarCliente(s_planif, ipPlanif, portPlanif, logger);
		puertoPlanif->idHilo = pthread_self(); //le agrego el id del hilo
		puertoPlanif->socketPlanificador = s_planif;
		//agrego a la lista el puerto del planificador
		pthread_mutex_lock(&m_puertos);
		list_add(l_puertosPlanif,puertoPlanif);
		pthread_mutex_unlock(&m_puertos);
		// INICIALIZACIONES DEL PERSONAJE
		log_debug(logger,"Me voy a inicializar");
		posX=1;
		posY=1;
		key= concat(3, "obj[", miNivel, "]");

		mision = config_get_array_value(Personaje, key);
		misionAux = config_get_string_value(Personaje, key);
		cantRecursos = tamanio_array(misionAux);
		log_debug(logger,"Termine de inicializar");
		// FIN INALIZACIONES DEL PERSONAJE

		presentacion=concat(5, config_get_string_value(Personaje, "nombre"), ":", simbolo, ":",mision[0]);
		//HANDSHAKE CON PLANIFICADOR
		enviarMensaje(s_planif->sock->idSocket,e_handshake, presentacion, logger); //mando mensaje de tipo "Mario:@:H"
		log_debug(logger, "Mando handshake al planif del %s", miNivel);
		bufPlanif = recibirMensaje(s_planif->sock->idSocket,logger); //espero el ok del handskahe

		// ---------------------------------CHEQUEO SI MORI--------------------------------------
		if (bufPlanif->tipoMensaje==e_muerte) {

			log_debug(logger,"Entre a e_muerte");

			pthread_mutex_lock(&Vidas.mutex);
				Vidas.vidas--;
			pthread_mutex_unlock(&Vidas.mutex);

			if(Vidas.vidas <= 0) {
				sleep(5);
				log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
				close(s_planif->sock->idSocket);

				goto iniciarNivel;
			} else {
				//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
				log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
				close(s_planif->sock->idSocket);
				goto iniciarNivel;
				//No me quedan mas vidas
			}
		}
		//------------------------------------------------------------------------------------

		if (bufPlanif->tipoMensaje == e_ok ) {
			log_debug(logger, "Me respondio: %s", bufPlanif->mensaje);
		} else {
			log_debug(logger, "1 ERROR en la respuesta del Plani por el Handskahe, tipo: %d",bufPlanif->tipoMensaje);
		}

		for (r=0; r<cantRecursos;r++)
		{ //por cada recurso.
			llegue=0;
			esperoUnRecurso=0;
			while(!llegue || (esperoUnRecurso == 1) ) {
				//---------------------------------ESCUCHA ATENTO A SEÑALES--------------------------------------

				bufPlanif=recibirMensaje( s_planif->sock->idSocket,logger);
				// ---------------------------------CHEQUEO SI MORI--------------------------------------
				if (bufPlanif->tipoMensaje==e_muerte) {

					log_debug(logger,"Entre a e_muerte");

					pthread_mutex_lock(&Vidas.mutex);
						Vidas.vidas--;
					pthread_mutex_unlock(&Vidas.mutex);

					if(Vidas.vidas <= 0) {
						sleep(5);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
					} else {
						//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
						//No me quedan mas vidas
					}
				}
				//------------------------------------------------------------------------------------
				// RECV BLOCKEANTE ESPERANDO TURNO DEL PLANIFICADOR
				log_debug(logger, "Se supone que el plani me dio turno, Mensaje: %s Tipo: %d", bufPlanif->mensaje, bufPlanif->tipoMensaje);

				if (bufPlanif->tipoMensaje==-1) {
					if (errno == EINTR){ //si fue por una senial..
						errno=0;
					}
				}

				/*
				if (bufPlanif->tipoMensaje==-1){
					log_debug(logger, "Se perdió la coneccion con el nivel.");
					goto iniciarNivel;

				} else
				*/

				if (bufPlanif->tipoMensaje!=e_movimiento) {
					log_debug(logger, "Llego un msj incorrecto");
					log_destroy(logger);
					//return 0;
					pthread_exit(NULL);
				}
				log_debug(logger, "El plani me avisa que me toca: %s Tipo msj: %d",bufPlanif->mensaje,bufPlanif->tipoMensaje);
				log_debug(logger, "Me toca!"); // SI ESTOY ACA, ME TOCA!!
				free(bufPlanif->mensaje);

				if (!tengoDestino)
				{ // si no tengo destino entro a este if y pido las coords
					log_debug(logger, "Pido coordenadas del recurso!");

					enviarMensaje(s_planif->sock->idSocket,e_coordenadas,mision[r], logger); //PIDO UBICACION AL PLANI

					bufPlanif = recibirMensaje( s_planif->sock->idSocket, logger);//ESPERO RESPUESTA (las coords) -me bloqueo
					// ---------------------------------CHEQUEO SI MORI--------------------------------------
					if (bufPlanif->tipoMensaje==e_muerte) {

						log_debug(logger,"Entre a e_muerte");

						pthread_mutex_lock(&Vidas.mutex);
							Vidas.vidas--;
						pthread_mutex_unlock(&Vidas.mutex);

						if(Vidas.vidas <= 0) {
							sleep(5);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
						} else {
							//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
							//No me quedan mas vidas
						}
					}
					//------------------------------------------------------------------------------------
					log_debug(logger,"Ubicacion del recurso: %s Tipo msj: %d",bufPlanif->mensaje, bufPlanif->tipoMensaje);

					if (bufPlanif->tipoMensaje!=e_coordenadas) {
						log_debug(logger, "Hubo un Error al recibir las coordenadas.");
						log_destroy(logger);
						return 0;
					}
					//calculo lo que falta moverme
					coordenadas=(char**)string_split(bufPlanif->mensaje, ":");
					faltanEnX= atoi(coordenadas[0])-posX;
					faltanEnY= atoi(coordenadas[1])-posY;
					free(coordenadas);
					tengoDestino=1;
					bufPlanif = recibirMensaje(s_planif->sock->idSocket,logger);
					// ---------------------------------CHEQUEO SI MORI--------------------------------------
					if (bufPlanif->tipoMensaje==e_muerte) {

						log_debug(logger,"Entre a e_muerte");

						pthread_mutex_lock(&Vidas.mutex);
							Vidas.vidas--;
						pthread_mutex_unlock(&Vidas.mutex);

						if(Vidas.vidas <= 0) {
							sleep(5);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
						} else {
							//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
							//No me quedan mas vidas
						}
					}
					//------------------------------------------------------------------------------------

				}

				if ( (faltanEnX && movimientoAlternado == 1) ||
					  (faltanEnY == 0)) { //en este IF decido hacia donde moverme
					if (faltanEnX>0)
					{
						faltanEnX--;
						posX++;
					}
					else
					{
						faltanEnX++;
						posX--;
					}
					movimientoAlternado = 0;
				}
				else if((faltanEnY && movimientoAlternado == 0) ||
						(faltanEnX == 0)) {
					if (faltanEnY>0)
					{
						faltanEnY--;
						posY++;
					}
					else
					{
						faltanEnY++;
						posY--;
					}
					movimientoAlternado = 1;
				}
				new_pos=concat(5, simbolo, ",", (char*)itoa(posX), ",", (char*)itoa(posY)); //armo msj nueva pos

				log_debug(logger, "Aviso movimiento al plani: %s", new_pos);

				enviarMensaje(s_planif->sock->idSocket,e_movimiento,new_pos,logger); //AVISO MOVIMIENTO AL PLANI

				// BEGIN PABLO

				bufPlanif = recibirMensaje(s_planif->sock->idSocket,logger); //ME BLOQUEO ESPERANDO UN OK
				//este primer if es un parche
				// ---------------------------------CHEQUEO SI MORI--------------------------------------
				if (bufPlanif->tipoMensaje==e_muerte) {

					log_debug(logger,"Entre a e_muerte");

					pthread_mutex_lock(&Vidas.mutex);
						Vidas.vidas--;
					pthread_mutex_unlock(&Vidas.mutex);

					if(Vidas.vidas <= 0) {
						sleep(5);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
					} else {
						//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
						//No me quedan mas vidas
					}
				}
				//------------------------------------------------------------------------------------

				if ( bufPlanif->tipoMensaje == e_movimiento ) {

					bufPlanif = recibirMensaje(s_planif->sock->idSocket,logger); //ME BLOQUEO ESPERANDO UN OK

				}
				// ---------------------------------CHEQUEO SI MORI--------------------------------------
				if (bufPlanif->tipoMensaje==e_muerte) {

					log_debug(logger,"Entre a e_muerte");

					pthread_mutex_lock(&Vidas.mutex);
						Vidas.vidas--;
					pthread_mutex_unlock(&Vidas.mutex);

					if(Vidas.vidas <= 0) {
						sleep(5);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
					} else {
						//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
						log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
						close(s_planif->sock->idSocket);
						//liberarCliente(
						goto iniciarNivel;
						//No me quedan mas vidas
					}
				}
				//------------------------------------------------------------------------------------

				if ( bufPlanif->tipoMensaje == e_ok) {
					log_debug(logger,"Plani me avisó que me movi OK");
				} else {
					log_debug(logger,"1 ERROR en el movimiento, tipo: %d", bufPlanif->tipoMensaje);
				}
				// END PABLO

				free(new_pos);
				free(bufPlanif);


				/*
				 ME MOVI, consumi quantum, AHORA ESPERO UN OK O ALGO QUE ME DIGA QUE SE MOVIO BIEN Y PUEDO CONTINUAR
				  (QUE TODAVIA ES MI TURNO/TENGO QUANTUM) hasta no recibirlo estoy bloqueado, no puedo pedir el recu
				*/

				if ((!faltanEnX) && (!faltanEnY)) {//si no falta nada YA LLEGUE!
					llegue=1;
					tengoDestino=0;

					bufPlanif = recibirMensaje( s_planif->sock->idSocket, logger);//ESPERO RESPUESTA para tener quantum
					// PABLO aca hay que agregar un if para controlar el tipo de mensaje, porque puede recibir un e_muerte
					// y se queda colgado
					// ---------------------------------CHEQUEO SI MORI--------------------------------------
					if (bufPlanif->tipoMensaje==e_muerte) {

						log_debug(logger,"Entre a e_muerte");

						pthread_mutex_lock(&Vidas.mutex);
							Vidas.vidas--;
						pthread_mutex_unlock(&Vidas.mutex);

						if(Vidas.vidas <= 0) {
							sleep(5);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
						} else {
							//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
							//No me quedan mas vidas
						}
					}
					//------------------------------------------------------------------------------------
					log_debug(logger, "Pido instancia de %s",mision[r]);

					enviarMensaje(s_planif->sock->idSocket,e_recurso,mision[r], logger);

					free(bufPlanif);

					bufPlanif = recibirMensaje( s_planif->sock->idSocket, logger);//ESPERO RESPUESTA, un OK por ejemplo
					// PABLO aca hay que agregar un if para controlar el tipo de mensaje, porque puede recibir un e_muerte
					// y se queda colgado
					// ---------------------------------CHEQUEO SI MORI--------------------------------------
					if (bufPlanif->tipoMensaje==e_muerte) {

						log_debug(logger,"Entre a e_muerte");

						pthread_mutex_lock(&Vidas.mutex);
							Vidas.vidas--;
						pthread_mutex_unlock(&Vidas.mutex);

						if(Vidas.vidas <= 0) {
							sleep(5);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
						} else {
							//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
							log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
							close(s_planif->sock->idSocket);
							//liberarCliente(
							goto iniciarNivel;
							//No me quedan mas vidas
						}
					}
					//------------------------------------------------------------------------------------

					log_debug(logger,"recibi el mensaje, %d",bufPlanif->tipoMensaje);

					switch (bufPlanif->tipoMensaje){
					case e_movimiento: case e_ok:
						log_debug(logger, "Aviso al planif q termine con recurso");
						//enviarMensaje(s_planif->sock->idSocket,e_finalizoTurno,"", logger);//aviso al planificardor que termine mi turno normal.
						break;
					case e_error:

						esperoUnRecurso=1;

						bufPlanif = recibirMensaje(s_planif->sock->idSocket,logger);
						// ---------------------------------CHEQUEO SI MORI--------------------------------------
						if (bufPlanif->tipoMensaje==e_muerte) {

							log_debug(logger,"Entre a e_muerte");

							pthread_mutex_lock(&Vidas.mutex);
								Vidas.vidas--;
							pthread_mutex_unlock(&Vidas.mutex);

							if(Vidas.vidas <= 0) {
								sleep(5);
								log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
								close(s_planif->sock->idSocket);
								//liberarCliente(
								goto iniciarNivel;
							} else {
								//printf("Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
								log_debug(logger, "Me mato un enemigo, me quedan %d vidas",Vidas.vidas);
								close(s_planif->sock->idSocket);
								//liberarCliente(
								goto iniciarNivel;
								//No me quedan mas vidas
							}
						}
						//------------------------------------------------------------------------------------

						//-------------------------------INTERBLOQUEO
							if(bufPlanif->tipoMensaje == e_matar_interbloq)
							{

								pthread_mutex_lock(&Vidas.mutex);
									Vidas.vidas--;
								pthread_mutex_unlock(&Vidas.mutex);

								if(Vidas.vidas <= 0)
								{
									log_debug(logger,"Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
									close(s_planif->sock->idSocket);
									goto iniciarNivel;

								}
								else
								{
									log_debug(logger,"Mori por causa de interbloqueo, me quedan %d, vidas", Vidas.vidas);
									close(s_planif->sock->idSocket);
									goto iniciarNivel;
									//No me quedan mas vidas
								}
							}
							//-----------------------------FIN INTERBLOQUEO


							// 	BEGIN PABLO
							if (bufPlanif->tipoMensaje == e_recurso) {
								esperoUnRecurso = 0;
								//tengoDestino = 0;
								log_debug(logger, "Entre al 1er if de e_recurso");
							}
							// END PABLO

						break;
					default:
						log_debug(logger, "llego un msj incorrecto"); // PABLO se queda colgado por aca
						log_destroy(logger);
						pthread_exit(NULL);
						//return 0; 	//PABLO los return 0 creo que hay que cambiarlo por pthread_exit(NULL);
						break;
					}


				}
				else{
					log_debug(logger, "Aviso al planif q termine normal");
					//enviarMensaje(s_planif->sock->idSocket,e_movimiento,"", logger);
					//aviso al planificardor que termine mi turno normal.
				}

			}//fin del while !llegue
		}//fin de cada recurso

		n = -1; // para que no vuelva a entrar al while
		close(s_planif->sock->idSocket);//me desconecto del planificadors
		log_debug(logger, "TERMINO NIVEL %s", miNivel );
		pthread_mutex_lock(&hilosCorriendo.mutex);
			hilosCorriendo.hilos_corriendo--;
		pthread_mutex_unlock(&hilosCorriendo.mutex);
		free(mision);
	}//fin de cada ni vel

	return 0;

}//Fin proceso personaje
