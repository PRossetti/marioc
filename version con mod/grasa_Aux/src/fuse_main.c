#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "funciones_aux.h"
#include <commons/collections/list.h>
#include <inttypes.h>


	/*
	 * Esta es una estructura auxiliar utilizada para almacenar parametros
	 * que nosotros le pasemos por linea de comando a la funcion principal
	 * de FUSE
	 */
	struct t_runtime_options {
		char* welcome_msg; //estaba en el codigo original
	} runtime_options;


/*
	////////////////////////////      VARIABLES GLOBALES     //////////////////////
		uint8_t* ptr_mmap; //el puntero a lo mapeado en memoria
		GHeader* ptr_header; //header del archivo
		GFile* ptr_archivos;
		t_bitarray* ptr_bitMap; //el array de bits del archivo
		t_log* logger;
	////////////////////////////      FIN  VARIABLES GLOBALES     //////////////////////
*/

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


int fuse_getattr(const char *path, struct stat *stbuf){

	puts("Entre a fuse_getattr");
	log_debug(logger, "Voy a hacer un getattr");
	//int res = 0;
	memset(stbuf, 0, sizeof(struct stat));

	uint32_t indiceArchivo;
	GFile archivo;

	//puts("******lockeo semaforo******");
	//pthread_mutex_lock(&semaforo);
	pthread_rwlock_wrlock(&semaforo);
	indiceArchivo= obtenerIndiceDeNodo(path);
	pthread_rwlock_unlock(&semaforo);

	if(strcmp(path,"/") == 0){ //Si es el directorio raiz
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	if(indiceArchivo != -1){ //Si el path coincide con un Nodo
		pthread_rwlock_wrlock(&semaforo);
		archivo = obtenerArchivoPorIndice(indiceArchivo);
		pthread_rwlock_unlock(&semaforo);
		if(archivo.state == DIRECTORIO){ //Si el archivo es un directorio
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 1;
			//ver si pongo mas datos
		} else { //if(archivo.state == ARCHIVO){
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = archivo.file_size;
		}
		//puts("******libero semaforo******");
		//pthread_mutex_unlock(&semaforo);
//		pthread_rwlock_unlock(&semaforo);
		return 0;
	} else{
		//puts("******libero semaforo******");
		//pthread_mutex_unlock(&semaforo);
//		pthread_rwlock_unlock(&semaforo);
		return -ENOENT; //Si el directorio no existe
	}
	//return res;
}



int fuse_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	(void) fi;
	(void) offset;
	//puts("******lockeo semaforo******");
	//pthread_mutex_lock(&semaforo);
	int indiceNodo = obtenerIndiceDeNodo(path); //Para verificar si existe Nodo con ese path
	GFile archivo;
	int i;
	puts("Voy a hacer un readdir");

//	filler(buffer, ".", NULL, 0);
//	filler(buffer, "..", NULL, 0);

	if(indiceNodo != -1){ //Si existe el nodo
		filler(buffer, ".", NULL, 0);
		filler(buffer, "..", NULL, 0);
		for(i = 0; i < GFILEBYTABLE;i++){
				archivo = obtenerArchivoPorIndice(i);
				if(archivo.parent_dir_block == indiceNodo && archivo.state != BORRADO)
					filler(buffer,(char*)archivo.fname, NULL, 0);
			}
//		puts("******libero semaforo******");
//		pthread_mutex_unlock(&semaforo);
		return 0;

		} else{
			//puts("******libero semaforo******");
			//pthread_mutex_unlock(&semaforo);
			return -ENOENT; //No Existe directorio
		}
}



