#include "swap.h"

int main(int argv, char** argc) {

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);



	// Levantamos el archivo de configuracion.
	LevantarConfig();



	return EXIT_SUCCESS;

}

#if 1 // METODOS CONFIGURACION //
void LevantarConfig() {
	t_config* config = config_create(PATH_CONFIG);
	//log_info(logger, "Archivo de configuración: %s", PATH_CONFIG);
	// Nos fijamos si el archivo de conf. pudo ser leido y si tiene los parametros
	if (config->properties->table_current_size != 0) {

		// Preguntamos y obtenemos el puerto donde esta escuchando el swap
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			g_Puerto_Swap = config_get_int_value(config, "PUERTO_ESCUCHA");
		} else {
			Error("No se pudo leer el parametro PUERTO_ESCUCHA");
		}
		// Preguntamos y obtenemos el archivo de particion que va a utilizar
		if (config_has_property(config, "NOMBRE_SWAP")) {
			g_Arch_Swap = config_get_string_value(config,"NOMBRE_SWAP");
		} else{
			Error("No se pudo leer el parametro NOMBRE_SWAP");
		}
		// Obtenemos la cantidad de paginas que va a soportar
		if (config_has_property(config, "CANTIDAD_PAGINAS")) {
			g_Cant_Pags = config_get_int_value(config, "CANTIDAD_PAGINAS");
		} else{
			Error("No se pudo leer el parametro CANTIDAD_PAGINAS");
		}
		// Obtenemos el tamanio de las paginas
		if (config_has_property(config, "TAMANIO_PAGINA")) {
			g_Tam_Pags = config_get_int_value(config, "TAMANIO_PAGINA");
		} else{
			Error("No se pudo leer el parametro TAMANIO_PAGINA");
		}
		// Obtenemos el tiempo de retardo que va a llevar la compactacion
		if (config_has_property(config, "RETARDO_COMPACTACION")) {
			g_Retardo_Compact = config_get_int_value(config, "RETARDO_COMPACTACION");
		} else{
			Error("No se pudo leer el parametro RETARDO_COMPACTACION");
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
