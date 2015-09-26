#include "memoria.h"

int main(int argv, char** argc) {

	//int iThreadOrquestador;


	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "memoria", true, LOG_LEVEL_TRACE);

	// Levantamos el archivo de configuracion.
	LevantarConfig();

	ConectarseConSwap(g_Puerto_Memoria);

	if(strcmp(g_Tlb_Habilitada,"SI")==0){
		crearTLB(g_Entradas_Tlb);
	}

	HiloOrquestadorDeConexiones();
	//Hilo orquestador conexiones para escuchar
	//	if ((iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL )) != 0){
		//	fprintf(stderr, (char *)NosePuedeCrearHilo, iThreadOrquestador);
			//exit(EXIT_FAILURE);
		//};
		//pthread_join(iThreadOrquestador, NULL );

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
	my_addr.sin_port = htons(g_Puerto_Memoria);
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

		int funcion;
		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			emisor = ObtenerComandoMSJ(buffer);
			//Evaluamos los comandos
						switch (emisor) {
						case 1: //MSJs que manda CPU
						funcion = ObtenerComandoMSJ(buffer+1);
					   // switch con los distintos codigos de mensaje
							switch (funcion){
								case 1:
									informarConexionCPU(buffer);
								break;

								case 2:
									printf("arrancando a correr programa\n");
									int socket_swap;
									conectarConSwap(&socket_swap);
									EnviarDatos(socket_swap, "32",2);
									mensaje="Ok";
								break;
							}
						break;

					//	case 4: PARA LOS MSJs que manda SWAP

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

void ConectarseConSwap(int g_Puerto_Memoria){
	int socket_swap;
//	int bytesRecibidos,cantRafaga=1,tamanio;
	char*buffer = string_new();
//	char*bufferR = string_new();

//	char*bufferE = string_new();

	if(conectarConSwap(&socket_swap)){
		printf("Conexion con Swap exitosa.\n");

		string_append(&buffer,"31");

	//	EnviarDatos(socket_swap,buffer, strlen(buffer));
	//	bufferR = RecibirDatos(socket_Memoria,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);

		free(buffer);

//		free(bufferR);
//		free(bufferE);
	}
	else {
		printf("No se pudo conectar al swap\n");
	}
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

	log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s tamanio:%d", socket,
	(char*) bufferAux, strlen(bufferAux));
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

void informarConexionCPU(char* buffer){
	char *el_Puerto, *ip_CPU;
	int posActual = 2;

	printf("Se conecto CPU con:\n");

	ip_CPU = DigitosNombreArchivo(buffer, &posActual);
	printf("ip CPU:%s\n", ip_CPU);

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto CPU:%s\n", el_Puerto);

}


char* DigitosNombreArchivo(char *buffer, int *posicion) {

			char *nombreArch;
			int digito = 0, i = 0, j = 0, algo = 0, aux = 0, x = 0;

			digito = PosicionDeBufferAInt(buffer, *posicion);
			for (i = 1; i <= digito; i++) {
				algo = PosicionDeBufferAInt(buffer, *posicion + i);
				aux = aux * 10 + algo;
			}
			nombreArch = malloc(aux + 1);
			for (j = *posicion + i; j < *posicion + i + aux; j++) {
				nombreArch[x] = buffer[j];
				x++;
			}
			nombreArch[x] = '\0';
			*posicion = *posicion + i + aux;
			return nombreArch;
		}



void CerrarSocket(int socket) {
	close(socket);
	//Traza("SOCKET SE CIERRA: (%d).", socket);
	//log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
}


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

t_list* crearTLB(int cant_entradas){
	t_list TLB;
	TLB = list_create();
	return TLB;
}

