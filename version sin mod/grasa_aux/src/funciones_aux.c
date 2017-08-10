

#include "funciones_aux.h"

#include <sys/mman.h> //biblioteca para usar mmap y sus derivadas

#include <commons/bitarray.h> //para las funciones que manejan el BitMap
#include <commons/collections/list.h> //para devolver listas en queHayAca
#include <commons/string.h>//para usar split en how



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


int array_string_length(char** unArray){
	int i=0;
	while(unArray[i] != NULL) i++;
	return i;
}


char* sacarUnoYConcat(char** directorios){
	int i = array_string_length(directorios);
	char* path = strdup("/");
	int aux;
	for(aux = 0; aux < i-1; aux++){
		path = concat(3,path,directorios[aux],"/");
	}
	return path;
}


/*
 * obtenerArchivoPorIndice()
 * devuelve NULL si el indice es el raiz o se paso del total
 * devuelve el GFile* si esta en el rango
 * NOTA: Atencion!! el indice es el del tipo que tiene el archivo osea de 1 a 1024 entonces le hago -1!!
 */

GFile obtenerArchivoPorIndice(uint32_t indice){

	GFile ptr_arch;
	ptr_arch = ptr_archivos[indice-1];
	return ptr_arch;
}


/*
 *  obtenerPunteroDeArchivoPorIndice()
 *  dado el indice del archivo, devuelve el puntero a él para modificarlo de una
 */

GFile* obtenerPunteroDeArchivoPorIndice(int indice){
	GFile* ptr_archivo = ptr_archivos + indice - 1;
	return ptr_archivo;

}

/*
 * cumplePath() recibe el archivo a ser analizado que cumpla el path y el path del padre
 * si el path coincide, retorna 0
 * si el path no coincide, retorna -1
 */

int cumplePath(GFile archivo,const char* pathPadre){

	if(strcmp(pathPadre,"/") == 0){
		if(archivo.parent_dir_block == 0) return 0;
		else return -1;
	}

	char** nombresPadre = string_split((char*)pathPadre,"/");
	int cantNombres = array_string_length(nombresPadre);
	uint32_t indicePadre = archivo.parent_dir_block;
	GFile ptr_Padre;

	ptr_Padre = obtenerArchivoPorIndice(indicePadre);
	if(string_equals_ignore_case((char*)ptr_Padre.fname, nombresPadre[cantNombres-1]) && archivo.state != BORRADO){
		char* nuevosNombres = sacarUnoYConcat(nombresPadre);
		return cumplePath(ptr_Padre,nuevosNombres);
	}else return -1;

}


/*
 * obtenerIndiceDeNodo()
 * devuelve 0 si el path es el raiz.
 * devuelve el numero de indice del archivo si el path corresponde a un archivo valido. NOTA ES UN NUMERO DE 1 a 1024 (dsp le tengo q restar 1 para saber el indice)
 * devuelve -1 si el path no corresponde a algun archivo
 */

int obtenerIndiceDeNodo(const char* path){
	if(strcmp(path,"/") == 0) return 0;
	int nodoEncontrado = 0;
	char** nombresHijo;
	nombresHijo = string_split((char*)path, "/");
	int longitudPath = array_string_length(nombresHijo);
	char* pathPadre = sacarUnoYConcat(nombresHijo);
	int i;

	for(i=0;i<GFILEBYTABLE;i++){
		if(string_equals_ignore_case(nombresHijo[longitudPath-1],(char*)ptr_archivos[i].fname)){
			if(cumplePath(ptr_archivos[i],pathPadre) == 0){
				nodoEncontrado = 1;
				break;
			}
		}
	}
	if(nodoEncontrado == 1){
		return (i+1); //Si se encontro devuelvo el indice
	}else return -1; //Si no se encontro el Path que corresponda a un archivo
}


/*
 * pathEsDirectorio()
 * recive un path y verifica que el path dado sea un directorio
 * devuelve 0 en caso positivo, -1 en caso de error
 */
