#include "cpu.h"

int main(int argv, char** argc) {

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);

	// Levantamos el archivo de configuracion.
	LevantarConfig();
	pthread_t hCpu[g_Cant_Hilos]; 			//Hilo de conexion
	int i = 0;
	//Conectamos con Planificador y con Memoria:
	while(i<g_Cant_Hilos){

	int iThreadCpu = pthread_create(&hCpu[i], NULL,
			(void*) iniciarCpu,(void*) g_Puerto_CPU );

		if (iThreadCpu) {
			fprintf(stderr,
				"Error al crear hilo - pthread_create() return code: %d\n",
				iThreadCpu);
			exit(EXIT_FAILURE);
		}
		i++;
		g_Puerto_CPU++;
		}

	for(i=0;i<g_Cant_Hilos;i++){
		pthread_join(hCpu[i],NULL);
	}

	return EXIT_SUCCESS;
}

void iniciarCpu(void* arg){
	int puertoCpu;
	puertoCpu=(int)arg;
	printf("Hola entre, puerto: %d\n", puertoCpu);
	conectarsePlanificador(puertoCpu);
	conectarseMemoria(puertoCpu);
	HiloOrquestadorDeConexiones(puertoCpu);

	//Hilo orquestador conexiones

	/*int iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL,
		(void*) HiloOrquestadorDeConexiones, puertoCpu );

	if (iThreadOrquestador) {
		fprintf(stderr,
			"Error al crear hilo - pthread_create() return code: %d\n",
			iThreadOrquestador);
		exit(EXIT_FAILURE);
	}

	pthread_join(hOrquestadorConexiones, NULL );
	 */

}
void conectarsePlanificador(int puertoCpu){
	int socket_Plani;
	int bytesRecibidos,cantRafaga=1,tamanio;
	char*buffer = string_new();
	char*bufferR = string_new();
	char*bufferE = string_new();
	char*aux;

	if( conectarPlanificador(&socket_Plani)){
		printf("Conexion ok Planificador\n");

		//ENVIO a PLANIFICADOR
		//11210127.0.0.1143000 primer conexion, manda ip y puerto.
		string_append(&buffer,"11");
		aux=obtenerSubBuffer("127.0.0.1");
		string_append(&buffer,aux);
		aux=obtenerSubBuffer(string_itoa(puertoCpu));
		string_append(&buffer,aux);

		EnviarDatos(socket_Plani,buffer, strlen(buffer));
		bufferR = RecibirDatos(socket_Plani,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);

		free(buffer);
		free(bufferR);
		free(bufferE);
		} else {
			printf("No se pudo conectar al Planificador\n");
		}
}

void conectarseMemoria(int puertoCpu){
	int socket_Memoria;
	int bytesRecibidos,cantRafaga=1,tamanio;
	char*buffer = string_new();
	char*bufferR = string_new();
	char*bufferE = string_new();

	if(conectarMemoria(&socket_Memoria)){
		printf("Conexion ok Memoria\n");
		string_append(&buffer,"11");
			//EnviarDatos(socket_Memoria,buffer, strlen(buffer));
			//bufferR = RecibirDatos(socket_Memoria,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);

		free(buffer);
		free(bufferR);
		free(bufferE);
	}else {
		printf("No se pudo conectar a la memoria\n");
	}
}

char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio) {
	*bytesRecibidos = 0;
	char *bufferAux= malloc(1);
	int digTamanio;
	if (buffer != NULL ) {
		free(buffer);
	}

	if(*cantRafaga==1){
		bufferAux = realloc(bufferAux,BUFFERSIZE * sizeof(char));
		memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

		if ((*bytesRecibidos = *bytesRecibidos+recv(socket, bufferAux, BUFFERSIZE, 0)) == -1) {
			Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
		}

		digTamanio=PosicionDeBufferAInt(bufferAux,1);
		*tamanio=ObtenerTamanio(bufferAux,2,digTamanio);
		//printf(COLOR_VERDE"TAMAÑO:%d\n"DEFAULT,*tamanio);
	}else if(*cantRafaga==2){
		bufferAux = realloc(bufferAux,*tamanio * sizeof(char)+1);
		memset(bufferAux, 0, *tamanio * sizeof(char)+1); //-> llenamos el bufferAux con barras ceros.

		//printf("ANTES BYTESRECIBIDO:%d\n",*bytesRecibidos);
		if ((*bytesRecibidos = *bytesRecibidos+recv(socket, bufferAux, *tamanio, 0)) == -1) {
			Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",socket);
		}
		//printf("DESPUES BYTESRECIBIDO:%d\n",*bytesRecibidos);
	}

	if(strlen(bufferAux)<100){
		//log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s tamanio:%d", socket,(char*) bufferAux, strlen(bufferAux));
	} else {
		//log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s tamanio:%d y mi rafaga:%d", socket,"soy grande", strlen(bufferAux),*cantRafaga);
	}
	return bufferAux; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
}

