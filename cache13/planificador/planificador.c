#include "planificador.h"

int main(int argv, char** argc) {

	int iThreadConsola, iThreadOrquestador;					//Hilo de consola
	sem_init(&sEntradaSalida,0,1);

	lista_cpu = list_create();		//Lista de cpus
	lista_procesos = list_create();		//Lista de procesos.
	cola_listos = list_create();		//Cola de procesos en estado listo.
	cola_bloqueados = list_create();		//Cola de procesos bloquedados.
	lista_ejecucion = list_create();  		//Lista de procesos en ejecucion
	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true,
			LOG_LEVEL_TRACE);

	// Instanciamos el archivo donde se grabará lo solicitado por consola
	g_ArchivoConsola = fopen(NOMBRE_ARCHIVO_CONSOLA, "wb");
	g_MensajeError = malloc(1 * sizeof(char));

	// Levantamos el archivo de configuracion.
	LevantarConfig();

	//Este hilo es el que maneja la consola
	if ((iThreadConsola = pthread_create(&hConsola, NULL,
			(void*) Comenzar_Consola, NULL)) != 0) {
		fprintf(stderr, (char *) NosePuedeCrearHilo, iThreadConsola);
		exit(EXIT_FAILURE);
	};

	//Hilo orquestador conexiones para escuchar a Cpu
	if ((iThreadOrquestador = pthread_create(&hOrquestadorConexiones, NULL,
			(void*) HiloOrquestadorDeConexiones, NULL)) != 0) {
		fprintf(stderr, (char *) NosePuedeCrearHilo, iThreadOrquestador);
		exit(EXIT_FAILURE);
	};

	pthread_join(hConsola, NULL);
	pthread_join(hOrquestadorConexiones, NULL);

	return EXIT_SUCCESS;

}

void Comenzar_Consola() {

	int corte_consola = -1;

	while (corte_consola != 0) {
		corte_consola = operaciones_consola();//menu de consola para elegir la opcion a realizar en filesystem
	}
	printf("Se termino la ejecucion de la consola del planificador\n");

}

int operaciones_consola() {
	//system("clear");
	//printf("------------Tamaño del Fs:%luMb Tamaño Ocupado:%luMb Tamaño Disponible:%luMb-------------\n",tamanioTotal(),tamanioOcupado(),tamanioDisponible());
	printf("Comandos posibles: correr, finalizar, ps y cpu. 0 para salir.\n");

	char* ingresoConsola = malloc(MAXLINEA);
	fgets(ingresoConsola, MAXLINEA, stdin);
	//system("clear");
	int cont = 0;
	if (!strcmp(ingresoConsola, "\n")) {
		return 1;
	}
	//Le saco el \n final a la linea ingresada
	char **sinBarraN = string_split(ingresoConsola, "\n");
	//Separo el comando y su campo ingresado en caso que tenga.
	char **comando = string_split(sinBarraN[0], " ");

	while (comando[cont] != NULL) {
		cont++;
	}
	if (cont >= 3) {
		printf("Comando invalido.\n");
		return 1;
	}
	printf("Comando ingresado: %s \n", comando[0]);
	if (strcmp(comando[0], "correr") == 0) {
		char* PATH = comando[1];
		printf("Ejecutando comando correr mCod:%s\n", PATH);

		//Creo la pcb del proceso a iniciar
		t_pcb* la_pcb = crearPcbProceso(PATH);
		if (la_pcb == NULL) {
			printf("Error al crear la PCB del proceso.\n");
			return -1;
		} else {
			list_add(lista_procesos, la_pcb); //Agrego el proceso a la lista de procesos en el sistema
		}

		return correrPrograma(la_pcb);

	} else if (strcmp(comando[0], "finalizar") == 0) {
		char* PID = comando[1];
		printf("Ejecutando comando finalizar para el pid %s\n", PID);
		finalizarProceso(CharAToInt(PID));
		// do something else
	} else if (strcmp(comando[0], "ps") == 0) {
		printf("Ejecutando comando ps\n");
		recorrerProcesos();
		// do something else
	} else if (strcmp(comando[0], "cpu") == 0) {
		printf("Ejecutando comando cpu\n");
		recorrerCpu();
		// do something else
	} else if (strcmp(comando[0], "0") == 0) {
		log_info(logger, "Terminando el programa");
		return 0;
	} else /* default: */
	{
		printf("Comando invalido.\n");
		return 1;
	}

	return -1;
}