int pathEsDirectorio(char* path){

	if(strcmp(path,"/") == 0) return 0;
	int indiceArchivo = obtenerIndiceDeNodo(path); //Acá tambien se verifica que el path exista
	if(indiceArchivo < 0) return -1;
	GFile archivo = obtenerArchivoPorIndice(indiceArchivo);
	if(archivo.state != DIRECTORIO) return -1; //Si no es un directorio!
	else return 0; //Es un directorio y existe el path solicitado
}


/*
 * existeNombreEnDirectorio()
 * recibe un path(verifica que sea directorio) y un nombre a buscar en él
 * devuelve -1 en caso de error o de no haber encontrado un archivo con ese nombre
 * devuelve 0 en caso de acierto
 */

int existeNombreEnDirectorio(char* path, char* nombre){
	if(pathEsDirectorio(path) != 0) return -1;
	int resultado = -1;
	GFile archivo;
	int indice;
	for(indice = 1; indice <= GFILEBYTABLE; indice++){
		archivo = obtenerArchivoPorIndice(indice);
		if(cumplePath(archivo, path) == 0 && strcmp((char*)archivo.fname, nombre) == 0){
				resultado = 0;
				break;
		}
	}
	return resultado;
}


/*
 * indiceParaNuevoArchivo()
 * busca un lugar en la tabla de nodos (Tabla de los archivos) para crear uno nuevo
 * devuelve el indice (de 1 a 1024) en caso de que haya espacio
 * devuelve -1 en caso de que no haya mas espacio en la tabla
 */

int indiceParaNuevoArchivo(){
	int indice = -1;
	int i;
	GFile archivo;
	for(i = 1; i <= GFILEBYTABLE; i++){
		archivo = obtenerArchivoPorIndice(i);
		if(archivo.state == BORRADO){
			indice = i;
			break;
		}
	}
	return indice;
}


/*
 * agregarArchivoEnTabla()
 * recibe el nombre del archivo nuevo que se va a crear en la tabla,
 * el indice donde se va a crear el nuevo archivo,
 * y el tipo de archivo (ARCHIVO o DIRECTORIO)
 *
 * no devuelve una pija
 */
// NOTA: HAY QUE MODIFICAR ESTA FUNCION IMPLEMENTANDO: GFile* obtenerPunteroDeArchivoPorIndice(int indice);
void agregarArchivoEnTabla(char* nombre, int indice, int tipoArchivo, uint32_t padre){

	GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indice);
	ptr_archivo->state = tipoArchivo;
	ptr_archivo->file_size = 0;


	int len = strlen(nombre);
	printf("El nombre de la carpeta es: %s y el len es: %d\n",nombre,len);
	int i;
	for(i=0; nombre[i] != '\0' ; i++) {
		//printf("Voy a grabar en la pos i==%d",i);
		ptr_archivo->fname[i] = nombre[i];
		//printf("Grabe la letra == %c",subDirectorios[len-1][i]);
	}
	ptr_archivo->fname[i] = '\0';

	ptr_archivo->parent_dir_block = padre;
	printf("Grabe el bloque del padre que es el: %zu\n",ptr_archivos[indice - 1].parent_dir_block);
	ptr_archivo->c_date = 111111;//strdup(temporal_get_string_time());
	ptr_archivo->m_date = 111111;

}


/*
 * obtenerPosicionDeByte()
 * Devuelve la posicion del byte de lo mapeado en memoria que corresponde
 * a algun nodo de datos de un archivo
 */

uint8_t* obtenerPunteroDeByteLectura(GFile archivo, uint32_t byte){
	uint8_t* pos = NULL; 	//Posicion de lo mapeado en memoria (puntero del byte dentro del puntero directo)
	int offsetEnNodo; 	//Desplazamiento en el bloque de datos
	int pos_ptr_indirecto; 	//Posicion en la lista de los punteros indirectos
	int pos_ptr_directo; 	//Posicion en la lista de los punteros directos
	uint32_t ptr_directo;	//el puntero directo al bloque de datos
	uint32_t* ptr_indirecto; //posicion del array de punteros directos

	int numBloqueALeer = byte / BLOCK_SIZE;
	offsetEnNodo = byte % BLOCK_SIZE;
	pos_ptr_indirecto = numBloqueALeer / GFILEBYTABLE;
	pos_ptr_directo = numBloqueALeer % GFILEBYTABLE;

	ptr_indirecto = (uint32_t*) (ptr_mmap + archivo.blk_indirect[pos_ptr_indirecto] * BLOCK_SIZE);

	ptr_directo = ptr_indirecto[pos_ptr_directo];

	pos = (uint8_t*) ptr_mmap + (ptr_directo * BLOCK_SIZE); //pos es el puntero inicial del bloque

	return (pos + offsetEnNodo); //corro el offset para apuntar al byte solicitado
}


