#include "cpu.h"


int main(int argv, char** argc) {

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);
	lista_global=list_create();
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
		t_global *la_global = global_create(g_Puerto_CPU);
		list_add(lista_global, la_global);
		g_Puerto_CPU++;
		}

	for(i=0;i<g_Cant_Hilos;i++){
		pthread_join(hCpu[i],NULL);
	}

	return EXIT_SUCCESS;
}

void iniciarCpu(void* arg){

	puerto=(int)arg;
	printf("Hola entre, puerto: %d\n", puerto);
	conectarsePlanificador(puerto);
	conectarseMemoria(puerto);
	//crearEscucha();
	HiloOrquestadorDeConexiones(puerto);

	//Hilo orquestador conexiones

	/*int iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL,
		(void*) HiloOrquestadorDeConexiones, NULL );

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
		//ENVIO a PLANIFICADOR
		//11210127.0.0.1143000 primer conexion, manda ip y puerto.
		string_append(&buffer,"11");
		aux=obtenerSubBuffer("127.0.0.1");
		string_append(&buffer,aux);
		aux=obtenerSubBuffer(string_itoa(puertoCpu));
		string_append(&buffer,aux);

		EnviarDatos(socket_Plani,buffer, strlen(buffer));
		printf("Conexion ok Planificador.BUFFER: %s\n",buffer);
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
	char*aux;
	if(conectarMemoria(&socket_Memoria)){
				//ENVIO a MEMORIA
				//11210127.0.0.1143000 primer conexion, manda ip y puerto.
				string_append(&buffer,"11");
				aux=obtenerSubBuffer(g_Ip_Planificador); //Le mando la ip del planificador que es la misma que CPU
				string_append(&buffer,aux);
				aux=obtenerSubBuffer(string_itoa(puertoCpu));
				string_append(&buffer,aux);

				EnviarDatos(socket_Memoria,buffer, strlen(buffer));
				printf("Conexion ok Memoria.BUFFER: %s\n",buffer);
				bufferR = RecibirDatos(socket_Memoria,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);

				free(buffer);
				free(bufferR);
				free(bufferE);
				} else {
					printf("No se pudo conectar al Planificador\n");
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
	string_append_with_format(&aux, "%s", x);

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
	my_addr.sin_port = htons(puerto);
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
			struct struct_atiende args;
			args.puertoCpu=puerto;
			args.socket=socket_client;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente,
					(void *) &args);
		} else {
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}
	CerrarSocket(socket_host);
}

int AtiendeCliente(void * arg) {
	struct struct_atiende *args = arg;
	puerto=args->puertoCpu;
	//int id=-1;
	//puerto=g_Puerto_CPU-1;
	int longitudBuffer;
	int socket=args->socket;
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
						case 2:
							printf("\nCPU Buffer RECIBIDO: %s\n",buffer);
							funcion = ObtenerComandoMSJ(buffer+1);
							if(funcion==1){
								//printf("Funcion de Ejecucion\n");
								//printf("Buffer : %s\n",buffer);
								//printf("PID %d\n",obtenerPID(buffer));
								//printf("Direccion: %s\n",obtenerDireccion(buffer));
								//printf("Instruccion: %s\n",obtenerProximaInstruccion(buffer));
								t_global* la_global = buscarGlobalPorPuerto(puerto);
								la_global->pidGlobal=obtenerPID(buffer);

								//Asigno el pid que se va a ejecutar a la lista global
								abrirArchivo(obtenerDireccion(buffer),CharAToInt(obtenerProximaInstruccion(buffer)),obtenerPID(buffer));

							}
							if(funcion==2){
								//Solicitud de estado de CPU
								printf("Solicitud de estado de CPU\n");
							}
							mensaje="ok";
							break;
						case 3:
							//Respuestas de la Memoria
							printf("MEMORIA Buffer RECIBIDO: %s\n",buffer);
							funcion = ObtenerComandoMSJ(buffer+1);
							switch(funcion){
								case 6:
									if(ObtenerComandoMSJ(buffer+4)==0){
										printf("ERROR!!!! No se puede escribir \n");
										t_global* la_global = buscarGlobalPorPuerto(puerto);
										(la_global->finError)=1;
										string_append(&(la_global->resultado),obtenerSubBuffer("30"));
									}
										else
										{
											printf("Escritura realizada \n");
											t_global* la_global = buscarGlobalPorPuerto(puerto);
											string_append(&(la_global->resultado),obtenerSubBuffer("13"));
										}

									break;
								case 7:
									if(ObtenerComandoMSJ(buffer+4)==0){
										printf("ERROR!!!! No se pudo iniciar el proceso\n");

										t_global* la_global = buscarGlobalPorPuerto(puerto);
										(la_global->finError)=1;
										string_append(&(la_global->resultado),obtenerSubBuffer("10"));
									}
									else{
										//printf("Proceso Iniciado \n");
										t_global* la_global = buscarGlobalPorPuerto(puerto);
										string_append(&(la_global->resultado),obtenerSubBuffer("11"));
									}
									break;
								case 8:
										//printf("Lectura realizada\n");
										leerPlanificador(buffer);

									break;
								case 9:
									{	//printf("Finalizacion\n"); //Grabar Texto
										t_global* la_global = buscarGlobalPorPuerto(puerto);
										string_append(&(la_global->resultado),obtenerSubBuffer("5"));

										if(finalizarPlanificador()==-1){
											printf("ERROR!!!! no pudo enviar resultado\n");
										}

									}
									break;
								default:
									break;
								}

							t_global* la_global = buscarGlobalPorPuerto(puerto);
							sem_post(&(la_global->sProxInstruccion));
							//Una vez que recibo la respuesta de memoria activo el semaforo para que siga con la siguiente intruccion.

							mensaje="ok";

						break;
						default:
							break;
						}

			longitudBuffer=strlen(mensaje);

			// Enviamos datos al cliente.
			EnviarDatos(socket, mensaje,longitudBuffer);

		} else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

void abrirArchivo(char* direccion, int instruccionAEjecutar, int pid){
FILE *archivoMcod;
archivoMcod = fopen(direccion,"r");
char *line = NULL;
size_t len = 0;
if (archivoMcod == NULL) {
	Error("Error al abrir el archivo");
}
int linea = 1;
int error =0;
int fin =0;
int ent_sal=0;
while ((getline(&line, &len, archivoMcod) != -1) && (linea!=instruccionAEjecutar)) {
	linea++;
}
do{
	t_global* la_global = buscarGlobalPorPuerto(puerto);
	//Busco en la lista por puerto, y le resto uno al semaforo para que se bloquee esperando la respuesta de memoria
	sem_wait(&(la_global->sProxInstruccion));
	la_global->instrucRealizadasGlobal++;
	printf("\nNro Inst: %d. RESULTADO PARCIAL: %s\n",la_global->instrucRealizadasGlobal, la_global->resultado);


//Le saco el \n final a la linea ingresada
char **sinBarraN = string_split(line, "\n");
//Separo el comando y su campo ingresado en caso que tenga.
char **sinBarraPunto = string_split(sinBarraN[0], ";");
char **comando = string_split(sinBarraPunto[0], " ");
char **comando2 = string_split(sinBarraN[0], "\"");

printf("\nIntruccion: %s\n",comando[0]);
if (strcmp(comando[0], "iniciar") == 0) {
	if(iniciar(CharAToInt(comando[1]),pid)==-1)
		printf("Error, no se pudo iniciar");

	sleep(g_Retardo);
}

if (strcmp(comando[0], "leer") == 0) {
	if(leer(CharAToInt(comando[1]),pid)==-1)
		printf("Error, no se pudo leer");

	sleep(g_Retardo);
}

if (strcmp(comando[0], "escribir") == 0) {
	if(escribir(CharAToInt(comando[1]),comando2[1],pid)==-1)
		printf("Error, no se pudo leer");

	sleep(g_Retardo);
}
if (strcmp(comando[0], "entrada-salida") == 0) {
	if(entradaSalida(CharAToInt(comando[1]),pid)==-1)
		printf("Error, no se pudo leer");
	ent_sal=1;
	sleep(g_Retardo);
}

if (strcmp(comando[0], "finalizar") == 0) {
	printf("\n\nFinalizar\n\n");
	fin = 1;
	if(finalizar(pid)==-1)
		printf("Error, no se pudo leer");

	sleep(g_Retardo);
}

error = (la_global->finError);
}while((getline(&line, &len, archivoMcod) != -1) && (error==0)&& (fin==0) && (ent_sal==0)); //Fin de archivo, Error, o Finalizado. agregar quantum

if(error==1){
	finalizarPlanificador();
}
if(ent_sal==1){
	//enviar msj a memoria para avisar que termine la rafaga a memoria
	finRafaga(pid);
}

fclose(archivoMcod);


}

int finRafaga(int pid){
	printf("\n\n !!!!se utiliza la funcion finalizar momentaneamente!!! \n\n");
	return finalizar(pid); //momentaneamnete usamos esta funcion. ojo, el PID no debe terminar en memoria.
}

int iniciar(int paginas, int pid){

	int socket_memoria;
	conectarMemoria(&socket_memoria);
	//13+puerto+pid+paginas
	char* buffer = string_new();
	string_append(&buffer,"13");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(paginas)));
	printf("(iniciar) ENVIADO a MEMORIA: %s\n",buffer);
	return EnviarDatos(socket_memoria, buffer,strlen(buffer));
}

int leer(int paginas, int pid){

	int socket_memoria;
	conectarMemoria(&socket_memoria);
	//14+puerto+pid+pagina
	char* buffer = string_new();
	string_append(&buffer,"14");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(paginas)));
	printf("(leer) ENVIADO a MEMORIA: %s\n",buffer);
	return EnviarDatos(socket_memoria, buffer,strlen(buffer));
}

int escribir(int paginas,char* texto,int pid){
	int socket_memoria;
	conectarMemoria(&socket_memoria);
	//15+puerto+pid+pagina+contenido
	char* buffer = string_new();
	string_append(&buffer,"15");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(paginas)));
	string_append(&buffer,obtenerSubBuffer(texto));
	printf("(escribir) ENVIADO a MEMORIA: %s\n",buffer);
	return EnviarDatos(socket_memoria, buffer,strlen(buffer));
}

int entradaSalida(int tiempo,int pid){
	int socket_Plani;
	conectarPlanificador(&socket_Plani);
	//13+puerto+pid+TiempoBloqueado+CantIntrucciones+Resultados
	char* buffer = string_new();
	t_global* la_global = buscarGlobalPorPuerto(puerto);

	string_append(&buffer,"13");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(tiempo)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(la_global->instrucRealizadasGlobal)));
	string_append(&buffer,obtenerSubBuffer(la_global->resultado));
	printf("(Ent-Sal) ENVIADO a PLANIFICADOR: %s\n",buffer);

	//Reinicio las variables del hilo:
	la_global->instrucRealizadasGlobal=0;
	la_global->finError =0;
	la_global->resultado=string_new();

	//AGREGAR RESULTADO PARCIAL
	return EnviarDatos(socket_Plani, buffer,strlen(buffer));
}

int finalizar(int pid){
	//printf("SOY EL PUERTO:%d",puerto);
	int socket_Memoria;
	conectarMemoria(&socket_Memoria);
	//12+puerto+pid
	char* buffer = string_new();
	string_append(&buffer,"12");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
	printf("(Fin) ENVIADO a MEMORIA: %s\n",buffer);
	return EnviarDatos(socket_Memoria, buffer,strlen(buffer));
}

int finalizarPlanificador(){
	int socket_Plani;
	conectarPlanificador(&socket_Plani);
	//12+puerto+pid+CantIntruccionesRealizadas+Resultados
	t_global* la_global = buscarGlobalPorPuerto(puerto);

	char* buffer = string_new();
	string_append(&buffer,"12");
	string_append(&buffer,obtenerSubBuffer(string_itoa(puerto)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(la_global->pidGlobal)));
	string_append(&buffer,obtenerSubBuffer(string_itoa(la_global->instrucRealizadasGlobal)));
	string_append(&buffer,obtenerSubBuffer(la_global->resultado));

	la_global->instrucRealizadasGlobal=0;
	la_global->finError =0;
	//free(la_global->resultado);
	la_global->resultado=string_new();
	printf("(FIN) ENVIADO a PLANIFICADOR: %s\n",buffer);
	return EnviarDatos(socket_Plani, buffer,strlen(buffer));
}

int devolverValorNumericoArchivo(char caracter,int numero){
	int digito;
	if(caracter!=';'){
		digito=ChartToInt(caracter);
		if(numero==0){
			numero=digito;
		}else{
			numero=numero*10+digito;
		}
	}
return numero;
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

int obtenerPID(char* buffer){

	int cantDigPID,posicion,cantPID;
	int j=0,i=0;
	int PID=0;
	cantDigPID = ObtenerComandoMSJ(buffer + 2);
	cantPID = ObtenerTamanio(buffer,3,cantDigPID);
	posicion= 2+cantDigPID+1;

		for (j = posicion + i; j < posicion + i + cantPID; j++) {
			PID= devolverValorNumericoArchivo(buffer[j],PID);
		}
	return PID;
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

t_global* buscarGlobalPorPuerto(int puerto) {
	t_global* la_cpu = malloc(sizeof(t_global));
	bool _true(void *elem) {
		return (((t_global*) elem)->puerto == puerto);
	}
	la_cpu = list_find(lista_global, _true);
	return la_cpu;
}

int leerPlanificador(char* buffer) {

	char *pagina, *contenido,*armarResultado;
	armarResultado=string_new();
	int posActual = 2;
	t_global* la_global = buscarGlobalPorPuerto(puerto);
	pagina = DigitosNombreArchivo(buffer, &posActual);
	//printf("Ip:%s\n",la_Ip);
	contenido = DigitosNombreArchivo(buffer, &posActual);
	//printf("Puerto:%s\n",el_Puerto);
	string_append(&armarResultado,"2");
	string_append(&armarResultado,obtenerSubBuffer(pagina));
	string_append(&armarResultado,obtenerSubBuffer(contenido));
	string_append(&(la_global->resultado),obtenerSubBuffer(armarResultado));

	free(armarResultado);
	return 1;
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