int correrPrograma(t_pcb* la_pcb) {

	//Busco si hay alguna CPU libre
	t_cpu* la_cpu = buscarCpuLibre();
	if (la_cpu == NULL) {
		//Si las CPU estan todas ocupadas agrego el proceso a la cola de listos.
		printf("No hay CPU disponibles.\n");
		agregarAColaListos(la_pcb->pid);
		return 1;
	} else {
		//Si encuentra una CPU libre inicia el proceso.

		iniciarPrograma(la_pcb, la_cpu->ip, la_cpu->puerto); //Falta enviar contexto de ejecucion Path prox instruccion.
		la_pcb->estado = 1;
		agregarAListaEjecucion(la_pcb->pid);
		return 1;
	}
	//PLANIFICAR buscarCpuLibre y algoritmo de ejecucion.

}

void agregarAListaEjecucion(int pid) {
	//Tomo el tiempo en que lo agrego a la cola de listos
	time_t tiempo;
	time(&tiempo);
	list_add(lista_ejecucion, cola_create(pid, tiempo));
}

void eliminarDeListaEjecucion(int pid) {
	bool _true(void *elem) {
		return (((t_cola*) elem)->pid == pid);
	}
	t_cola* cola_eliminar = list_remove_by_condition(lista_ejecucion, _true);
	//Tomo el tiempo en el que lo saco de la cola de listos.
	time_t tiempo;
	time(&tiempo);
	double tejecucion = difftime(tiempo, cola_eliminar->tiempoIngreso);
	t_pcb* la_pcb = buscarPCBporPid(pid);
	la_pcb->tejecucion = la_pcb->tejecucion + tejecucion;
	cola_destroy(cola_eliminar);
}

void eliminarPcb(int pid) {
	bool _true(void *elem) {
		return (((t_pcb*) elem)->pid == pid);
	}
	t_pcb* pcb_eliminar = list_remove_by_condition(lista_procesos, _true);
	//Tomo el tiempo en el que lo saco de la cola de listos.
	pcb_destroy(pcb_eliminar);
}

void agregarAColaListos(int pid) {
	//Tomo el tiempo en que lo agrego a la cola de listos
	time_t tiempo;
	time(&tiempo);
	list_add(cola_listos, cola_create(pid, tiempo));
}

t_pcb* eliminarDeColaListos() {

	t_cola* cola_eliminar = list_remove(cola_listos, 0);
	//Tomo el tiempo en el que lo saco de la cola de listos.
	time_t tiempo;
	time(&tiempo);
	double tespera = difftime(tiempo, cola_eliminar->tiempoIngreso);
	t_pcb* la_pcb = buscarPCBporPid(cola_eliminar->pid);
	la_pcb->tespera = la_pcb->tespera + tespera;
	cola_destroy(cola_eliminar);
	return la_pcb;
}

void agregarAColaBloqueados(int pid) {
	//Tomo el tiempo en que lo agrego a la cola de bloqueados
	time_t tiempo;
	time(&tiempo);
	list_add(cola_bloqueados, cola_create(pid, tiempo));
}