int fuse_open(const char *path, struct fuse_file_info *fi){

	pthread_rwlock_wrlock(&semaforo);
	int indiceArchivo;
	//pthread_mutex_lock(&semaforo);
	indiceArchivo= obtenerIndiceDeNodo(path);
	//pthread_mutex_unlock(&semaforo);
	(void) fi;

	//Si no se encuenta el archivo
	puts("Voy a hacer un open");
	if (indiceArchivo == -1){
		pthread_rwlock_unlock(&semaforo);
		return -ENOENT;
	}

	//Si no es accesible el archivo
	//if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;
	pthread_rwlock_unlock(&semaforo);
	return 0;
}



/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde se va a guardar el contenido solicitado
 * 		size - Nos indica cuanto tenemos que leer
 * 		offset - A partir de que posicion del archivo tenemos que leer
 *
 * 	@RETURN
 * 		Si se usa el parametro direct_io los valores de retorno son 0 si  elarchivo fue encontrado
 * 		o -ENOENT si ocurrio un error. Si el parametro direct_io no esta presente se retorna
 * 		la cantidad de bytes leidos o -ENOENT si ocurrio un error. ( Este comportamiento es igual
 * 		para la funcion write )
 */

int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

	pthread_rwlock_wrlock(&semaforo);
	size_t len;
	(void) fi;

	int indiceArchivo;
	//pthread_mutex_lock(&semaforo);
	indiceArchivo= obtenerIndiceDeNodo(path);

	//Si no se encuenta el archivo
	if (indiceArchivo < 0)
	{
		pthread_rwlock_unlock(&semaforo);
		return -ENOENT;
	}

	//Si el archivo existe:
	GFile archivo;
	archivo = obtenerArchivoPorIndice(indiceArchivo);
	len = archivo.file_size; //len = ptr_nodo[numNodo].file_size; //extern GFile* ptr_nodo;

//	if((size + offset) > archivo.file_size){
//		pthread_rwlock_unlock(&semaforo);
//		return -ENOENT;
//	}

	if (offset < len) {
		if (offset + size > len) size = len - offset;

		int byteInicio;
		int bytesLeidos = 0;
		int bytesALeer; //los que faltan leer
		int bytesRestantesEnElBloque;
		uint8_t* posDeLectura;


		while(bytesLeidos < size){
			if(bytesLeidos == 0) //Si es la primera vez
				byteInicio = offset % BLOCK_SIZE;
			else byteInicio = 0;

			bytesRestantesEnElBloque = BLOCK_SIZE - byteInicio;
			bytesALeer = size - bytesLeidos;

			if ( bytesALeer > bytesRestantesEnElBloque ) {
				bytesALeer = bytesRestantesEnElBloque;
			}

			posDeLectura = obtenerPunteroDeByteLectura(archivo, (bytesLeidos+offset) );

			memcpy(buf + bytesLeidos, posDeLectura, bytesALeer);
			bytesLeidos += bytesALeer;
		}

	} else	size = 0; //Si el archivo esta vacio
	//pthread_mutex_unlock(&semaforo);
	pthread_rwlock_unlock(&semaforo);
	return size;
}



/* fuse_mkdir()
 * Valores de retorno:
 * 		0 				En caso de acierto
 * 		-EEXIST 		En caso de que ya exista el path solicitado (no necesariamente un directorio)
 * 		-ENAMETOOLONG 	En caso de que el nombre del archivo sea muy largo
 *		-ENOSPC			En caso de que no haya lugar para el nuevo directorio (supera los 1024)
 *		-ENOSPC			EN caso de que el disco no tenga mas lugar en memoria
 *		-ENOENT			En caso de que el path no exita
 */

