#ifndef PRUEBA_H_
#define PRUEBA_H_

#include "mensajeria.h"
#include "../log.h"
#include <sys/types.h>

#define MAX_CLIENTES 3000

typedef enum
{
	desconectado = 0,
	conectado = 1,
}t_estado;

typedef struct
{
	int idSocket;
	struct sockaddr_in* direccion;
}t_socket;

typedef struct
{
	t_socket* sock;
	t_socket* socket_servidor;
	t_estado estado;

}t_socket_cliente;

typedef struct
{
	t_socket* sock;
	int max_conexiones;
	int sockClientes[MAX_CLIENTES];

}t_socket_servidor;

MSJ* recibirMsjConOSinSenial(int, int,t_log*);
MSJ* recibirMensaje(int,t_log*);
int enviarMsjConOSinSenial(int,int,char*, int,t_log*);
int enviarMensaje(int,int,char*,t_log*);
t_socket_cliente* crearCliente(char*,int,t_log*);
int conectarCliente(t_socket_cliente*,char*,int,t_log*);
t_socket_servidor* crearServidor(char*,int,t_log*);
void liberarServidor(t_socket_servidor*,t_log*);
void liberarCliente(t_socket_cliente*,t_log*);
t_socket_cliente* aceptarConexion(t_socket_servidor*,t_log*);
int multiplexarSockets(t_socket_servidor*,struct timeval*,t_log*);
int multiplexarClientes(t_socket_servidor*,t_log*);
char* concat(int, ...);
void desecharSocket(int,t_socket_servidor*);

#endif /* PRUEBA_H_ */