void eliminarDeColaBloqueados(int pid) {
	bool _true(void *elem) {
		return (((t_cola*) elem)->pid == pid);
	}
	t_cola* cola_eliminar = list_remove_by_condition(cola_bloqueados, _true);
	//Tomo el tiempo en el que lo saco de la cola de bloqueados.
	time_t tiempo;
	time(&tiempo);
	double trespuesta = difftime(tiempo, cola_eliminar->tiempoIngreso);
	t_pcb* la_pcb = buscarPCBporPid(pid);
	la_pcb->trespuesta = la_pcb->trespuesta + trespuesta;
	cola_destroy(cola_eliminar);
}

t_pcb* buscarPCBporPid(int pid) {
	t_pcb* la_pcb = malloc(sizeof(t_pcb));
	bool _true(void *elem) {
		return (((t_pcb*) elem)->pid == pid);
	}
	la_pcb = list_find(lista_procesos, _true);
	return la_pcb;
}

t_pcb* crearPcbProceso(char* archivo) {
	pidProcesos++;
	char* ruta = obtenerRutaArchivo(archivo);
	if (!(ruta == NULL)) {
		t_pcb* la_pcb = la_pcb = pcb_create(pidProcesos, ruta, 1, 0); //1 prox instriccion, 0 estado ready
		return la_pcb;
	} else {
		return NULL;
	}
}

char* obtenerRutaArchivo(char* archivo) {
	char* buf = malloc(PATH_MAX);
	char *res = realpath(archivo, buf);
	if (res) {
		printf("This source is at %s.\n", buf);
	} else {
		perror("realpath");
		return NULL;
	}
	return buf;
}

void recorrerCpu() {
	t_cpu * la_cpu;

	int i = 0;
	while (i < list_size(lista_cpu)) {
		la_cpu = list_get(lista_cpu, i);
		printf("Id Cpu:"COLOR_VERDE "%d\n"DEFAULT, la_cpu->id);
		printf("La IP:" COLOR_VERDE"%s\n"DEFAULT, la_cpu->ip);
		printf("El Puerto:"COLOR_VERDE"%s\n"DEFAULT, la_cpu->puerto);
		printf("Estado:"COLOR_VERDE "%d\n"DEFAULT, la_cpu->estado);
		i++;
	}
}

void recorrerProcesos() {
	t_pcb * la_pcb;

	int i = 0;
	while (i < list_size(lista_procesos)) {
		la_pcb = list_get(lista_procesos, i);
		printf(COLOR_VERDE"mProc %d: %s ->"COLOR_AMARILLO" %s\n"DEFAULT,
				la_pcb->pid, la_pcb->ruta, obtenerEstado(la_pcb->estado));
		i++;
	}
}

int ultimaInstruccion(char* ruta) {
	FILE* archivoMcod = fopen(ruta, "r");
	char *line = NULL;
	size_t len = 0;
	if (archivoMcod == NULL) {
		Error("Error al abrir el archivo");
		return -1;
	}
	int linea = 0;
	while (getline(&line, &len, archivoMcod) != -1) {
		linea++;
	}
	printf("EL ARCHIVO TIENEN %d LINEAS\n", linea);
	free(line);
	fclose(archivoMcod);
	return linea;
}

void finalizarProceso(int pid) {
	t_pcb* la_pcb = buscarPCBporPid(pid);
	if (la_pcb == NULL) {
		Error("El PID %d ingresado es incorrecto", pid);
	} else {
		la_pcb->proxInst = ultimaInstruccion(la_pcb->ruta);
	}

}
char* obtenerEstado(int estado) {
	if (estado == 0) {
		return "Listo";
	} else if (estado == 1) {
		return "Ejecutando";
	} else {
		return "Bloqueado";

	}
}

int ChartToInt(char x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);
	//char* aux = malloc(1 * sizeof(char));
	//sprintf(aux, "%c", x);
	numero = strtol(aux, (char **) NULL, 10);

	if (aux != NULL)
		free(aux);
	return numero;
}

