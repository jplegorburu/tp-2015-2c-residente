#include "memoria.h"


int main(int argv, char** argc) {

	//int iThreadOrquestador;
	sem_init(&sem_swap,0,1); //semaforo para conectarse con swap
	sem_init(&sem_Operacion,0,1); //semaforo para conectarse con swap
	sem_init(&sPrueba1,0,1); //semaforo para conectarse con swap
	sem_init(&sPrueba2,0,1); //semaforo para conectarse con swap
	lista_cpu=list_create();  //Creo la lista de las cpu.
	lista_procesos=list_create(); //Lista de procesos en memoria
	marcos = list_create();
	TLB = list_create();
	// Levantamos el archivo de configuracion.
	LevantarConfig();


	// Creamos los frames
	inicializarListaMarcos(marcos, g_Cant_Marcos);
	printf("\n 1LA LISTA TIENE %d MARCOS\n",list_size(marcos));

	memoriaPrincipal = malloc(g_Cant_Marcos*g_Tam_Marcos); //Reservo la porcion de memoria donde se van a escribir y leer los marcos.
	bzero(memoriaPrincipal,g_Cant_Marcos*g_Tam_Marcos*sizeof(char)); //Pone toda le memoria en /0

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "memoria", false, LOG_LEVEL_TRACE);



	ConectarseConSwap(g_Puerto_Memoria);


	if(strcmp(g_Tlb_Habilitada,"SI")==0){
		printf("Creo TLB\n");
		crearTLB(g_Entradas_Tlb);
	}

	//Manejo de señales
	SENIAL();

	pthread_t aciertosTLB;
	pthread_create(&aciertosTLB, NULL, (void*)calcularTlbHits, NULL);


	HiloOrquestadorDeConexiones();

	//Hilo orquestador conexiones para escuchar
	//	if ((iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL )) != 0){
		//	fprintf(stderr, (char *)NosePuedeCrearHilo, iThreadOrquestador);
			//exit(EXIT_FAILURE);
		//};
		//pthread_join(iThreadOrquestador, NULL );
	pthread_join(aciertosTLB,NULL);
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

		// Preguntamos y obtenemos la ip del swap
		if (config_has_property(config, "IP_MEMORIA")) {
			g_Ip_Memoria = config_get_string_value(config,"IP_MEMORIA");
		} else{
			Error("No se pudo leer el parametro IP_MEMORIA");
		}

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

		// ALGORITMO TLB: FIFO
		if (config_has_property(config, "ALGORITMO_TLB")) {
			g_Algoritmo_TLB = config_get_string_value(config,"ALGORITMO_TLB");
		} else{
			Error("No se pudo leer el parametro ALGORITMO_TLB");
		}

		// Obtenemos el tiempo de retardo que tiene la memoria
		if (config_has_property(config, "RETARDO_MEMORIA")) {
			g_Retardo_Memoria = config_get_int_value(config, "RETARDO_MEMORIA");
		} else{
			Error("No se pudo leer el parametro RETARDO_MEMORIA");
		}

		// Algoritmo de reemplazo (FIFO, LRU o CLOCK-M)
		if (config_has_property(config, "ALGORITMO_REEMPLAZO")) {
			g_Algoritmo_Reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
		} else{
			Error("No se pudo leer el parametro ALGORITMO_REEMPLAZO");
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
	string_append_with_format(&aux, "%s", x);

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

		printf("\nESTOY RECIBIENDO ESTO: %s\n", buffer);


		int funcion;
		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			sem_wait(&sPrueba1);
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
									informarFinDelProceso(buffer);

								break;

								case 3:
									informarInicio(buffer);
							    break;
								case 4:
									informarLeer(buffer);
								break;
								case 5:
									informarEscribir(buffer);
								break;
							}
							mensaje = "ok";

						break;



						default:
						break;
						}
			longitudBuffer=strlen(mensaje);
			printf("\nRespuesta: %s\n",mensaje);
			// Enviamos datos al cliente.
			EnviarDatos(socket, mensaje,longitudBuffer);
			sem_post(&sPrueba1);
		} else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

void mensajeDeSwap(char * buffer){
		int funcion;
		int error;

		 //PARA LOS MSJs que manda SWAP //
		funcion = ObtenerComandoMSJ(buffer+1);
								   // switch con los distintos codigos de mensaje
		switch (funcion){
											case 1: //Inicio
												resultadoInicioSwap(buffer);
											break;

											case 2: //Lectura
												error = ObtenerComandoMSJ(buffer+2);
												if(error==0){
													printf("Error en la lectura\n");
												}else{
													resultadoLecturaSwap(buffer);
												}
											break;

											case 3: //Escritura
												resultadoEscrituraSwap(buffer);
											break;

											case 4: //Fin
												resultadoFinSwap(buffer);
											break;

											default:
												printf("ok/n");
											break;
		}



}

void finProcesoSwap(int pid){


		//35+pid
		char* buffer = string_new();
		string_append(&buffer,"35");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		EnviarDatos(socket_swap, buffer,strlen(buffer));

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		mensajeDeSwap(buffer);
}

void inicioProcesoSwap(int pid, int cant_pag){


		//32+pid+cantidad paginas
		char* buffer = string_new();
		string_append(&buffer,"32");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		string_append(&buffer,obtenerSubBuffer(string_itoa(cant_pag)));
		EnviarDatos(socket_swap, buffer,strlen(buffer));

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		printf("\nINICIO PROCESO RECIBI: %s", buffer);
		mensajeDeSwap(buffer);

}

void leerSwap(int pid, int num_pag){

		sem_wait(&sem_swap);
		//33+pid+numero pagina
		char* buffer = string_new();
		string_append(&buffer,"33");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		string_append(&buffer,obtenerSubBuffer(string_itoa(num_pag)));
		EnviarDatos(socket_swap, buffer,strlen(buffer));

		//ACCESO A SWAP
		entrada_tablaProcesos * proc = buscarPorId(pid);
		proc->accesoSwap++;

		//AtiendeCliente((void *)socket_swap);

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		sem_post(&sem_swap);
		mensajeDeSwap(buffer);
}

char* leerSwapEscribir(int pid, int num_pag){

		sem_wait(&sem_swap);
		//33+pid+numero pagina
		char* buffer = string_new();
		string_append(&buffer,"33");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		string_append(&buffer,obtenerSubBuffer(string_itoa(num_pag)));
		EnviarDatos(socket_swap, buffer,strlen(buffer));
		//AtiendeCliente((void *)socket_swap);

		//ACCESO A SWAP
		entrada_tablaProcesos * proc = buscarPorId(pid);
		proc->accesoSwap++;

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		sem_post(&sem_swap);
		char *contenido, *pid2, *pagina;
		int posActual = 2;

		printf("\nResultado Lectura...\n");

		pid2 = DigitosNombreArchivo(buffer, &posActual);
		printf("PID:%s\n", pid2);
		pagina = DigitosNombreArchivo(buffer, &posActual);
		printf("\nPAGINA TRAIDA DE SWAP:%s\n", pagina);
		contenido = DigitosNombreArchivo(buffer, &posActual);
		printf("\nCONTENIDO:%s\n", contenido);
		return contenido;
}

void escribirSwap(int pid, int num_pag, char* contenido){

	sem_wait(&sem_swap);

		//34+pid+numero pagina
		char* buffer = string_new();
		string_append(&buffer,"34");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		string_append(&buffer,obtenerSubBuffer(string_itoa(num_pag)));
		string_append(&buffer,obtenerSubBuffer(contenido));
		EnviarDatos(socket_swap, buffer,strlen(buffer));

		//ACCESO A SWAP
		entrada_tablaProcesos * proc = buscarPorId(pid);
		proc->accesoSwap++;

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		sem_post(&sem_swap);
		mensajeDeSwap(buffer);
}