static int32_t fuse_mkdir(const char *path, mode_t modo){

	puts("Entre a fuse_mkdir\n");
	int ret = 0;
	//t_log* logger_mkdir = log_create("log_mkdir","main.c",0,LOG_LEVEL_DEBUG);
	//log_debug(logger_mkdir, "Voy a hacer un mkdir");
	//modo = S_IFDIR;
	if(strcmp(path,"/") == 0) return -EEXIST;

	char** nombresPath = string_split((char*)path, "/") ;
	int cantidadNombres = array_string_length(nombresPath);
	char* pathPadres = sacarUnoYConcat(nombresPath);
	char* nombreDirectorio = nombresPath[cantidadNombres-1];
	printf("Tome el path a crear y quiero crear :%s\n",path);
	//Verificar que exista el path del padre
	//log_debug(logger, "Verifico que exista el padre");
	puts("Verifico que exista el padre\n");
	if(pathEsDirectorio(pathPadres) < 0) return -ENOENT;

	//Verificar que no se repita el nombre del archivo
	puts("Veo que no exista el Directorio de antemano\n");
	if(existeNombreEnDirectorio(pathPadres, nombreDirectorio) == 0) return -EEXIST;

	//Verificar que haya lugar para crear el directorio
	puts("Obtengo el nuevo indice de la tabla\n");
	int nuevoIndice = indiceParaNuevoArchivo();
	if(nuevoIndice < 1) return -ENOSPC;
	printf("El Indice a grabar es el %d\n", nuevoIndice);

	//Verificar que el nombre no sea muy largo
	puts("Verifico que el nombre no sea muy largo\n");
	if(strlen(nombreDirectorio) > GFILENAMELENGTH) return -ENAMETOOLONG;

	//Se puede crear el directorio
	//Crear Directorio Nuevo
	puts("Creo el directorio nuevo\n");
	//pthread_mutex_lock(&semaforo);
	uint32_t indicePadre = obtenerIndiceDeNodo(pathPadres);
	agregarArchivoEnTabla(nombreDirectorio, nuevoIndice, DIRECTORIO, indicePadre);
	//pthread_mutex_unlock(&semaforo);
	return ret;
}




/*
 * fuse_rmdir()
 * 	Valores de retorno:
 *
 * 	0				En casp de acierto
 * 	-ENOENT			En caso de que el path no exista
 * 	-ENOTDIR		En caso de que el path no sea un directorio
 * 	-EBUSY			En casp de que si esta siendo usado por por otros archivos (tiene contenido adentro)
 *
 *
 */

int fuse_rmdir(const char *path){

	puts("Entre al rmdir\n");
	int indiceDirectorio;
	indiceDirectorio = obtenerIndiceDeNodo(path);
	int directorioLibre = 1;

	puts("Verifico que el path exista\n");
	if(indiceDirectorio < 0) return -ENOENT;

	GFile directorio;
	//pthread_mutex_lock(&semaforo);
	directorio = obtenerArchivoPorIndice(indiceDirectorio);

	printf("Verifico que el archivo \"%s\" sea un directorio \n",directorio.fname);
	if(directorio.state != DIRECTORIO) return -ENOTDIR;

	int i;
	GFile archivoEnDir;
	for(i = 1; i <= GFILEBYTABLE; i++){
		archivoEnDir = obtenerArchivoPorIndice(i);
		//if(indiceDirectorio == i) i++;
		if(cumplePath(archivoEnDir,path) == 0){
			directorioLibre = 0;
			break;
		}
	}

	if(directorioLibre){
		puts("El directorio se encuentra vacio y voy a eliminarlo");
		//borrarArchivo(indiceDirectorio);
		//Ahora marco como borrado el archivo:
		GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indiceDirectorio);
		ptr_archivo->state = BORRADO;
		ptr_archivo->fname[0] = '\0'; //borro nombre
		ptr_archivo->parent_dir_block = 1025; //le digo q ya no tiene padre (lo saco del rango de nodos)
		//pthread_mutex_unlock(&semaforo);
		return 0;
	}
	//pthread_mutex_unlock(&semaforo);
	return -EBUSY;
}




/*
int fuse_truncate(const char * path, off_t offset){
	puts("Entro al truncate");
	return 0;
}
*/