int CharAToInt(char* x) {
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%s", x);

	numero = strtol(aux, (char **) NULL, 10);
	if (aux != NULL)
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

int ObtenerTamanio(char *buffer, int posicion, int dig_tamanio) {
	int x, digito, aux = 0;
	for (x = 0; x < dig_tamanio; x++) {
		digito = PosicionDeBufferAInt(buffer, posicion + x);
		aux = aux * 10 + digito;
	}
	return aux;
}

int ObtenerComandoMSJ(char* buffer) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return PosicionDeBufferAInt(buffer, 0);
}

int AtiendeCpu(char* buffer) {

	char *la_Ip, *el_Puerto;
	int digitosCantNumIp = 0, tamanioDeIp;
	int posActual = 0;
	t_cpu * la_cpu;

	//BUFFER RECIBIDO = 1119127.0.0.1146000
	//1: es cpu 1: es primer conexion 19127.0.0.1: la ip 146000: el puerto
	//Ese 3 que tenemos abajo es la posicion para empezar a leer el buffer 411

	digitosCantNumIp = PosicionDeBufferAInt(buffer, 2);
	//printf("Cantidad de digitos de Tamanio de Ip:%d\n",digitosCantNumIp);
	tamanioDeIp = ObtenerTamanio(buffer, 3, digitosCantNumIp);
	//printf("Tamaño de IP:%d\n",tamanioDeIp);
	if (tamanioDeIp >= 10) {
		posActual = digitosCantNumIp;
	} else {
		posActual = 1 + digitosCantNumIp;
	}
	la_Ip = DigitosNombreArchivo(buffer, &posActual);
	//printf("Ip:%s\n",la_Ip);
	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	//printf("Puerto:%s\n",el_Puerto);
	int id = list_size(lista_cpu) + 1;
	la_cpu = cpu_create(id, la_Ip, el_Puerto, 0);
	list_add(lista_cpu, la_cpu);

	return 1;
}

int finDeQuantum(char* buffer){
	char *el_Puerto, *pid, *instrucRealizadas, *resultadoInstrucciones;
	int posActual = 2;

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto:%s\n", el_Puerto);
	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("PID:%s\n", pid);
	instrucRealizadas = DigitosNombreArchivo(buffer, &posActual);
	printf("instrucRealizadas:%s\n", instrucRealizadas);
	resultadoInstrucciones = DigitosNombreArchivo(buffer, &posActual);
	printf("resultadoInstrucciones:%s\n", resultadoInstrucciones);
	liberarCpu(el_Puerto);
	procesarInstrucciones(resultadoInstrucciones, CharAToInt(pid),CharAToInt(instrucRealizadas));
	t_pcb* la_pcb = buscarPCBporPid(CharAToInt(pid));
	la_pcb->estado=0;
	la_pcb->proxInst=la_pcb->proxInst+CharAToInt(instrucRealizadas);
	agregarAColaListos(CharAToInt(pid));

	if(list_size(cola_listos)>0)
	planificar();

	return 1;

}

int procesoBloqueado(char* buffer){
	char *el_Puerto, *pid, *instrucRealizadas, *resultadoInstrucciones, *tBloqueado;
	int posActual = 2;

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto:%s\n", el_Puerto);
	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("PID:%s\n", pid);
	tBloqueado = DigitosNombreArchivo(buffer, &posActual);
	printf("TBloqueado:%s\n", tBloqueado);
	instrucRealizadas = DigitosNombreArchivo(buffer, &posActual);
	printf("instrucRealizadas:%s\n", instrucRealizadas);
	resultadoInstrucciones = DigitosNombreArchivo(buffer, &posActual);
	printf("resultadoInstrucciones:%s\n", resultadoInstrucciones);
	liberarCpu(el_Puerto);
	procesarInstrucciones(resultadoInstrucciones, CharAToInt(pid),CharAToInt(instrucRealizadas));
	t_pcb* la_pcb = buscarPCBporPid(CharAToInt(pid));
	la_pcb->estado=2;
	la_pcb->proxInst=la_pcb->proxInst+CharAToInt(instrucRealizadas);

	//Tomo el tiempo en que lo agrego a la cola de bloqueados
	time_t tiempoIngreso;
	time(&tiempoIngreso);
	printf("\nEstoy bloqueado por %s segundos\n", tBloqueado);
	sem_wait(&sEntradaSalida);
	sleep(CharAToInt(tBloqueado));
	sem_post(&sEntradaSalida);

	//Tomo el tiempo en el que lo saco de la cola de bloqueados.
	time_t tiempo;
	time(&tiempo);
	double trespuesta = difftime(tiempo,tiempoIngreso);
	la_pcb->trespuesta = la_pcb->trespuesta + trespuesta;
	la_pcb->estado=0;
	agregarAColaListos(CharAToInt(pid));

	if(list_size(cola_listos)>0)
		planificar();

	return 1;

}


