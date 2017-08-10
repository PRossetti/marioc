#include "planificador.h"

void* planificador()
{

	t_log* logPlanificador=(t_log*)malloc(sizeof(t_log));

	int q;

	//nuevo

	char** algoritmoQuantumRetardo;

	//t_socket_cliente *socket_nivel;

	//finNuevo
	// --- mseg sera el valor de milisegundos del tiempo entre quantum ---
	// --- useg sera ese valor en microsegundos para hacer uso de usleep ---
	int mseg;
	int useg = 1000000; // lo inicializo en un segundo

	t_nuevosValores *nuevosValores;
	nuevosValores = malloc(sizeof (t_nuevosValores));
	nuevosValores->hayNuevosValores = 0;

	struct timeval tiempoSelect;
	tiempoSelect.tv_sec = 0.01;
	tiempoSelect.tv_usec = 0;

	TprocesoPersonaje* personaje = (TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje));

	tipoNiveles* elem=(tipoNiveles*)malloc(sizeof(tipoNiveles));
	elem = encontrarUnNivel(niveles);
	char ** ipPuerto=string_split(elem->ipPuertoPlan, ":");
	elem->planificador=pthread_self();
	char* nombrearch=concat(3,"Planif",elem->nivel,".txt");
	logPlanificador = log_create(nombrearch,elem->nivel,false,LOG_LEVEL_DEBUG);

	// creo el servidor. La ip y el puerto se leen por configPlataforma de configuracion
	t_socket_servidor* servidorPlanificador= crearServidor(ipPuerto[0], atoi(ipPuerto[1]),logPlanificador);
	elem->seCreoSockPlan=1;


	MSJ* msj;
	int descriptor;
	TprocesoPersonaje* procesoPersonaje = (TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje));
	char ** vector;
	TprocesoPersonaje* pers_actual=(TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje));
	//TprocesoPersonaje* pjBloq=(TprocesoPersonaje*)malloc(sizeof(TprocesoPersonaje)); //agregamos esto para resu del interb
	//char* recursoBloqueante;
	//char* recursosALiberar = NULL;
	//char* personajesALiberar = NULL;
	int ordenLlegada=0;
	int i, j;
	int socket_Nivel;
	char* mensajeAEnviar;
	int nivelConectado = 0;
	int retorno;
	char* algoritmo;

	//int fd_inotify;

	//listen a nivel

	//socket_nivel = aceptarConexion(servidorPlanificador,logPlanificador);

	//msj = recibirMensaje(socket_nivel->sock->idSocket,logPlanificador);


	//en la pos 0 tiene el algoritmo y en la uno tiene el quantummmm que me pasa el nivel

	//algoritmoyquantum = string_split(msj->mensaje,",");

	//char* quantum = strdup(algoritmoyquantum[1]);




	//msj->mensaje[0];
	//quantum = msj->mensaje[1];

	//pongo en la primer pos del vector el socket del nivel
	//servidorPlanificador->sockClientes[1]=socket_nivel->sock->idSocket;
	//chau listen


	while(1)
	{

		if(nuevosValores->hayNuevosValores == 1){
			algoritmoQuantumRetardo[0] = nuevosValores->algoritmo;
			algoritmoQuantumRetardo[1] = nuevosValores->quantum;
			algoritmoQuantumRetardo[2] = nuevosValores->retardo;
			mseg = nuevosValores->mseg;
			useg = nuevosValores->useg;
			nuevosValores->hayNuevosValores = 0;

			if ( strcmp(algoritmo,algoritmoQuantumRetardo[0]) != 0 )
			{
				algoritmo = strdup(algoritmoQuantumRetardo[0]);

				if (strcmp(algoritmoQuantumRetardo[0], "SRDF") == 0)
				{
					pers_actual = NULL;
				}
			}
		}


		descriptor = multiplexarSockets(servidorPlanificador, &tiempoSelect,logPlanificador);//NO BLOQUEANTE

		//mientras haya actividad
		while(descriptor != -2)
		{
			if(descriptor>0)
			{
				msj = recibirMensaje(descriptor,logPlanificador);

				switch(msj->tipoMensaje)
				{
					case e_handshake: //HANDSHAKE con PERSONAJE
						log_debug(logPlanificador, "Llegó un handshake.");
						//armo la estructura que necesito para manejar los personajes
						vector=string_split(msj->mensaje, ":");//el vector[0] tiene el nombre
						procesoPersonaje=crearNodoPersonaje();
						procesoPersonaje->fd = descriptor;
						procesoPersonaje->simbolo = vector[1];
						procesoPersonaje->estado = LISTO;
						procesoPersonaje->recursoEnEspera = "";
						procesoPersonaje->ordenLlegada=ordenLlegada++;
						procesoPersonaje->posActual[0]=1; //pos X
						procesoPersonaje->posActual[1]=1; //pos y
						procesoPersonaje->recursos = crear_cola();
						mensajeAEnviar = concat(5,vector[0],":",vector[1],":",itoa(ordenLlegada)); // Mario:@:1
						log_debug(logPlanificador,"Mensaje que voy a enviar al nivel: %s",mensajeAEnviar);
						//  para que cree y dibuje al personaje en la pos inicial
						enviarMensaje(socket_Nivel,e_handshake,mensajeAEnviar,logPlanificador);
						//cuando tengo el personaje listo lo meto en la lista de listos

						//sleep(1); // se podria cambiar ese sleep por un okey del nivel
						//se lo mando al nivel para ver donde queda
						msj = recibirMensaje(socket_Nivel,logPlanificador); // recibo el okey
						if (msj->tipoMensaje != e_ok) {
							log_debug(logPlanificador,"Error en handshake");
						} else {
							log_debug(logPlanificador,"El handskahe termino bien");
						}

						enviarMensaje(socket_Nivel,e_coordenadas,vector[2],logPlanificador); // "H"
						msj = recibirMensaje(socket_Nivel,logPlanificador);

						/*retorno = evaluarMensaje(socket_Nivel,msj,&q,&mseg,&useg,
								&algoritmoQuantumRetardo[0],
								&algoritmoQuantumRetardo[1],
								&algoritmoQuantumRetardo[2],
								&algoritmo,
								&pers_actual,
								logPlanificador); // para ver si cambio el archivo o murio el pj
						*/
						retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);

						if (retorno == 1) msj=recibirMensaje(socket_Nivel,logPlanificador);

						vector = string_split(msj->mensaje,":");
						//En vector[0] tengo la posición X y en vector [1] tengo la posición Y

						//hago los calculos
						//pers_actual->distanciaARecurso = sqrt( elevarALa2(pers_actual->posActual[0] - atoi(vector[0]) ) + elevarALa2(pers_actual->posActual[1]  - atoi(vector[1])) );

						procesoPersonaje->distanciaARecurso = abs( atoi(vector[0]) - procesoPersonaje->posActual[0] )
								+ abs( atoi(vector[1]) - procesoPersonaje->posActual[1] );


							//ordeno la lista

						// distancia del personaje al recurso
						log_debug(logPlanificador,"%d",procesoPersonaje->distanciaARecurso);

							//NOTA: si se ordena en la pos uno le saca el control al personajeque esta ejecutando

													//el algorimo es RR y lo coloco al personaje por orden de llegada

						if (pers_actual == NULL) {
							if( (strcmp(algoritmoQuantumRetardo[0], "SRDF") == 0)
						    &&  (tamanio_cola(elem->colaListos) > 0 ) ) {
								pers_actual = procesoPersonaje;
							}
						} else if( (strcmp(algoritmoQuantumRetardo[0], "SRDF") == 0)
					    &&  (tamanio_cola(elem->colaListos) > 0 )
						&&  (procesoPersonaje->distanciaARecurso < pers_actual->distanciaARecurso) ) {
							pers_actual = procesoPersonaje;
						}

						pushear_cola(elem->colaListos, procesoPersonaje);
						loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");

						log_debug(logPlanificador,"llegue a ponerlo en la cola");

						enviarMensaje(procesoPersonaje->fd,e_ok,"Respuesta Handshake",logPlanificador);

						break;

					case e_RR: case e_SRDF: // HANDSHAKE CON NIVEL
						//tambien para cambiar el algoritmo
						//recibo mensaje de tipo "rr:3"

						if ( nivelConectado == 0 ) {
							nivelConectado = 1;
							log_debug(logPlanificador, "Llegó el handshake del Nivel");
							socket_Nivel = descriptor;
							//pos 0 algorimo
							//pos 1 quamtum
							//pos 2 deberia tener el retardo
							algoritmoQuantumRetardo = string_split(msj->mensaje,",");
							algoritmo = strdup(algoritmoQuantumRetardo[0]);
							//si es SRDF el quamtum es igual a 1
							enviarMensaje(socket_Nivel,e_ok,"soy tu planificador",logPlanificador);
							//SRDF
							mseg = atoi(algoritmoQuantumRetardo[2]);
							useg = mseg*1000;
							//log_debug(logPlanificador,"El retardo es %d",mseg);
							if (strcmp(algoritmoQuantumRetardo[0], "SRDF") == 0)
							{
								pers_actual = NULL;
							}
						} else {
							algoritmoQuantumRetardo = string_split(msj->mensaje,",");
							log_debug(logPlanificador,"CAMBIO ALGO DE LA CONFIG");
							mseg = atoi(algoritmoQuantumRetardo[2]);
							useg = mseg*1000;
							//log_debug(logPlanificador,"El retardo es %d",mseg);
							if ( strcmp(algoritmo,algoritmoQuantumRetardo[0]) != 0 )
							{
								algoritmo = strdup(algoritmoQuantumRetardo[0]);

								if (strcmp(algoritmoQuantumRetardo[0], "SRDF") == 0)
								{
									pers_actual = NULL;
								}
							}
							//enviarMensaje(socket_Nivel,e_ok,"Soy plani diciendo que registre cambio",logPlanificador);
						}

						break;


					case e_desconexion: case e_finalizoNivel:
						log_debug(logPlanificador,"ENTRE AL e_finalizoNivel 1");
					// cuidado aca porque podria ser una desconexion de nivel me parece ...
						log_debug(logPlanificador,"Entre al e_desconexion del 1er case");
						//elimino el socket de la lista del servidor

						desecharSocket(descriptor,servidorPlanificador);

						if ( descriptor != socket_Nivel) {

							for (i = 0; i < (tamanio_cola(elem->colaListos)); i++) {
								personaje=obtener_contenido_pos_determinada(elem->colaListos, i);
								if(personaje->fd==descriptor) {
									break;
								}
							}

							if (i<tamanio_cola(elem->colaListos)){
								personaje=remover_cola_pos_determinada(elem->colaListos,i);
								loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
							} else {
								for (j = 0; j < (tamanio_cola(elem->colaBloqueados)); j++) {
									personaje=obtener_contenido_pos_determinada(elem->colaBloqueados, j);
									if(personaje->fd==descriptor) {
										personaje=remover_cola_pos_determinada(elem->colaBloqueados,j);
										loggeoPersonajes(logPlanificador, elem->colaBloqueados, "Listos");
										break;
									}
								}

							}

							enviarMensaje(socket_Nivel,e_desconexion,personaje->simbolo,logPlanificador);
							//begin  nueva logica
							msj = recibirMensaje(socket_Nivel,logPlanificador);

							retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);

							if (retorno == 1) msj=recibirMensaje(socket_Nivel,logPlanificador);

							if (msj->tipoMensaje == e_ok) {
								desbloquearPersonajes(socket_Nivel,personaje,elem,e_matar_interbloq,logPlanificador);
							} else {
								log_debug(logPlanificador,"Error dentro de la logicade desconexion");
							}
							//end nueva logica
							log_debug(logPlanificador,"Primer case, aviso al nivel que se desconecto: %s",personaje->simbolo);

							//SRDF
							if( strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0)	{
								pers_actual = NULL;
							}

							log_debug(logPlanificador,"Tamamio de la cola: %d", tamanio_cola(elem->colaListos));
						} else { //se desconecto el nivel
							nivelConectado = 0;
							log_debug(logPlanificador,"SE DESCONECTO EL NIVEL");
						}

					break;



					case e_matar_interbloq:

						log_debug(logPlanificador,"Entre a e_matar_interbloq");

						for (i = 0; i < (tamanio_cola(elem->colaBloqueados)); ++i) {
							personaje=obtener_contenido_pos_determinada(elem->colaBloqueados, i);
							if(personaje->simbolo[0]==msj->mensaje[0])
							{
								break;
							}
						}

						remover_cola_pos_determinada(elem->colaBloqueados,i);
						// le aviso al PERSONAJE que murio
						enviarMensaje(personaje->fd,e_matar_interbloq,"Sos victima",logPlanificador);

						desecharSocket(personaje->fd,servidorPlanificador);

						desbloquearPersonajes(socket_Nivel,personaje,elem,e_matar_interbloq,logPlanificador);


					break;

					default:
						break;
				}
			}

			descriptor = multiplexarSockets(servidorPlanificador, &tiempoSelect,logPlanificador);//NO BLOQUEANTE
		}


		if (tamanio_cola(elem->colaListos)>0) // if para no elija un personaje que no existe
		{
			q=atoi(algoritmoQuantumRetardo[1]); //uso el quamtum que me manda el nivel

			//verifico el algoritmo que me mando el nivel
			if(strcmp(algoritmoQuantumRetardo[0],"RR") == 0)
			{
				pers_actual=remover_de_cola(elem->colaListos);//Remueve el primero

				log_debug(logPlanificador, "entro al strcmp de RR");

				elem->pjEjec=pers_actual;

				loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
			}
			else if(pers_actual == NULL)
			//primer reparticion de turnos en modo SRDF
			//cambio de RR a SRDF
			//Cuando entra un personaje nuevo
				{

					log_debug(logPlanificador,"entre al if pers_actual == NULL");

					TprocesoPersonaje *auxProcesoPersonaje;
					pers_actual= obtener_contenido_pos_determinada(elem->colaListos, 0);

					//busco pos determinada

					for (i = 0; i < (tamanio_cola(elem->colaListos)); i++)
					{
						auxProcesoPersonaje = obtener_contenido_pos_determinada(elem->colaListos, i);
						if(pers_actual->distanciaARecurso > auxProcesoPersonaje->distanciaARecurso )
						{
							pers_actual = auxProcesoPersonaje; //me quedo con el personaje que tiene la distancia menor
							elem->pjEjec=pers_actual;
						}

					}
					log_debug(logPlanificador,"le toca a %s", pers_actual->simbolo);
			}


			while (q)
			{
				//sleep segun diga el config
				usleep(useg);
				log_debug(logPlanificador,"Entre al while q y pers_actual->df: %d",pers_actual->fd);// le agregue esto para chequear algo
				enviarMensaje(pers_actual->fd, e_movimiento, "",logPlanificador); //le aviso al pj q tiene quantum
				msj=recibirMensaje(pers_actual->fd,logPlanificador);

				switch(msj->tipoMensaje)
				{
					case e_movimiento:
						//envio mensaje al nivel con la nueva posicion
						enviarMensaje(socket_Nivel,e_movimiento,msj->mensaje,logPlanificador);

						//actualizo la pos actual y la distancia al recurso del personaje que ejecuto
						pers_actual->distanciaARecurso --;

						vector = string_split(msj->mensaje,",");
						//pers_actual->posActual[0] = atoi(string_split(msj->mensaje,",")[1]);
						//pers_actual->posActual[1] = atoi(string_split(msj->mensaje,",")[2]);
						pers_actual->posActual[0] = atoi(vector[1]);
						pers_actual->posActual[1] = atoi(vector[2]);

						msj=recibirMensaje(socket_Nivel,logPlanificador);

						/*retorno = evaluarMensaje(socket_Nivel,msj,&q,&mseg,&useg,
														&algoritmoQuantumRetardo[0],
														&algoritmoQuantumRetardo[1],
														&algoritmoQuantumRetardo[2],
														&algoritmo,
														&pers_actual,
														logPlanificador); // para ver si cambio el archivo o murio el pj
												*/
						retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);
						log_debug(logPlanificador, "Sali del parche y el algorimo es %s",algoritmoQuantumRetardo[0]);
						if (retorno == 1) {
							msj=recibirMensaje(socket_Nivel,logPlanificador);
						}

						if ( msj->tipoMensaje == e_ok ) {
							log_debug(logPlanificador,"El nivel movió OK");
						} else {
							log_debug(logPlanificador,"1 ERROR al mover en NIVEL, tipo: %d",msj->tipoMensaje);
						}

						enviarMensaje(pers_actual->fd,e_ok,"",logPlanificador);


						q--;// le saco quantum

						if ( (!q) && (strcmp(algoritmoQuantumRetardo[0],"SRDF")  != 0 ) )
						{
							pushear_cola(elem->colaListos, pers_actual);
							loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");

						}

						log_debug(logPlanificador, "Se movió el personaje %s.",pers_actual->simbolo);

					break;

					case e_coordenadas:
						// pido las coordenadas al nivel
						enviarMensaje(socket_Nivel, e_coordenadas, msj->mensaje, logPlanificador); // tipo "H"

						free(msj);

						msj = recibirMensaje(socket_Nivel,logPlanificador); //me responde las coordenadas "60:15"


						/*retorno = evaluarMensaje(socket_Nivel,msj,&q,&mseg,&useg,
														&algoritmoQuantumRetardo[0],
														&algoritmoQuantumRetardo[1],
														&algoritmoQuantumRetardo[2],
														&algoritmo,
														&pers_actual,
														logPlanificador); // para ver si cambio el archivo o murio el pj
												*/
						retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);

						if (retorno == 1) {
							msj=recibirMensaje(socket_Nivel,logPlanificador);
						}
						vector = string_split(msj->mensaje,":"); //SPLIT de las nuevas coordenadas
						enviarMensaje(pers_actual->fd, e_coordenadas, msj->mensaje, logPlanificador); //se las envio al pj

						//pers_actual->distanciaARecurso = sqrt( elevarALa2(pers_actual->posActual[0] - atoi(vector[0]) ) + elevarALa2(pers_actual->posActual[1]  - atoi(vector[1])) );


						pers_actual->distanciaARecurso = abs( atoi(vector[0]) - pers_actual->posActual[0] )
														+ abs( atoi(vector[1]) - pers_actual->posActual[1] );


						log_debug(logPlanificador,"La nueva distancia al recurso es %d",pers_actual->distanciaARecurso);

						if(strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0)
						{
							pers_actual = NULL;
							q--;
						}

					break;

					/*case e_finalizoTurno:
					q=0;
					log_debug(logPlanificador, "Longitud de mensaje: %d.",msj->longitudMensaje);
					if (msj->longitudMensaje == 0){
						pushear_cola(elem->colaListos, pers_actual);
						loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
						log_debug(logPlanificador, "El personaje %s finalizó turno.",pers_actual->simbolo);
					}else
					{
						pers_actual->estado = BLOQUEADO;
						pers_actual->recursoEnEspera = msj->mensaje;
						pushear_cola(elem->colaBloqueados, pers_actual);
						loggeoPersonajes(logPlanificador, elem->colaBloqueados, "Bloqueados");
						log_debug(logPlanificador, "El personaje %s finalizó turno y quedó bloqueado.",pers_actual->simbolo);
					}
					break;
					*/

					// arreglar
					case e_desconexion:
						q=0;
						log_debug(logPlanificador,"Entre al e_desconexion del 2do case");
						descriptor = pers_actual->fd;
						if ( strcmp(algoritmoQuantumRetardo[0],"RR") == 0) {
							log_debug(logPlanificador, "entre al if RR");
							pushear_cola(elem->colaListos,pers_actual); //este pushear es una especie de parche
							// para no cambiar el codigo de abajo
							//pers_actual = NULL;
						}

						if( strcmp(algoritmoQuantumRetardo[0],"SRDF")== 0 )
						{
							pers_actual = NULL;
						}

						//elimino el socket de la lista del servidor
						desecharSocket(descriptor,servidorPlanificador);

						for (i = 0; i < (tamanio_cola(elem->colaListos)); ++i) {
							personaje=obtener_contenido_pos_determinada(elem->colaListos, i);
							if(personaje->fd==descriptor)
							{
								break;
							}
						}

						if (i<tamanio_cola(elem->colaListos)){
							personaje=remover_cola_pos_determinada(elem->colaListos,i);
							loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
						}

						enviarMensaje(socket_Nivel,e_desconexion,personaje->simbolo,logPlanificador);

						//begin  nueva logica
						msj = recibirMensaje(socket_Nivel,logPlanificador);
						if (msj->tipoMensaje == e_ok) {
							desbloquearPersonajes(socket_Nivel,personaje,elem,e_matar_interbloq,logPlanificador);
						} else {
							log_debug(logPlanificador,"Error dentro de la logicade desconexion");
						}
						//end nueva logica

						log_debug(logPlanificador,"Segundo case, aviso al nivel que se desconecto: %s",personaje->simbolo);

						log_debug(logPlanificador,"Tamaño de la cola: %d", tamanio_cola(elem->colaListos));

						break;

					// arreglar
					case e_finalizoNivel:

						log_debug(logPlanificador,"ENTRE AL e_finalizoNivel 2");
						//ver
						q=0;
						pers_actual->estado = FINALIZADO;
						pushear_cola(elem->colaFinalizados, pers_actual);
						loggeoPersonajes(logPlanificador, elem->colaFinalizados, "Finalizados");
						log_debug(logPlanificador, "El personaje %s finalizó el nivel %s.",pers_actual->simbolo,elem->nivel);
						desecharSocket(descriptor,servidorPlanificador);

						//begin  nueva logica
						msj = recibirMensaje(socket_Nivel,logPlanificador);
						if (msj->tipoMensaje == e_ok) {
							desbloquearPersonajes(socket_Nivel,personaje,elem,e_matar_interbloq,logPlanificador);
						} else {
							log_debug(logPlanificador,"Error dentro de la logicade desconexion");
						}
						//end nueva logica

						//SRDF
						if( strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0 )
						{
							pers_actual = NULL;
						}
						break;

					case e_recurso:
						q--;
						pers_actual->recursoEnEspera = msj->mensaje;
						//en primera instancia el personaje me manda el primer recurso de la lista
						mensajeAEnviar = concat(3,pers_actual->simbolo,",",msj->mensaje);

						enviarMensaje(socket_Nivel,e_recurso,mensajeAEnviar,logPlanificador);

						msj = recibirMensaje(socket_Nivel,logPlanificador);

						/*retorno = evaluarMensaje(socket_Nivel,msj,&q,&mseg,&useg,
														&algoritmoQuantumRetardo[0],
														&algoritmoQuantumRetardo[1],
														&algoritmoQuantumRetardo[2],
														&algoritmo,
														&pers_actual,
														logPlanificador); // para ver si cambio el archivo o murio el pj
												*/
						retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);

						if (retorno == 1) {
							msj=recibirMensaje(socket_Nivel,logPlanificador);
						}

						int control = msj->tipoMensaje;

						//switch( control )
						//{

						TprocesoPersonaje *auxProcesoPersonaje;


							if (control == e_error) {

								pushear_cola(elem->colaBloqueados,pers_actual);

								for (i = 0; i < (tamanio_cola(elem->colaListos)); i++)
								{
									auxProcesoPersonaje = obtener_contenido_pos_determinada(elem->colaListos, i);
									if(pers_actual->simbolo[0] == auxProcesoPersonaje->simbolo[0])
									{
										remover_cola_pos_determinada(elem->colaListos,i);
										break;
									}
								}

								log_debug(logPlanificador,"Puse en la cola de bloqueados a %s, el tamanio de la cola es %d",pers_actual->simbolo,tamanio_cola(elem->colaBloqueados));

								enviarMensaje(pers_actual->fd,e_error,"Mande Error al Personaje",logPlanificador);

								q = 0;

								if(strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0)
								{
									pers_actual = NULL;
								}

							} else if(control == e_ok) {

								pushear_cola(pers_actual->recursos,pers_actual->recursoEnEspera);

								// SRDF funciona sin este pushear
								if ( (!q) && (strcmp(algoritmoQuantumRetardo[0],"RR") == 0) ) {
									pushear_cola(elem->colaListos,pers_actual);
									//logue los listos para ver que onda
									loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
								}

								log_debug(logPlanificador, "le asigne el recurso", msj->tipoMensaje);
								pers_actual->recursoEnEspera = NULL;

								enviarMensaje(pers_actual->fd,e_ok,"Mande OK al Personaje",logPlanificador);

								if(strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0)
								{
									pers_actual = NULL;
								}

							}

						//pers_actual = NULL;
						//q--;

					break;

					default:
						break;

				} // fin acciones personaje

				// --- LOGICA DE MUERTE DE PERSONAJES POR ENEMIGOS ---
				char** victimas = NULL;
				enviarMensaje(socket_Nivel,e_muerte,"Dame las muertes",logPlanificador);
				msj = recibirMensaje(socket_Nivel,logPlanificador);

				retorno = evaluarMensaje(socket_Nivel, msj, &q, &nuevosValores,logPlanificador);
				if (retorno == 1) {
					msj=recibirMensaje(socket_Nivel,logPlanificador);
				}

				if( strcmp(msj->mensaje,"") != 0 ) {
					//pers_actual = elem->pjEjec;
					victimas = string_split(msj->mensaje,",");
					int len = tamanio_matriz(victimas);
					//int tam_cola = tamanio_cola(elem->colaListos);
					int k;
					personaje = NULL;
					log_debug(logPlanificador,"personaje==%s",personaje);

					for(i=0; i < len; i++) {
						log_debug(logPlanificador,"Murio %c",victimas[i][0]);
						//log_debug(logPlanificador,"pers_actual == %s",pers_actual->simbolo);

						if ( pers_actual != NULL ) {
							log_debug(logPlanificador,"Supongo q pers_actual no es basura, victima: %s",pers_actual->simbolo);
							if ( (pers_actual->simbolo[0] == victimas[i][0]) ) {
								q = 0;
							}
						} else {
							log_debug(logPlanificador,"pers_actual es null");
						}

						// sacar de la cola de listos
						log_debug(logPlanificador,"Tamanio cola listos: %d",tamanio_cola(elem->colaListos));
						loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
						for (k = 0; k < tamanio_cola(elem->colaListos); k++) {
							log_debug(logPlanificador, "voy a buscar al personaje en la cola listos");
							personaje=obtener_contenido_pos_determinada(elem->colaListos, k);
							log_debug(logPlanificador, "Encontre al personaje %s", personaje->simbolo);
							if( (personaje != NULL) && (personaje->simbolo[0]==victimas[i][0]) ) {
								// le aviso al personaje que murio
								log_debug(logPlanificador,"Le aviso al pj: %s que murio fd: %d",personaje->simbolo,personaje->fd);
								enviarMensaje(personaje->fd,e_muerte,"Te mato un enemigo",logPlanificador);
								desecharSocket(personaje->fd,servidorPlanificador);
								desbloquearPersonajes(socket_Nivel,personaje,elem,e_matar_interbloq,logPlanificador);
								break;
							}
						}

						log_debug(logPlanificador, "Estoy despues del for");
						if ( k < tamanio_cola(elem->colaListos) ) { // sali por el break
							log_debug(logPlanificador, "Entre al if k<tamanio_cola");
							personaje=remover_cola_pos_determinada(elem->colaListos,k);
							loggeoPersonajes(logPlanificador, elem->colaListos, "Listos");
						}

						log_debug(logPlanificador,"Estoy dsp del if k < tam_cola y condicion = %d",pers_actual != NULL);
						//log_debug(logPlanificador,"personaje: %c",pers_actual->simbolo[0]);

						if ( (pers_actual != NULL) && (pers_actual->simbolo[0] == victimas[i][0])) { //si no esta en la cola de listos, es pers_actual o elem-pjEjec
							log_debug(logPlanificador,"Le aviso al pers_actual: %s que murio fd: %d",pers_actual->simbolo,pers_actual->fd);
							enviarMensaje(pers_actual->fd,e_muerte,"Te mato un enemigo",logPlanificador);
							desecharSocket(pers_actual->fd,servidorPlanificador);
							desbloquearPersonajes(socket_Nivel,pers_actual,elem,e_matar_interbloq,logPlanificador);
						}

						// le aviso al personaje que murio
						//log_debug(logPlanificador,"Le aviso al pj: %s que murio fd: %d",personaje->simbolo,personaje->fd);
						//enviarMensaje(personaje->fd,e_muerte,"Te mato un enemigo",logPlanificador);
						//desecharSocket(personaje->fd,servidorPlanificador);

						//enviarMensaje(socket_Nivel,e_desconexion,personaje->simbolo,logPlanificador);
						//log_debug(logPlanificador,"Primer case, aviso al nivel que se desconecto: %s",personaje->simbolo);


						//SRDF

						if( strcmp(algoritmoQuantumRetardo[0],"SRDF") == 0) {
							pers_actual = NULL;
							q = 0; // chequear
						}



						log_debug(logPlanificador,"Tamaño de la cola: %d", tamanio_cola(elem->colaListos));
					}
					//enviarMensaje(socket_Nivel,e_ok,"Todoo bien",logPlanificador);
				} else {// llave del if
					log_debug(logPlanificador,"Todavia no murio nadie");
				}

				// --- FIN LOGICA DE MUERTE DE PERSONAJES POR ENEMIGOS ---

			} // fin while quantum
			elem->pjEjec=NULL;

			//SRDF

		}//fin if para no entrar si es vacia la cola

		else usleep(useg); // si no hay pj listo para planificar igual lo "duermo un quantum"
	}

	return NULL;
}


