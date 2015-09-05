#include "memoria.h"

int main(int argv, char** argc) {

	int socket_swap;
	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "memoria", true, LOG_LEVEL_TRACE);



	// Levantamos el archivo de configuracion.
	LevantarConfig();

	if( conectarConSwap(&socket_swap))
			{printf("Error en conexion");}

	return EXIT_SUCCESS;

}

int conectarConSwap(int *socket_swap){

		struct addrinfo hints;
		struct addrinfo *serverInfo;
		int conexionOk = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


		if (getaddrinfo(g_Ip_Swap, g_Puerto_Swap, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
			log_info(logger,
					"ERROR: cargando datos de conexion socket_swap");
		}

		if ((*socket_swap = socket(serverInfo->ai_family, serverInfo->ai_socktype,
				serverInfo->ai_protocol)) < 0) {
			log_info(logger, "ERROR: crear socket_swap");
		}
		if (connect(*socket_swap, serverInfo->ai_addr, serverInfo->ai_addrlen)
				< 0) {
			log_info(logger, "ERROR: conectar socket_swap");
		} else {
			conexionOk = 1;
		}
		freeaddrinfo(serverInfo);	// No lo necesitamos mas
		return conexionOk;
}

#if 1 // METODOS CONFIGURACION //
void LevantarConfig() {
	t_config* config = config_create(PATH_CONFIG);
	//log_info(logger, "Archivo de configuración: %s", PATH_CONFIG);
	// Nos fijamos si el archivo de conf. pudo ser leido y si tiene los parametros
	if (config->properties->table_current_size != 0) {

		// Preguntamos y obtenemos el puerto donde esta escuchando la memoria
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			g_Puerto_Memoria = config_get_int_value(config, "PUERTO_ESCUCHA");
		} else {
			Error("No se pudo leer el parametro PUERTO_ESCUCHA");
		}

		// Preguntamos y obtenemos la ip del swap
		if (config_has_property(config, "IP_SWAP")) {
			g_Ip_Swap = config_get_string_value(config,"IP_SWAP");
		} else{
			Error("No se pudo leer el parametro IP_SWAP");
		}

		// Preguntamos y obtenemos el puerto de escucha del swap
		if (config_has_property(config, "PUERTO_SWAP")) {
			g_Puerto_Swap = config_get_string_value(config,"PUERTO_SWAP");
		} else{
			Error("No se pudo leer el parametro PUERTO_SWAP");
		}

		// Obtenemos la cantidad maxima de marcos por proceso que soporta la memoria
		if (config_has_property(config, "MAXIMO_MARCOS_POR_PROCESO")) {
			g_Max_Marcos_Proc = config_get_int_value(config, "MAXIMO_MARCOS_POR_PROCESO");
		} else{
			Error("No se pudo leer el parametro MAXIMO_MARCOS_POR_PROCESO");
		}

		// Obtenemos la cantidad de marcos que tiene la memoria
		if (config_has_property(config, "CANTIDAD_MARCOS")) {
			g_Cant_Marcos = config_get_int_value(config, "CANTIDAD_MARCOS");
		} else{
			Error("No se pudo leer el parametro CANTIDAD_MARCOS");
		}

		// Obtenemos el tamanio de los marcos de la memoria
		if (config_has_property(config, "TAMANIO_MARCO")) {
			g_Tam_Marcos = config_get_int_value(config, "TAMANIO_MARCO");
		} else{
			Error("No se pudo leer el parametro TAMANIO_MARCO");
		}

		// Obtenemos la cantidad de entradas de la TLB
		if (config_has_property(config, "ENTRADAS_TLB")) {
			g_Entradas_Tlb = config_get_int_value(config, "ENTRADAS_TLB");
		} else{
			Error("No se pudo leer el parametro ENTRADAS_TLB");
		}

		// Preguntamos y obtenemos si la TLB esta habilitada en la memoria
		if (config_has_property(config, "TLB_HABILITADA")) {
			g_Tlb_Habilitada = config_get_string_value(config,"TLB_HABILITADA");
		} else{
			Error("No se pudo leer el parametro TLB_HABILITADA");
		}

		// Obtenemos el tiempo de retardo que tiene la memoria
		if (config_has_property(config, "RETARDO_MEMORIA")) {
			g_Retardo_Memoria = config_get_int_value(config, "RETARDO_MEMORIA");
		} else{
			Error("No se pudo leer el parametro RETARDO_MEMORIA");
		}


	} else {
		Error("No se pudo abrir el archivo de configuracion");
	}
	if (config != NULL ) {
		free(config);
	}
}

#endif

#if 1 // METODOS MANEJO DE ERRORES //
void Error(const char* mensaje, ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR: %s\n", nuevo);
	log_error(logger, "%s", nuevo);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
}
#endif

void SetearErrorGlobal(const char* mensaje, ...) {
	va_list arguments;
	va_start(arguments, mensaje);
	if (g_MensajeError != NULL )
		// NMR COMENTADO POR ERROR A ULTIMO MOMENTO	free(g_MensajeError);
		g_MensajeError = string_from_vformat(mensaje, arguments);
	va_end(arguments);
}

void ErrorFatal(const char* mensaje, ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);
	printf("\nERROR FATAL--> %s \n", nuevo);
	log_error(logger, "\nERROR FATAL--> %s \n", nuevo);
	char fin;

	printf(
			"El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	fin = scanf("%c", &fin);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
	exit(EXIT_FAILURE);
}