void escribirSwapReemplazo(int pid, int num_pag, char* contenido){

		sem_wait(&sem_swap);
		//34+pid+numero pagina
		char* buffer = string_new();
		string_append(&buffer,"34");
		string_append(&buffer,obtenerSubBuffer(string_itoa(pid)));
		string_append(&buffer,obtenerSubBuffer(string_itoa(num_pag)));
		string_append(&buffer,obtenerSubBuffer(contenido));
		EnviarDatos(socket_swap, buffer,strlen(buffer));

		//ACCESO A SWAP
		entrada_tablaProcesos * proc = buscarPorId(pid);
		proc->accesoSwap++;

		int bytesRecibidos;
		int cantRafaga = 1, tamanio = 0;
		buffer = RecibirDatos(socket_swap, buffer, &bytesRecibidos,&cantRafaga,&tamanio);
		sem_post(&sem_swap);
}

void ConectarseConSwap(int g_Puerto_Memoria){


	int bytesRecibidos,cantRafaga=1,tamanio;
	char*buffer = string_new();
	char*bufferR = string_new();
	char*bufferE = string_new();
	char*aux;


	if(conectarConSwap(&socket_swap)){
		printf("Conexion con Swap exitosa.\n");

		string_append(&buffer,"31");

		aux=obtenerSubBuffer(g_Ip_Memoria);
		string_append(&buffer,aux);
		aux=obtenerSubBuffer(string_itoa(g_Puerto_Memoria));
		string_append(&buffer,aux);

		EnviarDatos(socket_swap,buffer, strlen(buffer));
		bufferR = RecibirDatos(socket_swap,bufferR, &bytesRecibidos,&cantRafaga,&tamanio);

		free(buffer);
		free(bufferR);
		free(bufferE);

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
	printf("Mensaje recibido: %s \n",bufferAux);
	return bufferAux; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
}


int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar) {
// Retardo antes de contestar una solicitud
	//sleep(g_Retardo / 1000);
sem_wait(&sPrueba2);
	int bytecount;

	//printf("CantidadBytesAEnviar:%d\n",cantidadDeBytesAEnviar);

	if ((bytecount = send(socket, buffer, cantidadDeBytesAEnviar, 0)) == -1)
		log_info(logger,"No puedo enviar información a al clientes. Socket: %d", socket);
	//printf("Cuanto Envie:%d\n",bytecount);
	//printf("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	//char * bufferLogueo = malloc(5);
	//bufferLogueo[cantidadDeBytesAEnviar] = '\0';

	//memcpy(bufferLogueo,buffer,cantidadDeBytesAEnviar);
	//if(strlen(buffer)<50){
		//log_info(logger, "ENVIO DATOS. socket: %d. Buffer:%s ",socket,(char*) buffer);
	//} else {
		//log_info(logger, "ENVIO DATOS. socket: %d. Tamanio:%d ",socket,strlen(buffer));
	//}
	log_trace(logger, "ENVIO DATOS. socket: %d. buffer: %s tamanio:%d", socket,	(char*) buffer, strlen(buffer));
	printf("Mensaje enviado: %s \n",buffer);
sem_post(&sPrueba2);
	return bytecount;
}

void informarConexionCPU(char* buffer){
	char *la_Ip, *el_Puerto;
	int digitosCantNumIp = 0, tamanioDeIp;
	int posActual = 0;
	t_cpu * la_cpu;

	digitosCantNumIp = PosicionDeBufferAInt(buffer, 2);

	tamanioDeIp = ObtenerTamanio(buffer, 3, digitosCantNumIp);

	if (tamanioDeIp >= 10) {
		posActual = digitosCantNumIp;
	} else {
		posActual = 1 + digitosCantNumIp;
	}
	//Chequeo el tamaño de la ip xq si es mas chica de 10 puede generar fallas.
	la_Ip = DigitosNombreArchivo(buffer, &posActual);
	printf("Ip:%s\n",la_Ip);
	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto:%s\n",el_Puerto);
	//Agrego la cpu conectada a la lista de cpu activas.
	la_cpu = cpu_create(la_Ip, el_Puerto);
	list_add(lista_cpu, la_cpu);


}

void informarFinDelProceso(char* buffer){
	char *el_Puerto, *pid;
	int posActual = 2;

	printf("Fin de Proceso:\n");

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
		printf("Puerto CPU:%s\n", el_Puerto);

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);


	//Agrego el proceso a la cpu correspondiente
	t_cpu* la_cpu =buscarCPUporPuerto(el_Puerto);
	//Busco la CPU por el puerto
	la_cpu->procesoActivo=CharAToInt(pid);
	//Le agreguo el proceso activo correspondiente.
	//AGREGO PROCESO A TABLAS DE LA MEMORIA

	bool _true(void *elem) {
			return (((entrada_tablaProcesos*) elem)->pid == CharAToInt(pid));
		}
	entrada_tablaProcesos* unProceso = list_remove_by_condition(lista_procesos, _true);
	printf("\n PROCESO %d ELIMINADO CON %d PAGS\n", unProceso->pid,list_size(unProceso->tablaPags));
	printf("\n ACCESO SWAP: %d FALLOS: %d\n", unProceso->accesoSwap,unProceso->falloPag);
	printf("\n HITS:%d\n",TLBhits);



	while(list_size(unProceso->tablaPags)!=0){
		entrada_tablaPags * entrada = list_remove(unProceso->tablaPags,0);
		entradaTablaPags_destroy(entrada);
		};
	while(list_size(unProceso->framesAsignados)!=0){
		t_marcoProceso * marcoProc = list_remove(unProceso->framesAsignados,0);
		//Libero los marcos
		t_frame* marcoDisponible = buscarFramePorNumero(marcoProc->frameNro);
		marcoDisponible->usado=0;
		marcoProceso_destroy(marcoProc);
		};
	entradaTablaProcesos_destroy(unProceso);

	sem_wait(&sem_swap);
	finProcesoSwap(CharAToInt(pid));
	sem_post(&sem_swap);


}

void AbortarProceso(char* el_Puerto, char* pid){

	//Agrego el proceso a la cpu correspondiente
	t_cpu* la_cpu =buscarCPUporPuerto(el_Puerto);
	//Busco la CPU por el puerto
	la_cpu->procesoActivo=CharAToInt(pid);
	//Le agreguo el proceso activo correspondiente.
	//AGREGO PROCESO A TABLAS DE LA MEMORIA

	bool _true(void *elem) {
			return (((entrada_tablaProcesos*) elem)->pid == CharAToInt(pid));
		}
	entrada_tablaProcesos* unProceso = list_remove_by_condition(lista_procesos, _true);
	printf("\n PROCESO %d ELIMINADO CON %d PAGS\n", unProceso->pid,list_size(unProceso->tablaPags));
	printf("\n ACCESO SWAP: %d FALLOS: %d\n", unProceso->accesoSwap,unProceso->falloPag);
	printf("\n HITS:%d\n",TLBhits);



	while(list_size(unProceso->tablaPags)!=0){
		entrada_tablaPags * entrada = list_remove(unProceso->tablaPags,0);
		entradaTablaPags_destroy(entrada);
		};
	while(list_size(unProceso->framesAsignados)!=0){
		t_marcoProceso * marcoProc = list_remove(unProceso->framesAsignados,0);
		//Libero los marcos
		t_frame* marcoDisponible = buscarFramePorNumero(marcoProc->frameNro);
		marcoDisponible->usado=0;
		marcoProceso_destroy(marcoProc);
		};
	entradaTablaProcesos_destroy(unProceso);

	sem_wait(&sem_swap);
	finProcesoSwap(CharAToInt(pid));
	sem_post(&sem_swap);


}

