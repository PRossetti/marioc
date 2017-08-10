#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdarg.h>
#include "sockets.h"
#include "../string.h"


MSJ* recibirMensaje(int idSocket,t_log* logger) // ante un error devuelve -1 en el tipoMensaje. Si corta por una seiaal vuelve con errno = EINTR
{

	MSJ* msj = crearMensaje();
	t_header encabezado;
	char* paquete;
	int recepcion;

	log_debug(logger,"Se recibe un mensaje del socket %d.",idSocket);

	recepcion = recv(idSocket,&encabezado,sizeof(t_header),0);
	if(recepcion <= 0){

		if(!recepcion){
	    	msj->tipoMensaje=0;
	    	errno=0;
	    	log_error(logger,"Desconeccion.");
	    }else{
	    	log_error(logger,"Error al recibir encabezado.");
	    }
	}
	else {
		paquete=calloc(encabezado.length, 1);
		recepcion = recv(idSocket,paquete,encabezado.length, 0);
		if(recepcion <= 0){

			if(!recepcion){
		    	msj->tipoMensaje=0;
		    	errno=0;
		    	log_error(logger,"Desconeccion.");
		    }else{
		    	log_error(logger,"Error al recibir mensaje.");
			}
		}
		else {
			msj->longitudMensaje=encabezado.length-1;
			msj->tipoMensaje=encabezado.tipo;
			free(msj->mensaje);
			msj->mensaje= string_duplicate(paquete);
			//msj->mensaje=string_duplicate(paquete);
			log_debug(logger,"Nuevo mensaje recibido! Tipo: %d. Mensaje: %s.",msj->tipoMensaje,msj->mensaje);
		}
		free(paquete);
	}
	return msj;

}

int enviarMensaje(int idSocket,int tipoMensaje,char* mensaje,t_log* logger)
{
	t_header encabezado;
		encabezado.tipo=tipoMensaje;
		encabezado.length=strlen(mensaje)+1;

		int status = send(idSocket, &encabezado, sizeof(t_header), MSG_NOSIGNAL);
		if (status <= 0) {
			log_error(logger,"Error al hacer el send del encabezado");
		}
		else{
			status = send(idSocket, mensaje, encabezado.length, MSG_NOSIGNAL);
			if (status <= 0) {
				log_error(logger,"Error al hacer el send del mensaje");
			}
			else{
				log_debug(logger,"Enviado.");
				log_debug(logger,"Tipo de Msj: %d. Mensaje: %s", tipoMensaje, mensaje);
			}
		}
		return status;		//devuelve la longitud del mensaje enviado, ó 0 o -1 si fallo



}