/*
 * buscarNodoLibre()
 * devuelve la posicion del nodo que esta libre
 * si no hay libres devuelve -1
 */

uint32_t buscarNodoLibre(void){

	uint32_t sizeBitmap = bitarray_get_max_bit(ptr_bitMap);
	uint32_t pos = 0;
	while(bitarray_test_bit(ptr_bitMap, pos)){
		if(pos >= sizeBitmap){
			pos = -1;
			break;
		}
		pos++;
	}
	return pos;
}

/*
 * nuevoPuntero()
 * busca nodo que este libre
 * si no hay libres devuelve -1
 */

uint32_t nuevoPuntero(){

	uint32_t pos_nodo = buscarNodoLibre();
	if(pos_nodo != -1){
		printf("*********Seteo a UNO el bit: %zu\n*********",pos_nodo);
		bitarray_set_bit(ptr_bitMap, pos_nodo);
	}
	return pos_nodo;

}




/*
 * obtenerPunteroDeByteEscritura()
 *
 */


uint8_t* obtenerPunteroDeByteEscritura(GFile* ptr_archivo,uint32_t byte, int necesitoNuevoNodo){

	uint8_t* pos = NULL;		//Posicion de lo mapeado en memoria (puntero del byte dentro del puntero directo)
	int offsetEnNodo; 			//Desplazamiento en el bloque de datos
	int pos_ptr_indirecto; 		//Posicion en la lista de los punteros indirectos
	int pos_ptr_directo; 		//Posicion en la lista de los punteros directos
	uint32_t ptr_directo;		//el puntero directo al bloque de datos
	uint32_t* ptr_indirecto; 	//posicion del array de punteros directos

	//cant_punteros_ind = ptr_archivo->file_size / GFILEBYTABLE;
	int numBloqueAEscribir = byte / BLOCK_SIZE;
	offsetEnNodo = byte % BLOCK_SIZE;
	pos_ptr_indirecto = numBloqueAEscribir / GFILEBYTABLE;
	pos_ptr_directo = numBloqueAEscribir % GFILEBYTABLE;


	if(necesitoNuevoNodo){
		if(pos_ptr_directo == 0){ // si la posicion del puntero directo es cero, tambien necesito un puntero indirecto nuevo
			uint32_t ptr_indirecto_Aux;
			ptr_indirecto_Aux = nuevoPuntero();
			if(ptr_indirecto_Aux == -1) return NULL; // Si no hay nodos libres
			ptr_archivo->blk_indirect[pos_ptr_indirecto] = ptr_indirecto_Aux;
			ptr_indirecto = (uint32_t*) (ptr_mmap + ptr_archivo->blk_indirect[pos_ptr_indirecto] * BLOCK_SIZE);
		}
		// Ahora obtengo el puntero indirecto:
		ptr_indirecto = (uint32_t*) (ptr_mmap + ptr_archivo->blk_indirect[pos_ptr_indirecto] * BLOCK_SIZE);
		ptr_directo = nuevoPuntero();
		if(ptr_directo == -1) return NULL; // Si no hay nodos libres
		ptr_indirecto[pos_ptr_directo] = ptr_directo;

	}

	if(!necesitoNuevoNodo){

		ptr_indirecto = (uint32_t*) (ptr_mmap + ptr_archivo->blk_indirect[pos_ptr_indirecto] * BLOCK_SIZE);

		ptr_directo = ptr_indirecto[pos_ptr_directo];
	}

	pos = (uint8_t*) ptr_mmap + (ptr_directo * BLOCK_SIZE); //pos es el puntero inicial del bloque
	printf("Voy a grabar en el nodo: %u\n", ptr_directo);
	return (pos + offsetEnNodo); //corro el offset para apuntar al byte solicitado

}