void informarInicio(char* buffer){
	char *el_Puerto, *pid, *cant_pag;
	int posActual = 2;
	int i;
	printf("Inicio:\n");

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
		printf("Puerto CPU:%s\n", el_Puerto);

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	cant_pag = DigitosNombreArchivo(buffer, &posActual);
	printf("Cantidad paginas:%s\n", cant_pag);

	//Agrego el proceso a la cpu correspondiente
	t_cpu* la_cpu =buscarCPUporPuerto(el_Puerto);
	//Busco la CPU por el puerto
	la_cpu->procesoActivo=CharAToInt(pid);
	//Le agrego el proceso activo correspondiente.

	//AGREGO PROCESO A TABLAS DE LA MEMORIA
	entrada_tablaProcesos * unProceso = entradaTablaProcesos_create(CharAToInt(pid));

	for (i = 0; i < CharAToInt(cant_pag); i++){
		entrada_tablaPags * entrada = entradaTablaPags_create(i);
		list_add(unProceso->tablaPags, entrada);
		};
	list_add(lista_procesos, unProceso);
	printf("\n PROCESO %d AGREGADO CON %d PAGS\n", unProceso->pid,list_size(unProceso->tablaPags));

	sem_wait(&sem_swap);
	inicioProcesoSwap(CharAToInt(pid),CharAToInt(cant_pag));
	sem_post(&sem_swap);

}

void informarLeer(char* buffer){


	char *el_Puerto, *pid, *num_pag;
	int posActual = 2;

	printf("Leer:\n");
	sem_wait(&sem_Operacion);
	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto CPU:%s\n", el_Puerto);

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	num_pag = DigitosNombreArchivo(buffer, &posActual);
	printf("Número pagina:%s\n", num_pag);

	//Agrego el proceso a la cpu correspondiente
	t_cpu* la_cpu =buscarCPUporPuerto(el_Puerto);
	//Busco la CPU por el puerto
	la_cpu->procesoActivo=CharAToInt(pid);
	//Le agrego el proceso activo correspondiente.

	accesosTotal++; //Para el calculo de la tasa de aciertos de la TLB

	entrada_tlb * entradaTLB;
	//PRIMERO BUSCA EN TLB
	if(strcmp(g_Tlb_Habilitada,"SI")==0){
		////printf("Buscar:ID:%d, PAG:%d\n",CharAToInt(pid), CharAToInt(num_pag));
	//      printf("Antes TLB:     \n");
	//      mostrarTLB();
		entradaTLB = buscarEnTLB(CharAToInt(pid), CharAToInt(num_pag));
	}else{
		//Si la TLB esta deshabilitada lo pongo en NULL para que vaya directo a memoria
		entradaTLB=NULL;
	}

	if(entradaTLB!=NULL){
		TLBhits++;
		//mostrarTLB()
		printf("TLB HIT!!!!\n");
		char * content = malloc(g_Tam_Marcos);

		content = leerEnMP(entradaTLB->frame);

		//Coseguimos la entrada de la tabla de procesos:
		entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));

		if((strcmp(g_Algoritmo_Reemplazo,"LRU"))==0){
			sacarMarcoProceso(proc->framesAsignados,entradaTLB->frame);
		}

		//printf("Frame: %d, ID: %d,Nro Pag:%d\n",entradaTLB->frame,CharAToInt(pid),CharAToInt(num_pag));
		//mostrarTLB();
		t_marcoProceso* frameProc = buscarMarcoProceso(proc->framesAsignados,entradaTLB->frame);
		frameProc->uso=1;
		printf("\n LEYENDO DE MP CONTENIIDO %s\n", content);
		leerCpu(la_cpu->ip,la_cpu->puerto, num_pag, content);
	}
else{
	//SI NO ENCUENTRA AHI....
	//Coseguimos la entrada de la tabla de procesos:
	entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
	//Conseguimos la entrada de la tabla de paginas:
	entrada_tablaPags * entradaTablaPag = buscarPagina(proc, CharAToInt(num_pag));
	 usleep(g_Retardo_Memoria); //Retardo Busqueda de pagina TODO

	if(entradaTablaPag->presenteEnMemoria==1){
		char * content = malloc(g_Tam_Marcos);
		content = leerEnMP(entradaTablaPag->frame);

		if((strcmp(g_Algoritmo_Reemplazo,"LRU"))==0){
			sacarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);
		}

		t_marcoProceso* frameProc = buscarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);
		frameProc->uso=1;

		printf("\n LEYENDO DE MP CONTENIIDO %s\n", content);

		if(strcmp(g_Tlb_Habilitada,"SI")==0){
			//Reemplazo en TLB

				entradaTLB = sacarDeTLB();
				entradaTLB->frame=entradaTablaPag->frame;
				entradaTLB->pid=CharAToInt(pid);
				entradaTLB->pagina=CharAToInt(num_pag);
				list_add(TLB,entradaTLB);
				//printf("Tamaño TLB:%d\n",list_size(TLB));
				//mostrarTLB()

		}
		leerCpu(la_cpu->ip,la_cpu->puerto, num_pag, content);
	}
	else{
		printf("LEYENDO DE SWAAAAP \n");
		proc->falloPag++;
		leerSwap(CharAToInt(pid), CharAToInt(num_pag));
	}


}
//	//Imprimir estado AAAA
//	entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
//		t_marcoProceso* el_marco;
//		int prueba, aux=0;
//		prueba = list_size(proc->framesAsignados);
//		while(prueba>aux){
//			el_marco=list_get(proc->framesAsignados,aux);
//			aux++;
//			printf("Prueba: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
//		}
//	///de prueba
	//mostrarTabaPaginas(CharAToInt(pid));
	sem_post(&sem_Operacion);
}