/*
 * fuse_create()
 * Valores de retorno:
 * 		0 				En caso de acierto
 * 		-EEXIST 		En caso de que ya exista el path solicitado (no necesariamente un directorio)
 * 		-ENAMETOOLONG 	En caso de que el nombre del archivo sea muy largo
 *		-ENOSPC			En caso de que no haya lugar para el nuevo directorio (supera los 1024)
 *		-ENOSPC			EN caso de que el disco no tenga mas lugar en memoria
 *		-ENOENT			En caso de que el path no exita
 */

static int32_t fuse_create(const char *path, mode_t modo, struct fuse_file_info *fi){

	pthread_rwlock_wrlock(&semaforo);
	puts("Entre en fuse_create");
	//modo = S_IRWXU;
	(void) fi;

	char** nombresPath = string_split((char*)path, "/");
	int cantNombres = array_string_length(nombresPath);
	char* pathPadres = sacarUnoYConcat(nombresPath);
	int indiceDir = obtenerIndiceDeNodo(pathPadres);
	char* nombreArchivo = strdup(nombresPath[cantNombres-1]);

	if(indiceDir == -1) return -ENOENT;
	//pthread_mutex_lock(&semaforo);
	GFile directorio = obtenerArchivoPorIndice(indiceDir);

	if(strcmp(pathPadres,"/") != 0){
		if(directorio.state != 2){
			pthread_rwlock_unlock(&semaforo);
			return -ENOTDIR;
		}
	}


	if(existeNombreEnDirectorio(pathPadres, nombreArchivo) == 0){
		pthread_rwlock_unlock(&semaforo);
		return -EEXIST;
	}

	if(strlen(nombreArchivo) > GFILENAMELENGTH){
		pthread_rwlock_unlock(&semaforo);
		return -ENAMETOOLONG;
	}

	int nuevoIndice = indiceParaNuevoArchivo();
	if(nuevoIndice < 1){
		pthread_rwlock_unlock(&semaforo);
		return -ENOSPC;
	}

	// Todo joya amiguiii, mandale gas

	agregarArchivoEnTabla(nombreArchivo, nuevoIndice, ARCHIVO, indiceDir);
	pthread_rwlock_unlock(&semaforo);
	return 0;
}



int fuse_truncate(const char * path, off_t offset) {

	pthread_rwlock_wrlock(&semaforo);
	puts("*******Entre a fuse_truncate*******");
	int indiceArchivo = obtenerIndiceDeNodo(path);
	//Verifico que el path exista
	if(indiceArchivo == -1) return -ENOENT;
	GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indiceArchivo);
	//Verifico que el archivo exista
	if(ptr_archivo->state != ARCHIVO) return -ENOENT;
	uint32_t eloffset = offset;
	printf("*******El offset es de %zu bytes*******\n", eloffset);
	printf("*******El tamaño del archivo es de %zu bytes*******\n", ptr_archivo->file_size);
	// no libera nodos cuando es un archivo que ya estaba escrito
	//(entonces cuando entra al write me vuelve a tomar nuevos bloques sin
	//liberar los anteriores) ---> Nota: probar if(ptr_archivo->file_size) -> entra siempre q no sea cero :)
//	pthread_rwlock_wrlock(&semaforo);
	if(ptr_archivo->file_size){ //Siempre que el archivo tenga un tamaño
		eliminarNodos(ptr_archivo, offset);
		ptr_archivo->file_size = offset;
	}
	pthread_rwlock_unlock(&semaforo);


	return 0;
}




//Write should return exactly the number of bytes requested except on error

/*
 * fuse_write()
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde tengo el contenido para guardar
 * 		size - Nos indica cuanto tenemos que grabar
 * 		offset - A partir de que posicion del archivo tenemos que grabar
 *
 * 	@RETURN
 * 		size			En caso de acierto
 *		-ENOSPC			EN caso de que el disco no tenga mas lugar en memoria
 *		-ENOENT			En caso de que el path no exita
 */