int finDeProceso(char* buffer) {

	char *el_Puerto, *pid, *instrucRealizadas, *resultadoInstrucciones;

	int posActual = 2;

	el_Puerto = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto:%s\n", el_Puerto);
	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("PID:%s\n", pid);
	instrucRealizadas = DigitosNombreArchivo(buffer, &posActual);
	printf("instrucRealizadas:%s\n", instrucRealizadas);
	resultadoInstrucciones = DigitosNombreArchivo(buffer, &posActual);
	printf("resultadoInstrucciones:%s\n", resultadoInstrucciones);
	liberarCpu(el_Puerto);
	procesarInstrucciones(resultadoInstrucciones, CharAToInt(pid),CharAToInt(instrucRealizadas));
	eliminarProceso(CharAToInt(pid));
	if(list_size(cola_listos)>0)
	planificar();

	return 1;
}

void eliminarProceso(int pid){
	eliminarDeListaEjecucion(pid);
	t_pcb* la_pcb = buscarPCBporPid(pid);
	printf("mProc %d FINALIZADO Tiempo de Espera:%.2f Tiempo de Ejecucion:%.2f Tiempo de Respuesta:%.2f",pid,la_pcb->tespera,la_pcb->tejecucion,la_pcb->trespuesta);
	eliminarPcb(pid);
}

void procesarInstrucciones(char* resultado, int pid,int cantInstrucciones) {
	int codigoInst;
	int posActual = 0, i,posActualAux=1;
	char* numPagina, *contenido, *tBloq, *instruccion;
	for (i = 1; i <= cantInstrucciones; i++) {

		instruccion= DigitosNombreArchivo(resultado, &posActual);
		codigoInst = ObtenerComandoMSJ(instruccion);
		switch (codigoInst) {
		case 1:			//Resultado Iniciar
			if (ObtenerComandoMSJ(instruccion + 1)) {
				//Inicio satisfactorio
				printf("mProc %d - Iniciado\n", pid);
			} else {
				//Inicio Fallido
				printf("mProc %d - Fallo\n", pid);
				}
			break;
		case 2:			//Resultado Leer
			if (ObtenerComandoMSJ(instruccion + 1)) {
			numPagina = DigitosNombreArchivo(instruccion, &posActualAux);
			contenido = DigitosNombreArchivo(instruccion, &posActualAux);
			printf("mProc %d - Pagina %s leida: %s\n", pid, numPagina,
					contenido);
			posActualAux=1;
			}else{
				//Lectura Fallida
				printf("mProc %d - Fallo al leer\n", pid);
			}

			break;
		case 3:			//Resultado Escribir
			if (ObtenerComandoMSJ(instruccion + 1)) {
			numPagina = DigitosNombreArchivo(instruccion, &posActualAux);
			contenido = DigitosNombreArchivo(instruccion, &posActualAux);
			printf("mProc %d - Pagina %s escrita: %s\n", pid, numPagina,
					contenido);
			posActualAux=1;
			}else{
				//Escritura Fallida
				printf("mProc %d - Fallo al escribir\n", pid);
			}
			break;
		case 4:			//Resultado E/S
			tBloq =DigitosNombreArchivo(instruccion, &posActualAux);
			printf("mProc %d en entrada-salida de tiempo %s\n",pid,tBloq);
			posActualAux=1;
			break;
		case 5:			//Resultado Fin
			printf("mProc %d finalizado\n",pid);
			break;

		default:
			break;
		}
	}
}