void informarEscribir(char* buffer){

	char *el_Puerto, *pid, *num_pag, *contenido;
	int posActual = 2;
	int abortar =0;
	printf("Escribir...\n");
	sem_wait(&sem_Operacion);
	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto CPU:%s\n", el_Puerto);

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	num_pag = DigitosNombreArchivo(buffer, &posActual);
	printf("Número pagina:%s\n", num_pag);

	contenido = DigitosNombreArchivo(buffer, &posActual);
	printf("Contenido:%s\n", contenido);

	//Agrego el proceso a la cpu correspondiente
	t_cpu* la_cpu =buscarCPUporPuerto(el_Puerto);
	//Busco la CPU por el puerto
	la_cpu->procesoActivo=CharAToInt(pid);
	//Le agreguo el proceso activo correspondiente.

	accesosTotal++; //Para el calculo de la tasa de aciertos de la TLB

	entrada_tlb * entradaTLB;
	//PRIMERO BUSCA EN TLB
	if(strcmp(g_Tlb_Habilitada,"SI")==0){

		entradaTLB = buscarEnTLB(CharAToInt(pid), CharAToInt(num_pag));

//		if(entradaTLB!=NULL && entradaTLB->frame!=-1){
//		printf("\nACA ESTA\n");
//		}

	}else{
		//Si la TLB esta deshabilitada lo pongo en NULL para que vaya directo a memoria
		entradaTLB=NULL;
	}
	if(entradaTLB!=NULL){
			TLBhits++;
			//mostrarTLB()
			printf("TLB HIT!!!!\n");
			//Coseguimos la entrada de la tabla de procesos:
			entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
			//Conseguimos la entrada de la tabla de paginas:
			entrada_tablaPags * entradaTablaPag = buscarPagina(proc, CharAToInt(num_pag));


			if((strcmp(g_Algoritmo_Reemplazo,"LRU"))==0){
				sacarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);
			}
			grabarEnMemoria(entradaTLB->frame,contenido);
			t_marcoProceso* frameProc = buscarMarcoProceso(proc->framesAsignados,entradaTLB->frame);
			frameProc->modificado=1;
			frameProc->uso=1;

			//printf("\n GRABANDO EN MP CONTENIIDO %s\n", contenido);

			char* resultado = string_new();
			//string_append(&resultado,"1"); //TODO OJO QUE ESTA HARDCODEADO!!
			string_append(&resultado,obtenerSubBuffer(num_pag));
			string_append(&resultado,obtenerSubBuffer(contenido));

			printf("\n ENVIANDO A CPU: %s\n", resultado);
			escribirCpu(la_cpu->ip,la_cpu->puerto, resultado);

		}
	else{

	//SI NO ENCUENTRA AHI....
	//Coseguimos la entrada de la tabla de procesos:
	entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
	//Conseguimos la entrada de la tabla de paginas:
	entrada_tablaPags * entradaTablaPag = buscarPagina(proc, CharAToInt(num_pag));
	 usleep(g_Retardo_Memoria); //Retardo Busqueda de pagina TODO

	if(entradaTablaPag->presenteEnMemoria==1){

	if((strcmp(g_Algoritmo_Reemplazo,"LRU"))==0){
		sacarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);
	}

	grabarEnMemoria(entradaTablaPag->frame,contenido);
	t_marcoProceso* frameProc = buscarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);
	frameProc->modificado=1;
	frameProc->uso=1;

	printf("\n GRABANDO EN MP CONTENIIDO %s\n", contenido);

	char* resultado = string_new();
	//string_append(&resultado,"1"); //TODO OJO QUE ESTA HARDCODEADO!!
	string_append(&resultado,obtenerSubBuffer(num_pag));
	string_append(&resultado,obtenerSubBuffer(contenido));

	if(strcmp(g_Tlb_Habilitada,"SI")==0){
		//Reemplazo en TLB

			entrada_tlb* entradaTLB = sacarDeTLB();
			entradaTLB->frame=entradaTablaPag->frame;
			entradaTLB->pid=CharAToInt(pid);
			entradaTLB->pagina=CharAToInt(num_pag);
			list_add(TLB,entradaTLB);

			//mostrarTLB()
	}

	printf("\n ENVIANDO A CPU: %s\n", resultado);
	escribirCpu(la_cpu->ip,la_cpu->puerto, resultado);

	}else{

	//Cuando escribo una pagina que no estaba cargada tengo que traerla de swap
	// y solo escribo en Swap cuando la reemplazoy fue modificada
	char* contenido2 = leerSwapEscribir(CharAToInt(pid),CharAToInt(num_pag)); //esto no hace nada mas que contemplar el retardo swap
	//escribirSwap(CharAToInt(pid),CharAToInt(num_pag),contenido);

	//CARGO PAGINA EN MP
		entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
	//Conseguimos la entrada de la tabla de paginas:
		entrada_tablaPags * entradaTablaPag = buscarPagina(proc, CharAToInt(num_pag));

		proc->falloPag++;

		printf("\nLA LISTA TIENE %d MARCOS\n",list_size(proc->framesAsignados));
		if(list_size(proc->framesAsignados)<g_Max_Marcos_Proc && buscarFrameLibre()!=NULL){

			printf("\nCargando Pagina a MP... \n");
			t_frame * marcoLibre;
			marcoLibre = buscarFrameLibre();


			list_add(proc->framesAsignados,marcoProceso_create(marcoLibre->frameNro));

			printf("\n PASE AGREGAR LISTA\n");

			//ASIGNAMOS ESE FRAME AL PROCESO.
			grabarEnMemoria(marcoLibre->frameNro, contenido);

			printf("\n PASE GRABAR MEMORIA\n");

			marcoLibre->pid=CharAToInt(pid);
			marcoLibre->pagina = CharAToInt(num_pag);

			entradaTablaPag->frame=marcoLibre->frameNro;
			marcoLibre->usado=1;
			entradaTablaPag->presenteEnMemoria=1;


			t_marcoProceso* frameProc = buscarMarcoProceso(proc->framesAsignados,entradaTablaPag->frame);

			printf("\n PASE BUSCAR MARCO PROCESO\n");

			frameProc->modificado=1;
			frameProc->uso=1;

			if(strcmp(g_Tlb_Habilitada,"SI")==0){
				//Reemplazo en TLB

					entrada_tlb* entradaTLB = sacarDeTLB();
					entradaTLB->frame=entradaTablaPag->frame;
					entradaTLB->pid=CharAToInt(pid);
					entradaTLB->pagina=CharAToInt(num_pag);
					list_add(TLB,entradaTLB);

					//mostrarTLB()
			}

			printf("\nel proceso %d, pagina %d, CONTENIDO CARGADO %s \n", marcoLibre->pid,marcoLibre->pagina, contenido);

		} else if (list_size(proc->framesAsignados)!=0) {
			//Si los marcos asignados al proceso estan llenos o la memoria no tiene frames correr algoritmo de remplazo
			correrAlgoritmo(proc,entradaTablaPag, contenido, 2);
		} else {
			printf(COLOR_CYAN"******ABORTAR PROCESO POR FALTA DE MARCOS******"DEFAULT"\n");
			//sleep(10);
			AbortarProceso(el_Puerto, pid);
			abortar =1;
		}
if(abortar ==0){
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));
	char* resultado = string_new();
	//string_append(&resultado,"1"); //TODO OJO QUE ESTA HARDCODEADO!!
	string_append(&resultado,obtenerSubBuffer(num_pag));
	string_append(&resultado,obtenerSubBuffer(contenido));
	escribirCpu(la_cpu->ip,la_cpu->puerto,resultado);
}
	}

	}
	if(abortar ==0){
	mostrarTabaPaginas(CharAToInt(pid));}

	sem_post(&sem_Operacion);
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

char* obtenerSubBuffer(char *nombre) {
			// Esta funcion recibe un nombre y devuelve ese nombre de acuerdo al protocolo. Ej: carlos ------> 16carlos
			char *aux = string_new();
			int tamanioNombre = 0;
			float tam = 0;
			int cont = 0;

			tamanioNombre = strlen(nombre);
			tam = tamanioNombre;
			while (tam >= 1) {
				tam = tam / 10;
				cont++;
			}
			string_append(&aux, string_itoa(cont));
			string_append(&aux, string_itoa(tamanioNombre));
			string_append(&aux, nombre);

			return aux;
}

void resultadoInicioSwap(char* buffer){
	char *resultado, *pid;
	int posActual = 2;

	printf("\nResultado Inicio...\n");

	pid = DigitosNombreArchivo(buffer, &posActual);
		printf("PID:%s\n", pid);
	resultado = (buffer+posActual);
		printf("RESULTADO:%s\n", resultado);
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));

	inicioProcesoCpu(la_cpu->ip,la_cpu->puerto, resultado);
}