//int evaluarMensaje(int socket_Nivel,MSJ* msj,int* q,int* mseg,int* useg,char **algoritmo,char** quantum, char** retardo,
//		char** algoritmoOld, TprocesoPersonaje**pers_actual,t_log* logger){
int evaluarMensaje (int socket_Nivel, MSJ* msj, int* q, t_nuevosValores** valoresNuevos, t_log* logger){
	char** vectorAux =string_split(msj->mensaje,",");

	switch( msj->tipoMensaje )
	{

		case e_RR: case e_SRDF:

			//*algoritmoQuantumRetardo = string_split(msj->mensaje,",")[0];
			//log_debug(logger,"Entre al parche y el algoritmo es %s",*algoritmo);
			//log_debug(logger,"CAMBIO ALGO DE LA CONFIG");
			(*valoresNuevos)->mseg = atoi(vectorAux[2]);
			(*valoresNuevos)->useg = (*valoresNuevos)->mseg*1000;
			//(*q) = atoi(vectorAux[1]);
			//*algoritmoOld = vectorAux[0];
			(*valoresNuevos)->algoritmo = vectorAux[0];
			(*valoresNuevos)->quantum = vectorAux[1];
			(*valoresNuevos)->hayNuevosValores = 1;
			/*if ( strcmp(*algoritmoOld,*algoritmo) != 0 ) {
				if (strcmp(*algoritmo, "SRDF") == 0) {
					*pers_actual = NULL;
				}
			}
			*/
			//enviarMensaje(socket_Nivel,e_ok,"Soy plani diciendo que registre cambio",logger););

			return 1;
			break;

		default:
			return 0;
			break;
	}
	return 0;
}