/*
 * borrarDirectorio()
 * dado el indice del directorio, lo borra de la tabla de nodos
 */

void borrarArchivo(int indiceArchivo){
	GFile* ptr_archivo = obtenerPunteroDeArchivoPorIndice(indiceArchivo);
	ptr_archivo->state = 0;

}


/*
 * eliminarNodos()
 * recive el puntero del archivo para borrar sus nodos y el inicioDeBorrado que es desde donde borro hasta el final
 */
/*
void eliminarNodos(GFile* ptr_archivo, int32_t inicioDeBorrado){

	int ultimoByte = ptr_archivo->file_size;
	uint32_t numNodoABorrar;

	while(ultimoByte / BLOCK_SIZE > inicioDeBorrado / BLOCK_SIZE){
		numNodoABorrar = ultimoByte / BLOCK_SIZE;
		eliminarNodo(ptr_archivo, numNodoABorrar);
		ultimoByte -= BLOCK_SIZE;
	}
}
*/

void eliminarNodos(GFile* ptr_archivo, int32_t inicioDeBorrado){
	int32_t ultimoByte = ptr_archivo->file_size - 1;
	uint32_t numNodoABorrar;
	puts("********Voy a entrar al while de eliminarNodos()********\n");
	while((ultimoByte - inicioDeBorrado) >= BLOCK_SIZE){ //Mientras tenga que borrar un bloque

		printf("********El valor del ulimo byte es: %zu********\n",ultimoByte);
		numNodoABorrar = ultimoByte / BLOCK_SIZE;
		printf("********Voy a eliminar el nodo numero %zu********\n",numNodoABorrar);
		eliminarNodo(ptr_archivo, numNodoABorrar);
		ultimoByte -= BLOCK_SIZE;
	}
	if(inicioDeBorrado == 0){ //Ahora si el inicio de borrado es el principio, elimino el ultimo nodo
		puts("********El inicio de borrado era el cero (se borra todo) => elimino el bloque cero********");
		eliminarNodo(ptr_archivo, 0);
	}
}

/*
 * eliminarNodo()
 * elimina el numero de nodo pasado por parametro, si estoy eliminando el nodo numero cero del array de punteros indirectos,
 * elimino tambien el puntero indirecto del archivo y le pongo cero
 */

void eliminarNodo(GFile* ptr_archivo, uint32_t numNodoABorrar){

	int pos_ptr_indirecto; 		//Posicion en la lista de los punteros indirectos
	int pos_ptr_directo; 		//Posicion en la lista de los punteros directos
	uint32_t ptr_directo;		//el puntero directo al bloque de datos
	uint32_t ptr_indirecto; 	//posicion del array de punteros directos
	uint32_t* ptr_indirecto_Array;

	//ERROR CUANDO numNodoABorrar = 1534
	//pos_ptr_directo = 510
	pos_ptr_indirecto = numNodoABorrar / GFILEBYTABLE;
	pos_ptr_directo = numNodoABorrar % GFILEBYTABLE;

	ptr_indirecto_Array = (uint32_t*) (ptr_mmap + ptr_archivo->blk_indirect[pos_ptr_indirecto] * BLOCK_SIZE);
	ptr_indirecto = ptr_archivo->blk_indirect[pos_ptr_indirecto];
	ptr_directo = ptr_indirecto_Array[pos_ptr_directo];

	bitarray_clean_bit(ptr_bitMap, ptr_directo); //Borro el puntero directo
	printf("Puse cero en el bit: %zu\n",ptr_directo);
	ptr_indirecto_Array[pos_ptr_directo] = 0;
	if(pos_ptr_directo == 0){ // Si estoy borrando el ultimo (osea el ultimo a borrar) borro tmb el puntero indirecto
		bitarray_clean_bit(ptr_bitMap, ptr_indirecto); //Borro el puntero indirecto
		printf("Puse cero en el bit: %zu\n",ptr_indirecto);
		ptr_archivo->blk_indirect[pos_ptr_indirecto] = 0;
	}

}


