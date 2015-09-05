#include "planificador.h"

int main(int argv, char** argc) {

	int iThreadConsola, iThreadOrquestador;					//Hilo de consola

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

	//Hilo orquestador conexiones para escuchar a Marta o a Nodos
	if ((iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL )) != 0){
		fprintf(stderr, (char *)NosePuedeCrearHilo, iThreadOrquestador);
		exit(EXIT_FAILURE);
	};

	pthread_join(hConsola, NULL );
	pthread_join(iThreadOrquestador, NULL );



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

int ChartToInt(char x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);
	//char* aux = malloc(1 * sizeof(char));
	//sprintf(aux, "%c", x);
	numero = strtol(aux, (char **) NULL, 10);

	if (aux != NULL )
		free(aux);
	return numero;
}

int CharAToInt(char* x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);

	numero = strtol(aux, (char **) NULL, 10);

	if (aux != NULL )
		free(aux);
	return numero;
}

int PosicionDeBufferAInt(char* buffer, int posicion) {
	int logitudBuffer = 0;
	logitudBuffer = strlen(buffer);

	if (logitudBuffer <= posicion)
		return 0;
	else
		return ChartToInt(buffer[posicion]);
}

int ObtenerTamanio (char *buffer , int posicion, int dig_tamanio){
	int x,digito,aux=0;
	for(x=0;x<dig_tamanio;x++){
		digito=PosicionDeBufferAInt(buffer,posicion+x);
		aux=aux*10+digito;
	}
	return aux;
}

int ObtenerComandoMSJ(char* buffer) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return PosicionDeBufferAInt(buffer, 0);
}

int AtiendeCliente(void * arg) {
	int socket = (int) arg;
	//int id=-1;

	int longitudBuffer;

// Es el encabezado del mensaje. Nos dice quien envia el mensaje
	int emisor = 0;

// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char* buffer;
	buffer = malloc(BUFFERSIZE * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

// Cantidad de bytes recibidos.
	int bytesRecibidos;

// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

// Código de salida por defecto
	int code = 0;
	int cantRafaga=1,tamanio=0;
	char * mensaje;
	while ((!desconexionCliente) & g_Ejecutando) {
		//	buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.
		if (buffer != NULL )
			free(buffer);
		buffer = string_new();

		//Recibimos los datos del cliente
		buffer = RecibirDatos(socket, buffer, &bytesRecibidos,&cantRafaga,&tamanio);


		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			emisor = ObtenerComandoMSJ(buffer);
			//Evaluamos los comandos
						switch (emisor) {
						case 1:
							mensaje="Ok";
							break;
						default:
							break;
						}
			longitudBuffer=strlen(mensaje);
			//printf("\nRespuesta: %s\n",buffer);
			// Enviamos datos al cliente.
			EnviarDatos(socket, mensaje,longitudBuffer);
		} else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

void HiloOrquestadorDeConexiones() {

	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		log_info(logger,
				"No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		log_info(logger,"Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(g_Puerto_Planificador);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		log_info(logger,"Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		log_info(logger,
				"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	//	log_trace(logger,
	//		"SOCKET LISTO PARA RECBIR CONEXIONES. Numero de socket: %d, puerto: %d",
		//	socket_host, fs_Puerto);

	while (g_Ejecutando) {
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host,(struct sockaddr *) &client_addr, &size_addr)) != -1) {
			//log_trace(logger,
			//		"NUEVA CONEXION ENTRANTE. Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d",
				//	inet_ntoa(client_addr.sin_addr), client_addr.sin_port,
					//socket_client);
			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			//sem_wait(&semHilos);
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente,
					(void *) socket_client);
			//sem_post(&semHilos);
		} else {
			log_info(logger,"ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}
	CerrarSocket(socket_host);
}



char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio) {
	*bytesRecibidos = 0;
	char *bufferAux = malloc(1);
	memset(bufferAux,0,1);
	int digTamanio;
	if (buffer != NULL ) {
		free(buffer);
	}

	if(*cantRafaga==1){
		bufferAux = realloc(bufferAux,BUFFERSIZE * sizeof(char));
		memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

		if ((*bytesRecibidos = *bytesRecibidos+recv(socket, bufferAux, BUFFERSIZE, 0)) == -1) {
			log_info(logger,"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
		}

		digTamanio=PosicionDeBufferAInt(bufferAux,1);
		*tamanio=ObtenerTamanio(bufferAux,2,digTamanio);


	}else if(*cantRafaga==2){
		bufferAux = realloc(bufferAux,*tamanio * sizeof(char)+1);
		memset(bufferAux, 0, *tamanio * sizeof(char)+1); //-> llenamos el bufferAux con barras ceros.

		if ((*bytesRecibidos = *bytesRecibidos+recv(socket, bufferAux, *tamanio, 0)) == -1) {
			Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
		}
	}

	//log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s tamanio:%d", socket,
		//	(char*) bufferAux, strlen(bufferAux));
	return bufferAux; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
}

int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar) {
// Retardo antes de contestar una solicitud
	//sleep(g_Retardo / 1000);

	int bytecount;

	//printf("CantidadBytesAEnviar:%d\n",cantidadDeBytesAEnviar);

	if ((bytecount = send(socket, buffer, cantidadDeBytesAEnviar, 0)) == -1)
		log_info(logger,"No puedo enviar información a al clientes. Socket: %d", socket);
	//printf("Cuanto Envie:%d\n",bytecount);
	//Traza("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	//char * bufferLogueo = malloc(5);
	//bufferLogueo[cantidadDeBytesAEnviar] = '\0';

	//memcpy(bufferLogueo,buffer,cantidadDeBytesAEnviar);
	if(strlen(buffer)<50){
		//log_info(logger, "ENVIO DATOS. socket: %d. Buffer:%s ",socket,(char*) buffer);
	} else {
		//log_info(logger, "ENVIO DATOS. socket: %d. Tamanio:%d ",socket,strlen(buffer));
	}

	return bytecount;
}

void CerrarSocket(int socket) {
	close(socket);
	//Traza("SOCKET SE CIERRA: (%d).", socket);
	//log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
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