// NOTAAA!!! VER QUE PIJA HACER CON LOS ARCHIVOS QUE NO SE PUEDEN LLEGAR A GRABAR POR FALTA DE ESPACIO

static int32_t fuse_write(const char *path, const char * buf, size_t size, off_t offset,struct fuse_file_info *fi){

	pthread_rwlock_wrlock(&semaforo);
	puts("*******Entre a fuse_write*******\n");

	(void) fi;


	int indiceArchivo = obtenerIndiceDeNodo(path);
	puts("Verifico que el path exista\n");
	if(indiceArchivo == -1) return -ENOENT;
	GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indiceArchivo);
	puts("Verifico que el archivo exista\n");
	if(ptr_archivo->state != ARCHIVO) return -ENOENT; //NO se que es lo q hay que devolver cuando no es un archivo

	int32_t bytesGrabados = 0;
	int32_t byteInicio;
	uint8_t* ptr_nodoEscritura;
	int bytesRestantesEnElBloque;
	int32_t bytesAEscribir;
	//int quedaEspacio = 1;
//	pthread_rwlock_wrlock(&semaforo);
	int necesitoNuevoNodo = 1;


	while(bytesGrabados < size){
		if(bytesGrabados == 0){
			byteInicio = offset % BLOCK_SIZE;
		} else byteInicio = 0;

		bytesRestantesEnElBloque = BLOCK_SIZE - byteInicio;
		bytesAEscribir = size - bytesGrabados;

		if ( bytesAEscribir > bytesRestantesEnElBloque ) {
			bytesAEscribir = bytesRestantesEnElBloque;
		}

		if(ptr_archivo->file_size == 0){ //si el tamaño era cero, siempre voy a necesitar un nodo nuevo
			necesitoNuevoNodo = 1;
		} else{
			if(((offset + 1)/BLOCK_SIZE) > ((ptr_archivo->file_size -1)/BLOCK_SIZE)) necesitoNuevoNodo = 1;
			else necesitoNuevoNodo = 0;
		}
		printf("********El tamaño del archivo es de: %zu********\n",ptr_archivo->file_size);
		int32_t valorOffset = offset;
		printf("********El tamaño del offset es de: %d********\n",valorOffset);
		printf("********El tamaño del size es de: %d********\n",size);
		printf("********Necesito nuevo nodo? (1 = si, 0 = no): %d********\n",necesitoNuevoNodo);

		ptr_nodoEscritura = obtenerPunteroDeByteEscritura(ptr_archivo, (bytesGrabados+offset), necesitoNuevoNodo);
		if(ptr_nodoEscritura == NULL){
			pthread_rwlock_unlock(&semaforo);
			return -ENOSPC;
		}
		//puts("********Voy a grabar en el nodo********");
		//printf("Voy a grabar en el nodo: %u\n", *ptr_nodoEscritura);
		memcpy(ptr_nodoEscritura,buf + bytesGrabados,bytesAEscribir);
		bytesGrabados += bytesAEscribir;
		//printf("********Grabe en el archivo \"%s\" y el total de lo grabado es: %zu bytes********\n",ptr_archivo->fname, bytesGrabados);
	}


	if(size + offset > ptr_archivo->file_size){
		ptr_archivo->file_size = size + offset;
		//printf("********Actualizo tamaño de archivo: %zu bytes********\n", ptr_archivo->file_size);
	}
	printf("****** GRABE EN EL ARCHIVO \"%s\" %zu BYTES ******\n",ptr_archivo->fname,ptr_archivo->file_size);
	pthread_rwlock_unlock(&semaforo);
	return size;//Si grabo todo lo que queria grabar

}





