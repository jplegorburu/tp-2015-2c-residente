#include "planificador.h"

int main(int argv, char** argc) {

	int iThreadConsola;					//Hilo de consola

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);

	// Instanciamos el archivo donde se grabará lo solicitado por consola
	g_ArchivoConsola = fopen(NOMBRE_ARCHIVO_CONSOLA, "wb");
	g_MensajeError   = malloc(1 * sizeof(char));

	// Levantamos el archivo de configuracion.
	LevantarConfig();

	//Este hilo es el que maneja la consola
	if ((iThreadConsola = pthread_create(&hConsola, NULL, (void*) Comenzar_Consola, NULL)) != 0){
		fprintf(stderr, (char *)NosePuedeCrearHilo, iThreadConsola);
		exit(EXIT_FAILURE);
	};


	pthread_join(hConsola, NULL );



	return EXIT_SUCCESS;

}

void Comenzar_Consola() {

	int corte_consola = -1;

	while (corte_consola != 0) {
		corte_consola = operaciones_consola();//menu de consola para elegir la opcion a realizar en filesystem
	}
	printf("Se termino la ejecucion de la consola del planificador\n");
	//free(puntero_inicial);
}

int operaciones_consola() {
	//system("clear");
	//printf("------------Tamaño del Fs:%luMb Tamaño Ocupado:%luMb Tamaño Disponible:%luMb-------------\n",tamanioTotal(),tamanioOcupado(),tamanioDisponible());
	printf("Comandos posibles: correr, finalizar, ps y cpu. 0 para salir.\n");


	char* ingresoConsola = malloc(MAXLINEA);
	fgets (ingresoConsola, MAXLINEA, stdin);
	//system("clear");
	int cont=0;
		//Le saco el \n final a la linea ingresada
		char **sinBarraN = string_split(ingresoConsola,"\n");
		//Separo el comando y su campo ingresado en caso que tenga.
		char **comando = string_split(sinBarraN[0]," ");

		while(comando[cont]!=NULL){
			  cont++;
		}
		if(cont == 3){
			printf("Comando invalido.\n");
			return 1;
		}
		printf("Comando ingresado: %s \n",comando[0]);
		printf("PATH ingresado: %s \n",comando[1]);
	if (strcmp(comando[0], "correr") == 0)
	{
		char* PATH = comando[1];
		printf("Ejecutando comando correr mCod:%s\n",PATH);


		// do something
	}
	else if (strcmp(comando[0], "finalizar") == 0)
	{
		char* PID =comando[1];
		printf("Ejecutando comando finalizar para el pid %s\n", PID);
		// do something else
	}
	else if (strcmp(comando[0], "ps") == 0)
	{
		printf("Ejecutando comando ps\n");
		  // do something else
	}
	else if (strcmp(comando[0], "cpu") == 0)
	{
		printf("Ejecutando comando cpu\n");
		  // do something else
	}
	else if (strcmp(comando[0], "0") == 0)
		{
		log_info(logger, "Terminando el programa");
		return 0;
		}
	else /* default: */
	{
		printf("Comando invalido.\n");
 	return 1;
	}



	return -1;
}


#if 1 // METODOS CONFIGURACION //
void LevantarConfig() {
	t_config* config = config_create(PATH_CONFIG);
	//log_info(logger, "Archivo de configuración: %s", PATH_CONFIG);
	// Nos fijamos si el archivo de conf. pudo ser leido y si tiene los parametros
	if (config->properties->table_current_size != 0) {

		// Preguntamos y obtenemos el puerto donde esta escuchando el planificador
		if (config_has_property(config, "PUERTO_ESCUCHA")) {
			g_Puerto_Planificador = config_get_int_value(config, "PUERTO_ESCUCHA");
		} else {
			Error("No se pudo leer el parametro PUERTO_ESCUCHA");
		}
		// Preguntamos y obtenemos el algoritmo de planificacion que va a utilizar
		if (config_has_property(config, "ALGORTIMO_PLANIFICACION")) {
			g_Algoritmo_Planificador = config_get_string_value(config,"ALGORTIMO_PLANIFICACION");
		} else{
			Error("No se pudo leer el parametro ALGORTIMO_PLANIFICACION");
		}
		// Obtenemos el quantum que se va a utilizar si el algorimo lo requiere
		if (config_has_property(config, "QUANTUM")) {
			g_Quantum_Planificador = config_get_int_value(config, "QUANTUM");
		} else{
			Error("No se pudo leer el parametro QUANTUM");
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