int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar) {
	int bytecount;

	if ((bytecount = send(socket, buffer, cantidadDeBytesAEnviar, 0)) == -1)
		Error("No puedo enviar información al cliente. Socket: %d", socket);

	return bytecount;
}

int ObtenerTamanio (char *buffer , int posicion, int dig_tamanio){
	int x,digito,aux=0;
	for(x=0;x<dig_tamanio;x++){
		digito=PosicionDeBufferAInt(buffer,posicion+x);
		aux=aux*10+digito;
	}
	return aux;
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

int conectarMemoria(int *socket_memoria){
	//Conecto a la memoria principal
	//ESTRUCTURA DE SOCKETS; EN ESTE CASO CONECTA CON NODO
		//log_info(logger, "Intentando conectar a nodo\n");
		//conectar con Nodo
		struct addrinfo hints;
		struct addrinfo *serverInfo;
		int conexionOk = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP


		if (getaddrinfo(g_Ip_Memoria, g_Puerto_Memoria, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
			log_info(logger,
					"ERROR: cargando datos de conexion socket_memoria");
		}

		if ((*socket_memoria = socket(serverInfo->ai_family, serverInfo->ai_socktype,
				serverInfo->ai_protocol)) < 0) {
			log_info(logger, "ERROR: crear socket_memoria");
		}
		if (connect(*socket_memoria, serverInfo->ai_addr, serverInfo->ai_addrlen)
				< 0) {
			log_info(logger, "ERROR: conectar socket_memoria");
		} else {
			conexionOk = 1;
		}
		freeaddrinfo(serverInfo);	// No lo necesitamos mas
		return conexionOk;
}

int PosicionDeBufferAInt(char* buffer, int posicion) {
	long unsigned logitudBuffer = 0;
	logitudBuffer = strlen(buffer);

	if (logitudBuffer <= posicion)
		return 0;
	else
		return ChartToInt(buffer[posicion]);
}


int ChartToInt(char x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);

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

char* obtenerSubBuffer(char *nombre){
	// Esta funcion recibe un nombre y devuelve ese nombre de acuerdo al protocolo. Ej: carlos ------> 16carlos
	char *aux=string_new();
	int tamanioNombre=0;
	float tam=0;
	int cont=0;

	tamanioNombre=strlen(nombre);
	tam=tamanioNombre;
	while(tam>=1){
		tam=tam/10;
		cont++;
	}
	string_append(&aux,string_itoa(cont));
	string_append(&aux,string_itoa(tamanioNombre));
	string_append(&aux,nombre);

	return aux;
}

int cuentaDigitos(int valor){
	int cont = 0;
	float tamDigArch=valor;

	while(tamDigArch>=1){
		tamDigArch=tamDigArch/10;
		cont++;
	}
	return cont;
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

void HiloOrquestadorDeConexiones(int puertoCpu) {

	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		ErrorFatal(
				"No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		ErrorFatal("Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(puertoCpu);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal(
				"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	//Traza("El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d", socket_host, g_Puerto);
	//log_trace(logger,
		//	"SOCKET LISTO PARA RECBIR CONEXIONES. Numero de socket: %d, puerto: %d",
			//socket_host, g_Puerto_Nodo);

	while (g_Ejecutando) {
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host,(struct sockaddr *) &client_addr, &size_addr)) != -1) {
			//Traza("Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);
			//log_trace(logger,
				//	"NUEVA CONEXION ENTRANTE. Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d",
					//inet_ntoa(client_addr.sin_addr), client_addr.sin_port,
					//socket_client);
			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente,
					(void *) socket_client);
		} else {
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}
	CerrarSocket(socket_host);
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
		//char* linea;
		//linea = malloc(100);
		//linea ="212101234567989277/home/utnso/workspace/tp-2015-2c-residente/cache13/planificador/programa.mcod1233";
		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			emisor = ObtenerComandoMSJ(buffer);
			//Evaluamos los comandos
						switch (emisor) {
						case 2:

							funcion = ObtenerComandoMSJ(buffer+1);
							if(funcion==1){
								printf("Funcion de Ejecucion\n");
								printf("Buffer : %s\n",buffer);
								printf("PID %s\n",obtenerPID(buffer));
								printf("Direccion: %s\n",obtenerDireccion(buffer));
								printf("Instruccion: %s\n",obtenerProximaInstruccion(buffer));
								//printf("arrancando a correr programa\n");


								int socket_memoria;
								conectarMemoria(&socket_memoria);
								EnviarDatos(socket_memoria, "12",2);
							}
							if(funcion==2){
								//Solicitud de estado de CPU
								printf("Solicitud de estado de CPU\n");
							}
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

char* obtenerProximaInstruccion(char* buffer){

	int cantDigPID, cantDigDIR, cantDIR,posicion,cantPID,cantDigProxInt,cantProxInt;
	int j=0,i=0,x=0;
	char *direccion;

	cantDigPID = ObtenerComandoMSJ(buffer + 2);
	cantPID = ObtenerTamanio(buffer,3,cantDigPID);
	cantDigDIR = ObtenerComandoMSJ(buffer + 2 + cantDigPID + cantPID + 1);
	cantDIR = ObtenerTamanio(buffer,3 + cantDigPID + cantPID + 1,cantDigDIR);
	cantDigProxInt = ObtenerComandoMSJ(buffer + 2 + cantDigPID + cantPID + cantDigDIR + cantDIR + 2);
	cantProxInt = ObtenerTamanio(buffer,3 + cantDigPID + cantPID + cantDIR + cantDigDIR + 2,cantDigProxInt);
	posicion= 2 + cantDigPID + 1 + cantPID + cantDIR + cantDigDIR + 1 + cantDigProxInt + 1 ;
	direccion = malloc(cantProxInt + 1);
		for (j = posicion + i; j < posicion + i + cantProxInt; j++) {
			direccion[x] = buffer[j];
			x++;
		}
	direccion[x] = '\0';
	return direccion;
}

char* obtenerDireccion(char* buffer){

	int cantDigPID, cantDigDIR, cantDIR,posicion,cantPID;
	int j=0,i=0,x=0;
	char *direccion;

	cantDigPID = ObtenerComandoMSJ(buffer + 2);
	cantPID = ObtenerTamanio(buffer,3,cantDigPID);
	cantDigDIR = ObtenerComandoMSJ(buffer + 2 + cantDigPID + cantPID + 1);
	cantDIR = ObtenerTamanio(buffer,2 + cantDigPID + cantPID + 2,cantDigDIR);
	posicion= 2 + cantDigPID + 1 + cantPID + cantDigDIR + 1;
	direccion = malloc(cantDIR + 1);
		for (j = posicion + i; j < posicion + i + cantDIR; j++) {
			direccion[x] = buffer[j];
			x++;
		}
	direccion[x] = '\0';
	return direccion;
}

char* obtenerPID(char* buffer){

	int cantDigPID,posicion,cantPID;
	int j=0,i=0,x=0;
	char *PIDChar;

	cantDigPID = ObtenerComandoMSJ(buffer + 2);
	cantPID = ObtenerTamanio(buffer,3,cantDigPID);
	posicion= 2+cantDigPID+1;

	PIDChar = malloc(cantPID+1);

		for (j = posicion + i; j < posicion + i + cantPID; j++) {
			PIDChar[x] = buffer[j];
			x++;
		}
		PIDChar[x] = '\0';

	return PIDChar;
}

void CerrarSocket(int socket) {
	close(socket);
	//Traza("SOCKET SE CIERRA: (%d).", socket);
	//log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
}


int ObtenerComandoMSJ(char* buffer) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return PosicionDeBufferAInt(buffer, 0);
}