void resultadoLecturaSwap(char* buffer){
	char *contenido, *pid, *pagina, *error;
	int posActual = 2;
	int abortar=0;

	printf("\nResultado Lectura...\n");

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("PID:%s\n", pid);
	error = buffer+posActual;
	if(!strcmp(error,"0")){
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));
	leerCpuError(la_cpu->ip,la_cpu->puerto);
	}
	else
	{
	pagina = DigitosNombreArchivo(buffer, &posActual);
	printf("\nPAGINA TRAIDA DE SWAP:%s\n", pagina);
	contenido = DigitosNombreArchivo(buffer, &posActual);
	printf("\nCONTENIDO:%s\n", contenido);

	//CARGO PAGINA EN MP
		entrada_tablaProcesos * proc = buscarPorId(CharAToInt(pid));
	//Conseguimos la entrada de la tabla de paginas:
		entrada_tablaPags * entradaTablaPag = buscarPagina(proc, CharAToInt(pagina));

		if(list_size(proc->framesAsignados)<g_Max_Marcos_Proc && buscarFrameLibre()!=NULL){

			printf("\nCargando Pagina a MP... \n");
			t_frame * marcoLibre;
			marcoLibre = buscarFrameLibre();
					if(marcoLibre==NULL){
						printf("No hay marco libre"); //TODO: ACA FINALIZAR EL PROCESO?
					}
			//Sumo uno a los frames asignados al procesoS
			list_add(proc->framesAsignados,marcoProceso_create(marcoLibre->frameNro));

			//ASIGNAMOS ESE FRAME AL PROCESO.
			grabarEnMemoria(marcoLibre->frameNro, contenido);

			marcoLibre->pid=CharAToInt(pid);
			marcoLibre->pagina = CharAToInt(pagina);

			//printf("LLEGUE ACA ! \n");

			entradaTablaPag->frame=marcoLibre->frameNro;
			marcoLibre->usado=1;
			entradaTablaPag->presenteEnMemoria=1;

			if(strcmp(g_Tlb_Habilitada,"SI")==0){
				//Reemplazo en TLB

					entrada_tlb* entradaTLB = sacarDeTLB();
					entradaTLB->frame=marcoLibre->frameNro;
					entradaTLB->pid=CharAToInt(pid);
					entradaTLB->pagina=CharAToInt(pagina);
					list_add(TLB,entradaTLB);

					//mostrarTLB()
			}
			printf("\nel proceso %d, pagina %d, CONTENIDO CARGADO %s \n", marcoLibre->pid,marcoLibre->pagina, contenido);

		} else if (list_size(proc->framesAsignados)!=0) {
			//Si los marcos asignados al proceso estan llenos o la memoria no tiene frames correr algoritmo de remplazo
			correrAlgoritmo(proc,entradaTablaPag, contenido, 1);
		} else {
			printf(COLOR_MAGENTA"ABORTAR PROCESO"DEFAULT"\n");
			//sleep(10);
			t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));
			AbortarProceso(la_cpu->puerto, pid);
			abortar =1;
		}
if(abortar ==0){
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));
	leerCpu(la_cpu->ip,la_cpu->puerto,pagina, contenido);
}
	}

	if(abortar ==0){
		mostrarTabaPaginas(CharAToInt(pid));}

}

void resultadoEscrituraSwap(char* buffer){
	char *resultado, *pid;
	int posActual = 2;

	printf("\nResultado Escritura...\n");

	pid = DigitosNombreArchivo(buffer, &posActual);
		printf("PID:%s\n", pid);
	resultado = (buffer+posActual);
		printf("RESULTADO:%s\n", resultado);
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	//t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));

	//escribirCpu(la_cpu->ip,la_cpu->puerto, resultado);

	//Cuando escribe swap no retrona a cpu

}

void resultadoFinSwap(char* buffer){
	char *resultado, *pid;
	int posActual = 2;

	printf("\nResultado Fin....\n");

	pid = DigitosNombreArchivo(buffer, &posActual);
		printf("PID:%s\n", pid);
	resultado = (buffer+posActual);
		printf("RESULTADO:%s\n", resultado);
	//Busco la CPU en la lista donde se esta ejecutando el proceso.
	t_cpu* la_cpu = buscarCPUporPid(CharAToInt(pid));

	finProcesoCpu(la_cpu->ip,la_cpu->puerto);
}

int finProcesoCpu(char*ip, char*puerto){

		int socket_cpu;
		if(!conectarConCpu(&socket_cpu, ip, puerto)){
			//Error conexion
			return 0;
		}
		//39+pid
		char* buffer = string_new();
		string_append(&buffer,"39");
		EnviarDatos(socket_cpu, buffer,strlen(buffer));
		printf("Envie el msj correcto: %s \n",buffer);
		free(buffer);
		return 1;
}

int inicioProcesoCpu(char*ip, char*puerto,char* resultado){

	int socket_cpu;
	if(!conectarConCpu(&socket_cpu, ip, puerto)){
		//Error conexion
		return 0;
	}
	//35+pid
	char* buffer = string_new();
	string_append(&buffer,"37");
	string_append(&buffer,resultado);
	EnviarDatos(socket_cpu, buffer,strlen(buffer));
	return 1;

}

int escribirCpu(char*ip, char*puerto,char* resultado){


	int socket_cpu;
	if(!conectarConCpu(&socket_cpu, ip, puerto)){
		//Error conexion
		return 0;
	}
	//35+pid
	char* buffer = string_new();
	string_append(&buffer,"36");
	string_append(&buffer,resultado);
	EnviarDatos(socket_cpu, buffer,strlen(buffer));
	return 1;

}

int leerCpu(char*ip, char*puerto,char*pagina,char* contenido){


	int socket_cpu;
	if(!conectarConCpu(&socket_cpu, ip, puerto)){
		//Error conexion
		return 0;
	}
	//35+pid
	char* buffer = string_new();
	string_append(&buffer,"38");
	string_append(&buffer,obtenerSubBuffer(pagina));
	string_append(&buffer,obtenerSubBuffer(contenido));
	EnviarDatos(socket_cpu, buffer,strlen(buffer));
	free(contenido);
	return 1;

}

int leerCpuError(char*ip, char*puerto){


	int socket_cpu;
	if(!conectarConCpu(&socket_cpu, ip, puerto)){
		//Error conexion
		return 0;
	}
	//35+pid
	char* buffer = string_new();
	string_append(&buffer,"35");
	string_append(&buffer,0);
	EnviarDatos(socket_cpu, buffer,strlen(buffer));
	return 1;

}