int AtiendeCliente(void * arg) {
			int socket = (int) arg;
			//int id=-1;

			int longitudBuffer;

// Es el encabezado del mensaje. Nos dice quien envia el mensaje
			int mensajeEmisor = 0;

// Dentro del buffer se guarda el mensaje recibido por el cliente.
			char* buffer;
			buffer = malloc(BUFFERSIZE * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

// Cantidad de bytes recibidos.
			int bytesRecibidos;

// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
			int desconexionCliente = 0;

// Código de salida por defecto
			int code = 0;
			int cantRafaga = 1, tamanio = 0;
			char * mensaje;
			while ((!desconexionCliente) & g_Ejecutando) {
				//	buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.
				if (buffer != NULL)
					free(buffer);
				buffer = string_new();

				//Recibimos los datos del cliente
				buffer = RecibirDatos(socket, buffer, &bytesRecibidos,
						&cantRafaga, &tamanio);

				if (bytesRecibidos > 0) {
					//Analisamos que peticion nos está haciendo (obtenemos el comando)
					mensajeEmisor = ObtenerComandoMSJ(buffer + 1);
					//Evaluamos los comandos
					switch (mensajeEmisor) {
					case CONEXION_CPU:        //Primer conexion de una CPU
						if (AtiendeCpu(buffer)) {
							mensaje = "Ok";
						} else {
							mensaje = "No";
						}
						break;
					case PROCESO_FIN:
						if (finDeProceso(buffer)) {
							//Mostrar el resultado de la ejecuion
							//Liberar CPU
							//Eliminar estrucutras del proceso
							//Mostrar las metricas del proceso
							//Planificar
							mensaje = "Ok";
						} else {
							mensaje = "No";
						}
						break;
					case PROCESO_BLOQ:
						if (procesoBloqueado(buffer)) {
							//Mostrar el resultado de la ejecuion
							//Liberar CPU
							//Agregrar el proceso a la cola de E/S
							//Ejecutar tiempo indicado de E/S
							//Planificar
							mensaje = "Ok";
						} else {
							mensaje = "No";
						}
						break;
					case FIN_QUANTUM:
						if (finDeQuantum(buffer)) {
							//Mostrar el resultado de la ejecuion
							//Liberar CPU
							//Agregar el proceso a la cola de listos
							//Planificar

							mensaje = "Ok";
						} else {
							mensaje = "No";
						}
						break;

					case EJECUCION_CPU:
						if (1) {
							//Carga en las CPU correspondiente el % de ejecucion recibido

							mensaje = "Ok";
						} else {
							mensaje = "No";
						}
						break;

					default:
						break;
					}
					longitudBuffer = strlen(mensaje);
					//printf("\nRespuesta: %s\n",buffer);
					// Enviamos datos al cliente.
					EnviarDatos(socket, mensaje, longitudBuffer);
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

			if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
				log_info(logger, "Error al hacer el 'setsockopt'");
			}

			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons(g_Puerto_Planificador);
			my_addr.sin_addr.s_addr = htons(INADDR_ANY);
			memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

			if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr))
					== -1)
				log_info(logger,
						"Error al hacer el Bind. El puerto está en uso");

			if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
				log_info(logger,
						"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

			//	log_trace(logger,
			//		"SOCKET LISTO PARA RECBIR CONEXIONES. Numero de socket: %d, puerto: %d",
			//	socket_host, fs_Puerto);

			while (g_Ejecutando) {
				int socket_client;

				size_addr = sizeof(struct sockaddr_in);

				if ((socket_client = accept(socket_host,
						(struct sockaddr *) &client_addr, &size_addr)) != -1) {
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
					log_info(logger,
							"ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
				}
			}
			CerrarSocket(socket_host);
		}

		char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,
				int *cantRafaga, int *tamanio) {
			*bytesRecibidos = 0;
			char *bufferAux = malloc(1);
			memset(bufferAux, 0, 1);
			int digTamanio;
			if (buffer != NULL) {
				free(buffer);
			}

			if (*cantRafaga == 1) {
				bufferAux = realloc(bufferAux, BUFFERSIZE * sizeof(char));
				memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

				if ((*bytesRecibidos = *bytesRecibidos
						+ recv(socket, bufferAux, BUFFERSIZE, 0)) == -1) {
					log_info(logger,
							"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",
							socket);
				}

				digTamanio = PosicionDeBufferAInt(bufferAux, 1);
				*tamanio = ObtenerTamanio(bufferAux, 2, digTamanio);

			} else if (*cantRafaga == 2) {
				bufferAux = realloc(bufferAux, *tamanio * sizeof(char) + 1);
				memset(bufferAux, 0, *tamanio * sizeof(char) + 1); //-> llenamos el bufferAux con barras ceros.

				if ((*bytesRecibidos = *bytesRecibidos
						+ recv(socket, bufferAux, *tamanio, 0)) == -1) {
					Error(
							"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",
							socket);
				}
			}

			//log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s tamanio:%d", socket,
			//(char*) bufferAux, strlen(bufferAux));
			return bufferAux; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
		}

		int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar) {
// Retardo antes de contestar una solicitud
			//sleep(g_Retardo / 1000);

			int bytecount;

			//printf("CantidadBytesAEnviar:%d\n",cantidadDeBytesAEnviar);

			if ((bytecount = send(socket, buffer, cantidadDeBytesAEnviar, 0))
					== -1)
				log_info(logger,
						"No puedo enviar información a al clientes. Socket: %d",
						socket);
			//printf("Cuanto Envie:%d\n",bytecount);
			//Traza("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);

			//char * bufferLogueo = malloc(5);
			//bufferLogueo[cantidadDeBytesAEnviar] = '\0';

			//memcpy(bufferLogueo,buffer,cantidadDeBytesAEnviar);
			if (strlen(buffer) < 50) {
				log_info(logger, "ENVIO DATOS. socket: %d. Buffer:%s ", socket,
						(char*) buffer);
			} else {
				log_info(logger,
						"ENVIO DATOS. socket: %d. Tamanio:%d Buffer:%s", socket,
						strlen(buffer), buffer);
			}

			return bytecount;
		}

		void CerrarSocket(int socket) {
			close(socket);
			//Traza("SOCKET SE CIERRA: (%d).", socket);
			//log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
		}

		t_cpu* buscarCpuLibre() {
			t_cpu* la_cpu = malloc(sizeof(t_cpu));
			bool _true(void *elem) {
				return (((t_cpu*) elem)->estado == 0);
			}
			la_cpu = list_find(lista_cpu, _true);
			if (la_cpu != NULL) {
				la_cpu->estado = 1;
			}
			return la_cpu;
		}

		void liberarCpu(char* puerto) {
			t_cpu* la_cpu = malloc(sizeof(t_cpu));
			bool _true(void *elem) {
				return (!strcmp(((t_cpu*) elem)->puerto,puerto));
			}
			la_cpu = list_find(lista_cpu, _true);
			if (la_cpu != NULL) {
				la_cpu->estado = 0;
			}
		}


		int iniciarPrograma(t_pcb* proceso, char* ip, char*puerto) {
			char* bufferE, *bufferR;
			int socket, tamanioE, bytesRecibidos, cantRafaga = 1, tamanio;
			bufferE = string_new();
			bufferR = string_new();
			//SOLO UNA RAFAGA
			string_append(&bufferE, "21");
			string_append(&bufferE,
					obtenerSubBuffer(string_itoa(proceso->pid)));
			string_append(&bufferE, obtenerSubBuffer(proceso->ruta));
			string_append(&bufferE,
					obtenerSubBuffer(string_itoa(proceso->proxInst)));
			if((strcmp(g_Algoritmo_Planificador,"FIFO"))){
				string_append(&bufferE,obtenerSubBuffer("0"));
			}else{
				string_append(&bufferE,obtenerSubBuffer(string_itoa(g_Quantum_Planificador)));
			}
			if (conectarCpu(&socket, ip, puerto)) {
				tamanioE = strlen(bufferE);
				if (tamanioE == EnviarDatos(socket, bufferE, tamanioE)) {
					//bufferR = RecibirDatos(socket, bufferR, &bytesRecibidos,
					//		&cantRafaga, &tamanio);
					//printf("TAMANIO DE BUFFER:%d\n", tamanio);
					if (bufferR != NULL) {
						return 1;
					}
				}
			}
			return 0;
		}

		int conectarCpu(int * socket_Cpu, char* ipCpu, char* puertoCpu) {

			//ESTRUCTURA DE SOCKETS; EN ESTE CASO CONECTA CON CPU
			//log_info(logger, "Intentando conectar a nodo\n");
			//conectar con CPU
			struct addrinfo hints;
			struct addrinfo *serverInfo;
			int conexionOk = 0;

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_UNSPEC;// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
			hints.ai_socktype = SOCK_STREAM;// Indica que usaremos el protocolo TCP

			if (getaddrinfo(ipCpu, puertoCpu, &hints, &serverInfo) != 0) {// Carga en serverInfo los datos de la conexion
				log_info(logger,
						"ERROR: cargando datos de conexion socket_nodo");
			}

			if ((*socket_Cpu = socket(serverInfo->ai_family,
					serverInfo->ai_socktype, serverInfo->ai_protocol)) < 0) {
				log_info(logger, "ERROR: crear socket_Cpu");
			}
			if (connect(*socket_Cpu, serverInfo->ai_addr,
					serverInfo->ai_addrlen) < 0) {
				log_info(logger, "ERROR: conectar socket_Cpu");
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

				// Preguntamos y obtenemos el puerto donde esta escuchando el planificador
				if (config_has_property(config, "PUERTO_ESCUCHA")) {
					g_Puerto_Planificador = config_get_int_value(config,
							"PUERTO_ESCUCHA");
				} else {
					Error("No se pudo leer el parametro PUERTO_ESCUCHA");
				}
				// Preguntamos y obtenemos el algoritmo de planificacion que va a utilizar
				if (config_has_property(config, "ALGORTIMO_PLANIFICACION")) {
					g_Algoritmo_Planificador = config_get_string_value(config,
							"ALGORTIMO_PLANIFICACION");
				} else {
					Error(
							"No se pudo leer el parametro ALGORTIMO_PLANIFICACION");
				}
				// Obtenemos el quantum que se va a utilizar si el algorimo lo requiere
				if (config_has_property(config, "QUANTUM")) {
					g_Quantum_Planificador = config_get_int_value(config,
							"QUANTUM");
				} else {
					Error("No se pudo leer el parametro QUANTUM");
				}

			} else {
				Error("No se pudo abrir el archivo de configuracion");
			}
			if (config != NULL) {
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
			if (nuevo != NULL)
				free(nuevo);
		}
#endif

		void SetearErrorGlobal(const char* mensaje, ...) {
			va_list arguments;
			va_start(arguments, mensaje);
			if (g_MensajeError != NULL)
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
			if (nuevo != NULL)
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

void planificar(){
	t_pcb* pcbSiguiente = eliminarDeColaListos();
	if(pcbSiguiente != NULL){ //Si no encuentra nada la cola de listos esta vacia.
		correrPrograma(pcbSiguiente);
	}

}
