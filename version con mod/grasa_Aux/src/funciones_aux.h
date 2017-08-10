/*
 * funciones_aux.h
 *
 *  Created on: 28/11/2013
 *      Author: utnso
 */

#ifndef FUNCIONES_AUX_H_
#define FUNCIONES_AUX_H_


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <xcb/xcb.h>   //libreria chota para usar los int "especiales"
#include <sys/mman.h>
#include <commons/bitarray.h> //para las funciones que manejan el BitMap
#include <commons/collections/list.h> //para devolver listas en queHayAca
#include <commons/string.h>//para usar split en how
#include <commons/log.h>
#include <pthread.h>


#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define GHEADERBLOCKS 1
#define BLKINDIRECT 1000
#define BLOCK_SIZE 4096
#define BORRADO 0
#define ARCHIVO 1
#define DIRECTORIO 2
#define _FILE_OFFSET_BITS 64

typedef uint32_t ptrGBloque;

typedef struct grasa_header_t { // un bloque
	unsigned char grasa[5];
	uint32_t version;
	uint32_t blk_bitmap;
	uint32_t size_bitmap; // en bloques
	unsigned char padding[4073];
} GHeader;

typedef struct grasa_file_t { // un cuarto de bloque (256 bytes)
	uint8_t state; // 0: borrado, 1: archivo, 2: directorio
	unsigned char fname[GFILENAMELENGTH];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t c_date;
	uint64_t m_date;
	ptrGBloque blk_indirect[BLKINDIRECT];
} GFile;


////////////////////////////      VARIABLES GLOBALES     //////////////////////
	uint8_t* ptr_mmap; //el puntero a lo mapeado en memoria
	GHeader* ptr_header; //header del archivo
	GFile* ptr_archivos;
	t_bitarray* ptr_bitMap; //el array de bits del archivo
	t_log* logger;
	pthread_rwlock_t semaforo;
////////////////////////////      FIN  VARIABLES GLOBALES     //////////////////////



char* concat(int cant, ...);
int array_string_length(char** unArray);
char* sacarUnoYConcat(char** directorios);
GFile obtenerArchivoPorIndice(uint32_t indice);
int cumplePath(GFile archivo,const char* pathPadre);
int obtenerIndiceDeNodo(const char* path);
int pathEsDirectorio(char* path);
int existeNombreEnDirectorio(char* path, char* nombre);
int indiceParaNuevoArchivo();
void agregarArchivoEnTabla(char* nombre, int indice, int tipoArchivo, uint32_t padre);
uint8_t* obtenerPunteroDeByteLectura(GFile archivo, uint32_t byte);
uint8_t* obtenerPunteroDeByteEscritura(GFile* ptr_archivo,uint32_t byte,int necesitoNuevoNodo);
void borrarArchivo(int indiceDirectorio);
GFile* obtenerPunteroDeArchivoPorIndice(int indiceDirectorio);
uint32_t nuevoPuntero(void);
uint32_t buscarNodoLibre(void);
void eliminarNodos(GFile* ptr_archivo, int32_t offset);
void eliminarNodo(GFile* ptr_archivo, uint32_t numNodoABorrar);


#endif /* FUNCIONES_AUX_H_ */
