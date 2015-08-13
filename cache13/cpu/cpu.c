#include "cpu.h"

int main(int argv, char** argc) {

	int socket_Plani;
	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);



	// Levantamos el archivo de configuracion.
	LevantarConfig();

	if( conectarPlanificador(&socket_Plani))
		{printf("Erro en conexion");}

		//Fin conexion

	return EXIT_SUCCESS;

}

int conectarPlanificador(int *socket_plani){
	//Conecto al planificador
	//ESTRUCTURA DE SOCKETS; EN ESTE CASO CONECTA CON NODO
		//log_info(logger, "Intentando conectar a nodo\n");
		//conectar con Nodo
		struct addrinfo hints;
		struct addrinfo *serverInfo;
		int conexionOk = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


		if (getaddrinfo(g_Ip_Planificador, g_Puerto_Planificador, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
			log_info(logger,
					"ERROR: cargando datos de conexion socket_plani");
		}

		if ((*socket_plani = socket(serverInfo->ai_family, serverInfo->ai_socktype,
				serverInfo->ai_protocol)) < 0) {
			log_info(logger, "ERROR: crear socket_plani");
		}
		if (connect(*socket_plani, serverInfo->ai_addr, serverInfo->ai_addrlen)
				< 0) {
			log_info(logger, "ERROR: conectar socket_plani");
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

		// Preguntamos y obtenemos la ip del planificador
		if (config_has_property(config, "IP_PLANIFICADOR")) {
			g_Ip_Planificador = config_get_string_value(config,"IP_PLANIFICADOR");
		} else{
			Error("No se pudo leer el parametro IP_PLANIFICADOR");
		}

		// Preguntamos y obtenemos el puerto de escucha del planificador
		if (config_has_property(config, "PUERTO_PLANIFICADOR")) {
			g_Puerto_Planificador = config_get_string_value(config,"PUERTO_PLANIFICADOR");
		} else{
			Error("No se pudo leer el parametro PUERTO_PLANIFICADOR");
		}

		// Preguntamos y obtenemos la ip de la memoria
		if (config_has_property(config, "IP_MEMORIA")) {
			g_Ip_Memoria = config_get_string_value(config,"IP_MEMORIA");
		} else{
			Error("No se pudo leer el parametro IP_MEMORIA");
		}

		// Preguntamos y obtenemos el puerto de escucha de la memoria
		if (config_has_property(config, "PUERTO_MEMORIA")) {
			g_Puerto_Memoria = config_get_string_value(config,"PUERTO_MEMORIA");
		} else{
			Error("No se pudo leer el parametro PUERTO_MEMORIA");
		}

		// Obtenemos la cantidad de hilos que va soportar la cpu
		if (config_has_property(config, "CANTIDAD_HILOS")) {
			g_Cant_Hilos = config_get_int_value(config, "CANTIDAD_HILOS");
		} else{
			Error("No se pudo leer el parametro CANTIDAD_HILOS");
		}

		// Obtenemos el retardo que va tener la cpu
		if (config_has_property(config, "RETARDO")) {
			g_Retardo = config_get_int_value(config, "RETARDO");
		} else{
			Error("No se pudo leer el parametro RETARDO");
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