t_socket_cliente* crearCliente(char* ip,int puerto,t_log* logger)
{
	t_socket* sock = (t_socket*)malloc(sizeof(t_socket));

	if((sock->idSocket = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("Error al crear el socket");
		log_error(logger,"Error al crear el socket del cliente.");

		free(sock);
		exit(EXIT_FAILURE);
	}

	log_debug(logger,"Socket cliente creado.");

	struct sockaddr_in* direccion = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

	direccion->sin_family = AF_INET;
	direccion->sin_addr.s_addr = inet_addr(ip);
	direccion->sin_port = htons(puerto);

	sock->direccion = direccion;

	t_socket_cliente* socketCliente = (t_socket_cliente*)malloc(sizeof(t_socket_cliente));

	socketCliente->sock = sock;
	socketCliente->socket_servidor = (t_socket*)malloc(sizeof(t_socket));
	socketCliente->socket_servidor->direccion = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

	log_debug(logger,"Cliente creado satisfactoriamente! IP: %s - Puerto: %d",ip,puerto);

	return socketCliente;
}

t_socket_servidor* crearServidor(char* ip,int puerto,t_log* logger)
{
	t_socket* sock = (t_socket*)malloc(sizeof(t_socket));
	t_socket_servidor* sockServer = (t_socket_servidor*)malloc(sizeof(t_socket_servidor));
	struct sockaddr_in* direccion = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	int i,opt = 1;

	sock->idSocket = socket(AF_INET,SOCK_STREAM,0);

	if(sock->idSocket == -1)
	{
		perror("Error al crear el socket");
		log_error(logger,"Error al crear el socket del servidor.");

		free(sock);
		free(sockServer);
		free(direccion);

		exit(EXIT_FAILURE);
	}

	log_debug(logger,"Socket servidor creado.");

	if (setsockopt(sock->idSocket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) == -1)
	{
		perror("Error al configurar socket como servidor");
		log_error(logger,"Error al configurar el socket como servidor.");

		free(sock);
		free(sockServer);
		free(direccion);

		exit(EXIT_FAILURE);
	}

	direccion->sin_family = AF_INET;
	direccion->sin_addr.s_addr = inet_addr(ip);
	direccion->sin_port = htons(puerto);

	sock->direccion = direccion;

	sockServer->sock = sock;
	sockServer->max_conexiones = MAX_CLIENTES;

	for(i = 0;i < MAX_CLIENTES;i++)
		sockServer->sockClientes[i] = 0;

    if(bind(sockServer->sock->idSocket,(struct sockaddr*)sockServer->sock->direccion,sizeof(struct sockaddr_in)) < 0)
    {
        perror("Error en el bind");
        log_error(logger,"Error en el bind.");

        free(sock);
        free(sockServer);
        free(direccion);

        exit(EXIT_FAILURE);
    }

    if (listen(sockServer->sock->idSocket,sockServer->max_conexiones) < 0)
    {
        perror("Error en el listen");
        log_error(logger,"Error en la escucha de la conexion.");

        free(sock);
        free(sockServer);
        free(direccion);

        exit(EXIT_FAILURE);
    }

    log_debug(logger,"Servidor creado satisfactoriamente! IP: %s - Puerto: %d",ip,puerto);

    return sockServer;
}

void liberarServidor(t_socket_servidor* servidor,t_log* logger)
{
	close(servidor->sock->idSocket);

	free(servidor->sock->direccion);
	free(servidor->sock);
	free(servidor);

	log_debug(logger,"Servidor eliminado.");
}

void liberarCliente(t_socket_cliente* cliente,t_log* logger)
{
	close(cliente->sock->idSocket);

	free(cliente->sock->direccion);
	free(cliente->sock);
	free(cliente->socket_servidor->direccion);
	free(cliente->socket_servidor);
	free(cliente);

	log_debug(logger,"Cliente eliminado.");
}

int conectarCliente(t_socket_cliente* socketCliente,char* ipServidor,int puertoServidor,t_log* logger)
{
	socketCliente->socket_servidor->direccion->sin_family = AF_INET;
	socketCliente->socket_servidor->direccion->sin_addr.s_addr = inet_addr(ipServidor);
	socketCliente->socket_servidor->direccion->sin_port = htons(puertoServidor);

	int conexion = connect(socketCliente->sock->idSocket,(struct sockaddr*)socketCliente->socket_servidor->direccion,sizeof(struct sockaddr_in));

	if(conexion == -1)
	{
		//perror("Error en la conexion al servidor");
		log_error(logger,"Error en la conexion al servidor.");
	}else{
		log_info(logger,"Conexion exitosa al Servidor %s:%d",ipServidor,puertoServidor);
	}
	return conexion;
}

t_socket_cliente* aceptarConexion(t_socket_servidor* servidor,t_log* logger)
{
	struct sockaddr_in* direccionCliente = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	int addr_len = sizeof(direccionCliente);

	log_debug(logger,"Intentando aceptar una nueva conexion...");

	int cliente = accept(servidor->sock->idSocket,(struct sockaddr*)direccionCliente,(socklen_t*)&addr_len);

	if(cliente == -1)
	{
		perror("Error al conectar");
		log_error(logger,"Error al aceptar la conexion.");

		free(direccionCliente);
		exit(EXIT_FAILURE);
	}

	t_socket* sock = (t_socket*)malloc(sizeof(t_socket));
	sock->idSocket = cliente;
	sock->direccion = direccionCliente;

	t_socket_cliente* socketCliente = (t_socket_cliente*)malloc(sizeof(t_socket_cliente));
	socketCliente->sock = sock;
	socketCliente->estado = conectado;

	socketCliente->socket_servidor = (t_socket*)malloc(sizeof(t_socket));
	socketCliente->socket_servidor->direccion = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	memcpy(socketCliente->socket_servidor->direccion,servidor->sock->direccion,sizeof(struct sockaddr_in));

	return socketCliente;
}

int multiplexarSockets(t_socket_servidor* servidor,struct timeval* tiempoEspera,t_log* logger)
{
	fd_set descriptores;
	int sock,i;

	FD_ZERO(&descriptores);
	FD_SET(servidor->sock->idSocket,&descriptores);

	for(i = 0;i < MAX_CLIENTES;i++)
	{
		sock = servidor->sockClientes[i];
		if(sock)
			FD_SET(sock,&descriptores);
	}

	log_debug(logger,"Atendiendo clientes...");

	int actividad = select(MAX_CLIENTES + 3,&descriptores,NULL,NULL,tiempoEspera);

	if(actividad < 0)											//error
	{
		perror("Error en la multiplexacion de I/O");
		log_error(logger,"Error en la multiplexacion de clientes.");
		exit(EXIT_FAILURE);
	}

	log_debug(logger,"Verificando tipo de actividad...");

	if(actividad == 0)											//fin de tiempo de espera
		return -2;

	if(FD_ISSET(servidor->sock->idSocket,&descriptores))		//actividad en el servidor = pedido de conexion
	{
		t_socket_cliente* nuevoSocket;
		nuevoSocket = aceptarConexion(servidor,logger);

		for(i = 0;i < MAX_CLIENTES;i++)
		{
			sock = servidor->sockClientes[i];

			if(sock == 0)
			{
				servidor->sockClientes[i] = nuevoSocket->sock->idSocket;
				i = MAX_CLIENTES;
			}
		}

		return 0;
	}

	for(i = 0;i < MAX_CLIENTES;i++)								//actividad en alguno de los clientes = mensaje
	{
		sock = servidor->sockClientes[i];

		if(FD_ISSET(sock,&descriptores))
		{
			break;
		}
	}

	return sock;	//devuelve 0 si es conexion nueva
					//-2 si termino el tiempo de espera
					//NRO DE SOCKET si hay un mensaje
}

int multiplexarClientes(t_socket_servidor* servidor,t_log* logger)
{
	return multiplexarSockets(servidor,NULL,logger);
}

void desecharSocket(int idSocket,t_socket_servidor* servidor){
  int i, sock;

  for(i = 0;i < MAX_CLIENTES;i++){
	  sock = servidor->sockClientes[i];
	  if(sock == idSocket){
		  servidor->sockClientes[i] = 0;
		  i = MAX_CLIENTES;
	  }
  }
}

char* concat(int cant, ...){
	int sum = 0, cont = 0;
	char* plus;
	va_list marker;
    if (cant<1) {return 0;}
	va_start( marker, cant );
		while( cont<cant )
	   {
		  plus = va_arg( marker, char*);
	      sum += strlen(plus);
	      cont++;
	   }
	cont=0;
    char* aux ="";
	aux = malloc(sum+1); /* make space for the new string (should check the return value ...) */
	strcpy(aux, "");
	va_start( marker, cant );
	while( cont<cant )
	 {
		plus = va_arg( marker, char*);
		cont++;
		strcat(aux, plus); /* copy name into the new var */
	   }
	va_end( marker );
	return aux;
}