int desbloquearPersonajes(int socket_Nivel,TprocesoPersonaje* personaje,tipoNiveles* elem,int tipo,t_log* logPlanificador) {
	log_debug(logPlanificador,"Voy a armar los mensajes de liberacion");

	int i,j;
	char* recursoBloqueante = strdup("");
	char* recursosALiberar = strdup(""); // para no enviar un null
	char* personajesALiberar = strdup("");
	TprocesoPersonaje* pjBloq;
	MSJ* msj;

	//int cantidadRecursos = tamanio_cola(personaje->recursos);
	// BEGIN PRUEBA
	char *recu;
	log_debug(logPlanificador,"personaje: %s cant recursos asignados: %d",personaje->simbolo,tamanio_cola(personaje->recursos));
	log_debug(logPlanificador,"los recursos asignados son");
	for(i=0; i < tamanio_cola(personaje->recursos); i++) {
		recu = obtener_contenido_pos_determinada(personaje->recursos,i);
		log_debug(logPlanificador,"recurso: %c",recu[0]);
	}
	// FIN PRUEBA

	// armo los mensajes de recursosLiberados y personajesLiberados
	if ( tamanio_cola(personaje->recursos) > 0 ) {
	for ( i = 0; i <= tamanio_cola(personaje->recursos); i++ ) {
		i = 0;
		recursoBloqueante = obtener_contenido_pos_determinada(personaje->recursos,i);
		remover_cola_pos_determinada(personaje->recursos,i);


		log_debug(logPlanificador,"recursoBloqueante: %s",recursoBloqueante);

		for ( j = 0; j < tamanio_cola(elem->colaBloqueados); j++ ) {

			pjBloq = obtener_contenido_pos_determinada(elem->colaBloqueados,j);

			log_debug(logPlanificador,"pjBloq->recursoEnEspera: %s",pjBloq->recursoEnEspera);
			log_debug(logPlanificador, "colaBloqueados: %s",elem->colaBloqueados);

			if ( recursoBloqueante[0] == pjBloq->recursoEnEspera[0] ) {
				if ( strcmp(personajesALiberar,"")== 0 ) {
					personajesALiberar = strdup(pjBloq->simbolo);
					break;
				} else {
					personajesALiberar = concat(3,personajesALiberar,",",pjBloq->simbolo);
					break;
				}
			}
		} // fin 2do for
		// me fijo por que sali del for actuando en consecuencia
		if ( j < tamanio_cola(elem->colaBloqueados) ) {//significa que alguien se desbloquea con ese recu
			pjBloq->estado = LISTO;
			pjBloq->recursoEnEspera = strdup("");
			pushear_cola(pjBloq->recursos,recursoBloqueante);
			// avisar al personaje que siga con el proximo recurso en su plan
			pushear_cola(elem->colaListos,pjBloq);

			TprocesoPersonaje* pjBloqAux = obtener_contenido_pos_determinada(elem->colaBloqueados,j);

			if (pjBloqAux->simbolo[0] == pjBloq->simbolo[0]) {
			remover_cola_pos_determinada(elem->colaBloqueados,j);
			} else {

			}

			enviarMensaje(pjBloq->fd,e_recurso,"I release you",logPlanificador); // revisar bien


		} else { // nadie se desbloquea con ese recu, lo puedo devolver al mapa
			if ( strcmp(recursosALiberar,"") == 0 ) {
				recursosALiberar = strdup(recursoBloqueante);
			} else {
				recursosALiberar = concat(3,recursosALiberar,",",recursoBloqueante);
			}
		}

	} // fin 1er for
	}

	log_debug(logPlanificador,"Arme los mensajes");
	log_debug(logPlanificador,"Recursos a liberar: %s",recursosALiberar);
	log_debug(logPlanificador,"Personajes a liberar: %s",personajesALiberar);


	enviarMensaje(socket_Nivel,tipo,recursosALiberar,logPlanificador);
	//enviarMensaje(socket_Nivel,e_recursosLiberados,recursosALiberar,logPlanificador);


	msj = recibirMensaje(socket_Nivel,logPlanificador);

	if (msj->tipoMensaje == e_ok) {
		log_debug(logPlanificador,"Venimos bien");
	} else {
		log_debug(logPlanificador,"Pincho");
	}

	enviarMensaje(socket_Nivel,tipo,personajesALiberar,logPlanificador);

	msj = recibirMensaje(socket_Nivel,logPlanificador);
	if (msj->tipoMensaje == e_ok) {
		log_debug(logPlanificador,"Venimos bien 2");
	} else {
		log_debug(logPlanificador,"Pincho 2");
	}


	return 0;
}