static int32_t fuse_unlink(const char * path){

	pthread_rwlock_wrlock(&semaforo);
	puts("*******Entre a fuse_unlink*******");
//	pthread_rwlock_wrlock(&semaforo);
	int indiceArchivo = obtenerIndiceDeNodo(path);
	//Verifico que el path exista
	if(indiceArchivo == -1){
		pthread_rwlock_unlock(&semaforo);
		return -ENOENT;
	}
	GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indiceArchivo);
	//Verifico que el archivo exista
	if(ptr_archivo->state != ARCHIVO){
		pthread_rwlock_unlock(&semaforo);
		return -ENOENT;
	}

	if(ptr_archivo->file_size){ //Siempre que el archivo tenga un tamaño
		eliminarNodos(ptr_archivo, 0);//Le borro todo su contenido
	}

	//Ahora marco como borrado el archivo:
	ptr_archivo->state = BORRADO;
	ptr_archivo->fname[0] = '\0'; //borro nombre
	ptr_archivo->parent_dir_block = 1025; //le digo q ya no tiene padre (lo saco del rango de nodos)
	pthread_rwlock_unlock(&semaforo);
	return 0;
}








/*
 * Esta es la estructura principal de FUSE con la cual nosotros le decimos a
 * biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
 * Como se observa la estructura contiene punteros a funciones.
 */

/*
IMPLEMENTADA ---> Leer archivos
IMPLEMENTADA ---> Crear archivos
Escribir archivos
Borrar archivos
IMPLEMENTADA ---> Crear directorios y dos niveles de subdirectorios.
IMPLEMENTADA ---> Borrar directorios vacíos
*/
static struct fuse_operations fuse_oper = {
		.getattr = fuse_getattr,
		.readdir = fuse_readdir,
		.open = fuse_open,
		.read = fuse_read,
		.mkdir = fuse_mkdir,
		.rmdir = fuse_rmdir,
		.create = fuse_create,
		.truncate = fuse_truncate,
		.write = fuse_write,
		.unlink = fuse_unlink
};

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};


/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		//CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0), //estaba en el codigo original

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};




int main(int argc, char *argv[]){


	pthread_rwlock_init(&semaforo,NULL); //INICIALIZO SEMAFORO

	logger = log_create("log_fuse","main.c",0,LOG_LEVEL_DEBUG);

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct stat arch_stat; //el stat para el archivo

	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	int fd = -1;

	fd = open(argv[1], O_RDWR);
    if (fd == -1) perror("open");//abro el archivo con la estructura GRASA

	fstat(fd, &arch_stat); //llena la estructura stat del archivo abierto

	ptr_mmap = (uint8_t*) mmap(NULL, arch_stat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); //data_map es un puntero a lo mapeado en memoria

	// Recupero datos del archivo mapeado:
	ptr_header = (GHeader*)(ptr_mmap + 0); //siempre es 1 bloque

	ptr_archivos = (GFile*) (ptr_mmap + BLOCK_SIZE * GHEADERBLOCKS + BLOCK_SIZE * ptr_header->size_bitmap);

	int cantBytes = ((arch_stat.st_size/BLOCK_SIZE)/8); //cant d bytes q ocupa el bitarray
	ptr_bitMap = bitarray_create((char *) (ptr_mmap+BLOCK_SIZE*GHEADERBLOCKS),cantBytes); //hago el bitmap

	printf("--------- PRUEBA PARA BITMAP ------------\n");
	uint32_t sizeBitmap = bitarray_get_max_bit(ptr_bitMap);
	printf("el tamaño del bitmap es: %zu\n",sizeBitmap);

	printf("--------- FIN PRUEBA PARA BITMAP ------------\n");
	//Delego a Fuse:



	// Limpio la estructura que va a contener los parametros


	// Esta funcion de FUSE lee los parametros recibidos y los intepreta




	//Esto no se si lo tengo que hacer, ya que estaria eliminando las variables globales!!
	//bitarray_destroy(bitMap); //destruyo el bitmap
	//munmap(data_map, arch_stat.st_size); //libero lo mapeado en memoria
	//close(fd);//cierro el descriptor de archivo

	return fuse_main(args.argc -1, args.argv +1, &fuse_oper, NULL);
}