int conectarConCpu(int *socket_cpu, char*ip, char*puerto){
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


		if (getaddrinfo(ip, puerto, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
			log_info(logger,
					"ERROR: cargando datos de conexion socket_cpu");
		}

		if ((*socket_cpu = socket(serverInfo->ai_family, serverInfo->ai_socktype,
				serverInfo->ai_protocol)) < 0) {
			log_info(logger, "ERROR: crear socket_cpu");
		}
		if (connect(*socket_cpu, serverInfo->ai_addr, serverInfo->ai_addrlen)
				< 0) {
			log_info(logger, "ERROR: No se pudo conectar CPU");
		} else {
			conexionOk = 1;
		}
		freeaddrinfo(serverInfo);	// No lo necesitamos mas
		return conexionOk;
}

t_cpu* buscarCPUporPid(int pid) {
	t_cpu* la_cpu = malloc(sizeof(t_cpu));
	bool _true(void *elem) {
		return (((t_cpu*) elem)->procesoActivo == pid);
	}
	la_cpu = list_find(lista_cpu, _true);
	return la_cpu;
}

t_cpu* buscarCPUporPuerto(char* puerto) {
	t_cpu* la_cpu = malloc(sizeof(t_cpu));
	bool _true(void *elem) {
		return (!strcmp(((t_cpu*) elem)->puerto, puerto));
	}
	la_cpu = list_find(lista_cpu, _true);
	return la_cpu;
}




void inicializarListaMarcos(t_list * marcos, int cant_marcos){
	int i;
	for (i=0; i<cant_marcos; i++){
	list_add(marcos, frame_create(g_Tam_Marcos,i));
	}
	}

entrada_tablaProcesos * buscarPorId(int id){
	bool _true(void *elem) {
			return (((entrada_tablaProcesos*) elem)->pid == id);
		}
	entrada_tablaProcesos * proceso;
	proceso = list_find(lista_procesos, _true);

	return proceso;
	}

entrada_tablaPags * buscarPagina(entrada_tablaProcesos * proc, int numPag){
	bool _true(void *elem) {
			return (((entrada_tablaPags*) elem)->pagN == numPag);
		}
	entrada_tablaPags * entradaTablaPag;
	entradaTablaPag = list_find(proc->tablaPags, _true);

	return entradaTablaPag;
	}

entrada_tablaPags * buscarPaginaPorMarco(entrada_tablaProcesos * proc, int marco){
	bool _true(void *elem) {
			return (((entrada_tablaPags*) elem)->frame == marco);
		}
	entrada_tablaPags * entradaTablaPag;
	entradaTablaPag = list_find(proc->tablaPags, _true);

	return entradaTablaPag;
	}


t_frame * buscarFrameLibre(){
	t_frame* marcoLibre = malloc(sizeof(t_frame));
	bool _true(void *elem) {
				return (((t_frame*) elem)->usado == 0);
			}

	//printf("\n LALALAALALALALLALALLALAL\n");

	//if(list_find(marcos, _true)!= NULL){
	marcoLibre = list_find(marcos, _true);

//	printf("\nFRAME %d, %d, %d, %d\n", marcoLibre->frameNro, marcoLibre->pagina, marcoLibre->pid, marcoLibre->usado);
//	}
//	else{
//	marcoLibre->frameNro=-2;
//	marcoLibre->pagina=-2;
//	marcoLibre->pid=-2;
//	marcoLibre->usado=-2;
//	printf("\nFRAMETRUCHO %d, %d, %d, %d\n", marcoLibre->frameNro, marcoLibre->pagina, marcoLibre->pid, marcoLibre->usado);
//	}
	return marcoLibre;
}

void sacarMarcoProceso(t_list* listaFrames, int nroFrame){
	t_marcoProceso* marcoLibre = malloc(sizeof(t_marcoProceso));
	bool _true(void *elem) {
				return (((t_marcoProceso*) elem)->frameNro == nroFrame);
			}
		marcoLibre = list_remove_by_condition(listaFrames, _true);
		list_add(listaFrames,marcoLibre);
		//return marcoLibre;
}

t_frame * buscarFramePorNumero(int nroFrame){
	t_frame* marcoLibre = malloc(sizeof(t_frame));
	bool _true(void *elem) {
				return (((t_frame*) elem)->frameNro == nroFrame);
			}
		marcoLibre = list_find(marcos, _true);
		return marcoLibre;
}

t_marcoProceso * buscarMarcoProceso(t_list* listaFrames, int nroFrame){
	t_marcoProceso* marcoLibre = malloc(sizeof(t_marcoProceso));
	bool _true(void *elem) {
				return (((t_marcoProceso*) elem)->frameNro == nroFrame);
			}
		marcoLibre = list_find(listaFrames, _true);
		return marcoLibre;
}

char* leerEnMP(int nroMarco) {
 char * memoria = memoriaPrincipal;
 int i = 0;
 int contador = g_Tam_Marcos;
 char* aux = malloc(g_Tam_Marcos + 1);
 memset(aux, '0', g_Tam_Marcos * sizeof(char)); //Setea el aux a 0

 memoria = memoria + ((nroMarco * g_Tam_Marcos));
 while (contador--) {
  aux[i] = memoria[i];
  i++;

 }
 aux[i] = '\0';

 //memcpy(buffer, aux, g_Tam_Marcos); //Copia el aux en buffer
 //printf("\nLA LECTURA FUE: %s\n",aux);
 usleep(g_Retardo_Memoria);
 return aux;
}

int grabarEnMemoria(int nroMarco, char * texto) {
 char* aux = malloc(g_Tam_Marcos);
 int i = 0;
 int cont = g_Tam_Marcos;
 memset(aux, '0', g_Tam_Marcos); //Setea el aux a 0

 memcpy(aux, texto, g_Tam_Marcos); //Copia el texto en el aux

 char * memoria = memoriaPrincipal;
 memoria = memoria + (nroMarco * g_Tam_Marcos);
 while (cont--) {
	 memoria[i]= aux[i];
	 i++;
	 printf("%c",*(memoria-1));
 }
 usleep(g_Retardo_Memoria);
 return 1;
}

void correrAlgoritmo(entrada_tablaProcesos* proceso, entrada_tablaPags* tPaginas, char* contenido, int operacion){
	printf("\nESTOY CORRIENDO EL ALGORITMO DE REEMPLAZO!!\n");
	//operacion es para saber si vino de lectura o escritura, 1 es lectura y 2 escritura para los bits de uso y modificacion de Clock modificado
if((strcmp(g_Algoritmo_Reemplazo,"FIFO"))==0 || (strcmp(g_Algoritmo_Reemplazo,"LRU"))==0){
		//FIFO y LRU se manejan igual salvo cuando leen o escriben una pagina que ya estaba cargada.
		t_marcoProceso* el_marco =list_remove(proceso->framesAsignados,0);
		printf("\nLA marco %d, uso %d, modif: %d\n",el_marco->frameNro, el_marco->uso,el_marco->modificado);

		entrada_tablaPags * entrTP= buscarPaginaPorMarco(proceso, el_marco->frameNro);
		printf("\nLA PAGINA %d, FRAME %d, PReSENTE EN MP: %d\n",entrTP->pagN, entrTP->frame,entrTP->presenteEnMemoria);

		if(el_marco->modificado==1){
			//Si el marco fue modificado cargarlo devuelta a swap
			char * lecMP= leerEnMP(el_marco->frameNro);
			escribirSwapReemplazo(proceso->pid,entrTP->pagN,lecMP);//TODO: Que escribirSwapReemplazo devuelva 1 para que salga t0do bien
		}
			//
			if(strcmp(g_Tlb_Habilitada,"SI")==0){
			//Reemplazo en TLB
				entrada_tlb* entradaTLB = buscarEnTLB(proceso->pid, entrTP->pagN);
				if(entradaTLB!=NULL){

					eliminoEnTLB(proceso->pid, entrTP->pagN);
					entrada_tlb* entradaTLB = malloc(sizeof(entrada_tlb));
					entradaTLB->frame=el_marco->frameNro;
					entradaTLB->pid=proceso->pid;
					entradaTLB->pagina=tPaginas->pagN;
					list_add(TLB,entradaTLB);

					//mostrarTLB()
				}

			}
			grabarEnMemoria(el_marco->frameNro, contenido);
			t_frame * marcoModif;
			marcoModif = buscarFramePorNumero(el_marco->frameNro);
			marcoModif->pagina=tPaginas->pagN;

			tPaginas->presenteEnMemoria=1;
			tPaginas->frame=el_marco->frameNro;
			entrTP->presenteEnMemoria=0;
			entrTP->frame=-1;

	//		if(strcmp(g_Tlb_Habilitada,"SI")==0){
	//			//Reemplazo en TLB
	//				entrada_tlb* entradaTLB = sacarDeTLB();
	//				entradaTLB->frame=el_marco->frameNro;
	//				entradaTLB->pid=proceso->pid;
	//				entradaTLB->pagina=tPaginas->pagN;
	//				list_add(TLB,entradaTLB);
	//		}

		list_add(proceso->framesAsignados,el_marco);
		//Vuelvo a agregarlo al final de la lista

	}

//Algoritmo clock modificado
else if((strcmp(g_Algoritmo_Reemplazo,"CLOCK-M"))==0){
		t_marcoProceso* el_marco;
		int primerCheck=0, segundoCheck=0, tercerCheck=0;
		int punteroAux = punteroClock;

		//Para saber cuando ya recorrí todos los marcos.

		//veo como estan los punteros:
//		int prueba, aux=0;
//		prueba = list_size(proceso->framesAsignados);
//		printf("Ptro esta en: %d\n",punteroClock);
//	while(prueba>aux){
//			el_marco=list_get(proceso->framesAsignados,aux);
//			aux++;
//			printf("Antes: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
//		}

	//Primer chequeo buscando Uso=0 y Modificacion=0
	while(primerCheck==0){
		el_marco=list_get(proceso->framesAsignados,punteroClock);
		punteroClock++;
		//printf("1er: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
		if(punteroClock==list_size(proceso->framesAsignados)){
		punteroClock=0;  //El puntero va de 0 a CantMarcos-1 , si es igual a list size hay que reiniciarlo a 0
		}

		if((el_marco->modificado==0 && el_marco->uso==0)){
			primerCheck=1; //1 encontro el marco para cambiar
		}

		if(punteroAux==punteroClock){
			primerCheck=2; //2 salio por haber recorrido todos
		}
	}

	//Segundo chequeo buscando Uso=0 y Modificacion=1 cambiando el bit de uso a medida que avanzo
	while(primerCheck==2 && segundoCheck == 0){
		el_marco=list_get(proceso->framesAsignados,punteroClock);
		punteroClock++;
		//printf("2do: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
		if(punteroClock==list_size(proceso->framesAsignados)){
		punteroClock=0;  //El puntero va de 0 a CantMarcos-1 , si es igual a list size hay que reiniciarlo a 0
		}

		if((el_marco->modificado==1 && el_marco->uso==0)){
			segundoCheck=1; //1 encontro el marco para cambiar
		}else{
			el_marco->uso=0;
		}

		if(punteroAux==punteroClock){
			segundoCheck=2; //2 salio por haber recorrido todos
		}

	}

	//Tercer chequeo igual al primero
	while(primerCheck==2 && segundoCheck == 2 && tercerCheck==0){
		el_marco=list_get(proceso->framesAsignados,punteroClock);
		punteroClock++;
		//printf("3ro: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
		if(punteroClock==list_size(proceso->framesAsignados)){
		punteroClock=0;  //El puntero va de 0 a CantMarcos-1 , si es igual a list size hay que reiniciarlo a 0
		}

		if((el_marco->modificado==0 && el_marco->uso==0)){
			tercerCheck=1; //1 encontro el marco para cambiar
		}

		if(punteroAux==punteroClock){
			tercerCheck=2; //2 salio por haber recorrido todos
		}
	}
	if(tercerCheck==2){
		el_marco=list_get(proceso->framesAsignados,0);
		punteroClock=1;
	} //Si recorri todos los marcos agarro el 1ro y no el ultimo.

	entrada_tablaPags * entrTP= buscarPaginaPorMarco(proceso, el_marco->frameNro);

	if(el_marco->modificado==1){
		//Si el marco fue modificado cargarlo devuelta a swap
		escribirSwapReemplazo(proceso->pid,entrTP->pagN,leerEnMP(el_marco->frameNro));
	}

	grabarEnMemoria(el_marco->frameNro, contenido);
	t_frame * marcoModif;
	marcoModif = buscarFramePorNumero(el_marco->frameNro);
	marcoModif->pagina=tPaginas->pagN;
	//El puntero queda en la proxima instruccion.
	//punteroClock++;
	if(punteroClock==list_size(proceso->framesAsignados)){
		punteroClock=0;
	}

	tPaginas->presenteEnMemoria=1;
	tPaginas->frame=el_marco->frameNro;
	entrTP->presenteEnMemoria=0;
	entrTP->frame=-1;

	if(operacion==1){ //es LECTURA
	el_marco->uso=1;
	el_marco->modificado=0;
	}
	else if(operacion==2){ //es ESCRITURA
	el_marco->uso=1;
	el_marco->modificado=1;
	}

	if(strcmp(g_Tlb_Habilitada,"SI")==0){
		//Reemplazo en TLB
				//printf("Clock-> TLB ANTES:");
				//mostrarTLB();
			entrada_tlb* entradaTLB = buscarEnTLB(proceso->pid, entrTP->pagN);
			if(entradaTLB!=NULL){
				eliminoEnTLB(proceso->pid, entrTP->pagN);
				//printf("Clock-> Elimine PID: %d, Pag: %d",proceso->pid, entrTP->pagN);
				//mostrarTLB();
				entrada_tlb* entradaTLB = malloc(sizeof(entrada_tlb));
				entradaTLB->frame=el_marco->frameNro;
				entradaTLB->pid=proceso->pid;
				entradaTLB->pagina=tPaginas->pagN;
				list_add(TLB,entradaTLB);
			//	printf("Tamaño TLB:%d\n",list_size(TLB));
				//printf("Clock-> TLB despues:");
				mostrarTLB();
			}

//	aux=0;
//	printf("Ptro quedo en: %d\n",punteroClock);
//	while(prueba>aux){
//		el_marco=list_get(proceso->framesAsignados,aux);
//		aux++;
//		printf("DESPUES: Marco: %d || Mod: %d || Uso: %d \n",el_marco->frameNro,el_marco->modificado,el_marco->uso);
//	}




		}

//	if(strcmp(g_Tlb_Habilitada,"SI")==0){
//		//Reemplazo en TLB
//			entrada_tlb* entradaTLB = sacarDeTLB();
//			entradaTLB->frame=el_marco->frameNro;
//			entradaTLB->pid=proceso->pid;
//			entradaTLB->pagina=tPaginas->pagN;
//			list_add(TLB,entradaTLB);
//			printf("Tamaño TLB:%d\n",list_size(TLB));
//	}


	}
}

void mostrarTabaPaginas(int pid){

	printf("------------------------------------------------------\nTabla de paginas: \n");

	int index=0;
	entrada_tablaProcesos *proc = malloc(sizeof(entrada_tablaProcesos));

	proc = buscarPorId(pid);

	entrada_tablaPags * entrada = list_get(proc->tablaPags, index);
	while(entrada!=NULL){
		printf("Frame: %d  //   Pagina: %d    //   Presente: %d\n", entrada->frame, entrada->pagN, entrada->presenteEnMemoria);
		index++;
		entrada = list_get(proc->tablaPags, index);
	}

	//free(entrada);
	//free(proc);
}

void mostrarTLB(){

	printf("------------------------------------------------------\nTLB.....: \n");

	int index=0;
	entrada_tlb *entrada = malloc(sizeof(entrada_tlb));

	entrada = list_get(TLB, index);
	while(entrada!=NULL){
		printf("Frame: %d  //   Pagina: %d    //   PID: %d\n",
				entrada->frame,entrada->pagina,entrada->pid);
		index++;
		entrada = list_get(TLB, index);
	}

	//free(entrada);
	//free(proc);
}

void crearTLB(int g_Entradas_Tlb){
	int i;
	for (i=0; i<g_Entradas_Tlb; i++){
		list_add(TLB, entrada_tlb_create());
		}
}

entrada_tlb * buscarEnTLB(int id, int pagina){
	bool _true(void *elem) {
			return (((entrada_tlb*) elem)->pid == id && ((entrada_tlb*) elem)->pagina == pagina);
		}
	entrada_tlb * entrada;
	entrada = list_find(TLB, _true);
	return entrada;
	}

void eliminoEnTLB(int id, int pagina){
	bool _true(void *elem) {
			return (((entrada_tlb*) elem)->pid == id && ((entrada_tlb*) elem)->pagina == pagina);
		}
	list_remove_by_condition(TLB, _true);
	}


entrada_tlb * sacarDeTLB(){
	entrada_tlb * entrada;
	entrada = list_remove(TLB, 0);
	return entrada;
	}


void * SENIAL(){
	signal(SIGUSR1, AtenderSenial);
	signal(SIGUSR2, AtenderSenial);
	signal(SIGPOLL, AtenderSenial);
	return NULL;
}

void AtenderSenial(int s){
	switch(s){

	case SIGUSR1:
	{
		//TLB FLUSH, hilo correctamente sincronizado

		//printf("*****ANTES TLB*****\n");
		//mostrarTLB();
		log_trace(logger,"SEÑAL RECIBIDA: SIGUSR1 - Corriendo TLB Flush");
		sem_wait(&sem_Operacion);
		sem_wait(&sPrueba1);
		pthread_t senial;
		pthread_create(&senial, NULL, (void*)tlbFlush, NULL);
		pthread_join(senial,NULL);
		sem_post(&sem_Operacion);
		sem_post(&sPrueba1);
		log_trace(logger,"SEÑAL RECIBIDA: SIGUSR1 - Finalizando TLB Flush");
		//printf("*****DESPUES TLB*****\n");
		//mostrarTLB();
	}
	break;

	case SIGUSR2:
	{
		//Limpiar completamente la memoria
		//actualizando los bits que sean necesarios en las tablas de páginas
		//de los diferentes procesos con un hilo correctamente sincronizado
		//printf("Arranco señal de borrado\n");
		log_trace(logger,"SEÑAL RECIBIDA: SIGUSR2 - Corriendo Limpieza Completa de la memoria");
		pthread_t senial;
		pthread_create(&senial, NULL, (void*)limpiarMemoria, NULL);
		pthread_join(senial,NULL);
		log_trace(logger,"SEÑAL RECIBIDA: SIGUSR2 - Finalizo Limpieza de la memoria");
	}
	break;

	case SIGPOLL:
	{
		//DUMP del contenido de la memoria principal indicando marco proceso y contenido.
		log_trace(logger,"SEÑAL RECIBIDA: SIGPOLL - Corriendo Dump del contendio de la memoria");
		printf(COLOR_VERDE"SEÑAL: CORRIENDO DUMP DE MEMORIA"DEFAULT"\n");
		pid_t pid = fork();

		   if (pid == -1) {
		      perror("fork failed");
		      exit(EXIT_FAILURE);
		   }
		   else if (pid == 0) {
		      int i;
		      for (i=0; i<g_Cant_Marcos; i++){
		    	char*contenido = malloc(g_Tam_Marcos);
		    	contenido=leerEnMP(i);
		    	log_trace(logger, "MARCO: %d CONTENIDO: %s", i,contenido);
		      }
		      _exit(EXIT_SUCCESS);
		     }
		   printf(COLOR_VERDE"SEÑAL: FIN DE DUMP DE MEMORIA"DEFAULT"\n");
		   log_trace(logger,"SEÑAL RECIBIDA: SIGPOLL - Finalizando Dump del contendio de la memoria");
		   }
	break;
	}

}

void limpiarMemoria(void * arg) {
	int i=0,k=0,l=0,cant_pag=0;
	//int ,cant_frame=0;
	//PASO A SWAP:
	char *lecMP;
	sem_wait(&sem_Operacion);
	sem_wait(&sPrueba1);
	printf(COLOR_VERDE"SEÑAL: EMPIEZA LIMPIEZA DE MEMORIA"DEFAULT"\n");
	while(list_size(lista_procesos)>k){

		entrada_tablaProcesos* unProceso = list_get(lista_procesos, k);
		//printf("Tamaño tabla pags%d\n",list_size(unProceso->tablaPags));
		//printf("Cantidad de framesasignados:%d||cant tabla pag:%d\n",list_size(unProceso->framesAsignados),list_size(unProceso->tablaPags));

			while(list_size(unProceso->tablaPags)>i){
				entrada_tablaPags * entrada = list_get(unProceso->tablaPags,i);
				//printf("Entradaframe:%d, pagn%d\n:",entrada->frame,entrada->pagN);
				if(entrada->presenteEnMemoria==1){
					lecMP= leerEnMP(entrada->frame);
					printf("Grabar en swap:PID:%d|| PagN:%d||Contenido:%s\n",unProceso->pid,entrada->pagN,lecMP);
					escribirSwapReemplazo(unProceso->pid,entrada->pagN,lecMP);//TODO: Que escribirSwapReemplazo devuelva 1 para que salga t0do bien
				}
				i++;
			};
		k++;
	}

	//BORRO MEMORIA:
	i=0;
	k=0;
	l=0;
	printf("MP ANTES %s\n",memoriaPrincipal);
	bzero(memoriaPrincipal,g_Cant_Marcos*g_Tam_Marcos*sizeof(char));
	printf("MP BORRADA:%s\n",memoriaPrincipal);
	//Pone toda le memoria en /0
	printf("Cant proc:%d\n",list_size(lista_procesos));
	if(list_size(lista_procesos)!=0){
	while(list_size(lista_procesos)>k){
		printf("Ingreso a borrar tabla de pagina y marcos\n");
		//Limpio las paginas y los marcos de cada proceso.

		entrada_tablaProcesos* unProceso = list_get(lista_procesos, k);
		//printf("Tabla Pag:%d||Cant:%d\n",list_size(unProceso->tablaPags),j);
		cant_pag=list_size(unProceso->tablaPags);
		//cant_frame=list_size(unProceso->framesAsignados);
		//printf("ProcesoID:%d||cantpag:%d||CantFrame:%d\n",k,cant_pag,cant_frame);
		printf("Borro tabla de paginas y frames del proceso:%d\n",unProceso->pid);
		while(list_size(unProceso->tablaPags)!=0){
			//printf("Dentro del 1er while\n");
			entrada_tablaPags * entrada = list_remove(unProceso->tablaPags,0);
			entradaTablaPags_destroy(entrada);
			};

		while(list_size(unProceso->framesAsignados)!=0){
			//printf("Dentro del 2do while\n");
			t_marcoProceso * marcoProc = list_remove(unProceso->framesAsignados,0);
			//Libero los marcos
			t_frame* marcoDisponible = buscarFramePorNumero(marcoProc->frameNro);
			marcoDisponible->usado=0;
			marcoProceso_destroy(marcoProc);
			};
			//	printf("Creo tabla pag\n");
				unProceso->tablaPags = list_create();
				for (i = 0; i < cant_pag; i++){
				//printf("Dentro del fore\n");
					entrada_tablaPags * entrada = entradaTablaPags_create(i);
					list_add(unProceso->tablaPags, entrada);
					};

				k++;

		}

	if(strcmp(g_Tlb_Habilitada,"SI")==0){
	printf("BORRO TLB:\n");
	l=0;
	while(list_size(TLB)>l){
		//Elimino los frames asignados
		entrada_tlb * entradaTLB = list_get(TLB,l);
		entradaTLB->frame=-1;
		entradaTLB->pagina=-1;
		entradaTLB->pid=-1;
		l++;
		};
	//mostrarTLB();
	};
	};
	printf(COLOR_VERDE"SEÑAL: FINALIZO LIMPIEZA DE MEMORIA"DEFAULT"\n");
	sem_post(&sem_Operacion);
	sem_post(&sPrueba1);
}

void tlbFlush(void * arg) {

	int i=0;

	printf(COLOR_VERDE"SEÑAL: EMPIEZO A LIMPIAR TLB"DEFAULT"\n");
	while(list_size(TLB)!=i){
		//Elimino los frames asignados
		entrada_tlb * entradaTLB = list_get(TLB,i);
		entradaTLB->frame=-1;
		entradaTLB->pagina=-1;
		entradaTLB->pid=-1;
		i++;
		};
	printf(COLOR_VERDE"SEÑAL: FINALIZO LIMPIEZA TLB"DEFAULT"\n");
	//sem_post(&sPrueba1);
	//sem_post(&sem_Operacion);
}

void calcularTlbHits (void* arg){

while(1){
		sleep(60);
		if(accesosTotal==0){
		printf("LA TASA HISTORICA DE ACIERTOS DE LA TLB FUE: 0 (No hubo accesos a memoria aun)\n");
		}
		else{
		int tasaAciertos= (TLBhits*100)/accesosTotal;
		printf("LA TASA HISTORICA DE ACIERTOS DE LA TLB FUE: %d \n",tasaAciertos);
		}
}

}
