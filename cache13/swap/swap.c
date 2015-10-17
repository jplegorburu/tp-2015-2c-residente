#include "swap.h"

int main(int argv, char** argc) {

	//Archivo de Log
	logger = log_create(NOMBRE_ARCHIVO_LOG, "planificador", true, LOG_LEVEL_TRACE);

	//Creamos las listas de espacio en Swap ocupado y libre
	listaOcupado = list_create();
	listaLibre = list_create();



	// Levantamos el archivo de configuracion.
	LevantarConfig();

	crearArchivoParticionSwap();

	iniciarlizarListas();

	int test;

	test = agregarProceso(2,30);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(3,50);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(4,20);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(5,150);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(6,20);
	mostrarListaLibres();
	mostrarListaOcupados();

	quitarProceso(3);
	mostrarListaLibres();
	mostrarListaOcupados();

	quitarProceso(5);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(7,160);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(8,80);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(9,30);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(10,50);
	mostrarListaLibres();
	mostrarListaOcupados();

	quitarProceso(4);
	mostrarListaLibres();
	mostrarListaOcupados();

	test = agregarProceso(11,30);
	mostrarListaLibres();
	mostrarListaOcupados();

	escucharConexiones();


	return EXIT_SUCCESS;

}


void quitarProceso(int pid){

	printf("\nQUITAR PROCESO %d\n",pid);

	bool _trueOcupados(void *elem) {
			return (((espacio_ocupado*) elem)->pid == pid);
	}

	espacio_ocupado *ocupado = malloc(sizeof(espacio_ocupado));
	ocupado = list_find(listaOcupado, _trueOcupados);

	bool _trueLibreDerecha(void *elem) {
		return (((espacio_libre*) elem)->paginaInicio == (ocupado->paginaInicio + ocupado->cantidadPaginas));
	}

	espacio_libre *libreDerecha = malloc(sizeof(espacio_libre));
	libreDerecha = list_find(listaLibre, _trueLibreDerecha);

	bool _trueLibreIzquierda(void *elem) {
		return ((((espacio_libre*) elem)->paginaInicio + ((espacio_libre*) elem)->cantidadPaginas) == (ocupado->paginaInicio));
	}

	espacio_libre *libreIzquierda = malloc(sizeof(espacio_libre));
	libreIzquierda = list_find(listaLibre, _trueLibreIzquierda);

	espacio_libre *libre = malloc(sizeof(espacio_libre));

	libre->paginaInicio = ocupado->paginaInicio;
	libre->cantidadPaginas = ocupado->cantidadPaginas;

	if(libreIzquierda != NULL){
		libre->paginaInicio = libreIzquierda->paginaInicio;
		libre->cantidadPaginas = libreIzquierda->cantidadPaginas + ocupado->cantidadPaginas;

		bool _eliminarIzquierda(void *elem) {
			return (((espacio_libre*) elem)->paginaInicio == libreIzquierda->paginaInicio);
		}

		list_remove_by_condition(listaLibre, _eliminarIzquierda);
	}
	if(libreDerecha != NULL){
		libre->cantidadPaginas = libre->cantidadPaginas + libreDerecha->cantidadPaginas;

		bool _eliminarDerecha(void *elem) {
			return (((espacio_libre*) elem)->paginaInicio == libreDerecha->paginaInicio);
		}
		list_remove_by_condition(listaLibre, _eliminarDerecha);

	}

	agregarElementoALibres(libre->paginaInicio, libre->cantidadPaginas);

	list_remove_by_condition(listaOcupado, _trueOcupados);

	free(libreDerecha);
	free(libreIzquierda);
	free(libre);
}

//DEVUELVE 1 SI EL PROCESO SE PUDO INICIAR Y 0 SI SE RECHAZO POR FALTA DE ESPACIO EN SWAP
int agregarProceso(int pid, int paginas){
	int paginaDeInicio = hayLugarLibreSinCompactar(paginas);

	if(paginaDeInicio != -1){
		agregarProcesoYActualizarListas(pid, paginaDeInicio, paginas);
		printf("\nPROCESO %d AGREGADO! Paginas: %d\n", pid, paginas);
	}else{
		if(hayLugarCompactando(paginas)){
			//COMPACTAR
			printf("\nCOMPACTAR!\n");
			compactar();
			paginaDeInicio = hayLugarLibreSinCompactar(paginas);
			agregarProcesoYActualizarListas(pid, paginaDeInicio, paginas);
			printf("\nPROCESO %d AGREGADO! Paginas: %d\n", pid, paginas);
		}else{
			// 0 indica proceso rechazado
			printf("\nPROCESO %d RECHAZADO! Paginas: %d\n", pid, paginas);
			return 0;
		}
	}
	//INDICA QUE EL PROCESO SE INICIO
	return 1;
}

void compactar(){
	int espacioTotalLibre=0;
	int espacioTotalOcupado=0;
	int index=0;

	int inicioAnterior=1;
	int cantPaginasAnterior=0;

	espacio_libre *libre = malloc(sizeof(espacio_libre));

	libre = list_get(listaLibre,index);
	while(libre!=NULL){
		espacioTotalLibre = espacioTotalLibre + libre->cantidadPaginas;
		index++;
		libre = list_get(listaLibre, index);
	}

	index = 0;
	espacio_ocupado *ocupado = malloc(sizeof(espacio_ocupado));

	ocupado = list_get(listaOcupado, index);
	while(ocupado!=NULL){
		espacioTotalOcupado = espacioTotalOcupado + ocupado->cantidadPaginas;

		ocupado->paginaInicio = inicioAnterior + cantPaginasAnterior;

		inicioAnterior = ocupado->paginaInicio;
		cantPaginasAnterior = ocupado->cantidadPaginas;

		list_replace(listaOcupado, index, ocupado);

		index++;
		ocupado = list_get(listaOcupado, index);
	}

	list_clean(listaLibre);

	agregarElementoALibres(espacioTotalOcupado, espacioTotalLibre);

}

void agregarProcesoYActualizarListas(int pid, int inicio, int paginas){
	agregarElementoAOcupados(pid, inicio, paginas);
	quitarEspacioLibre(inicio, paginas);
}

void quitarEspacioLibre(int inicio, int paginas){

	bool _true(void *elem) {
		return (((espacio_libre*) elem)->paginaInicio == inicio);
	}

	espacio_libre *libre = malloc(sizeof(espacio_libre));
	libre = list_find(listaLibre, _true);

	list_remove_by_condition(listaLibre, _true);

	libre->paginaInicio = libre->paginaInicio + paginas;
	libre->cantidadPaginas = libre->cantidadPaginas - paginas;

	agregarElementoALibres(libre->paginaInicio, libre->cantidadPaginas);
}

void agregarElementoAOcupados(int pid, int inicio, int paginas){
	list_add(listaOcupado, crearElementoOcupado(pid, inicio, paginas));
}



bool hayLugarCompactando(int paginasNecesarias){
	int espacioTotal=0;
	int index=0;
	espacio_libre *libre = malloc(sizeof(espacio_libre));

	libre = list_get(listaLibre,index);
	while(libre!=NULL){
		espacioTotal = espacioTotal + libre->cantidadPaginas;
		index++;
		libre = list_get(listaLibre, index);
	}

	free(libre);

	if(espacioTotal<paginasNecesarias){
		return false;
	}else{
		return true;
	}
}

//Devuelve -1 si no hay lugares contiguos. Si hay, devuevle la pagina de inicio
int hayLugarLibreSinCompactar(int paginasNecesarias){

	bool _true(void *elem) {
		return (((espacio_libre*) elem)->cantidadPaginas >= paginasNecesarias);
	}

	espacio_libre *libre = malloc(sizeof(espacio_libre));
	libre = list_find(listaLibre, _true);

	if (libre==NULL){
		return -1;
	}else{
		return libre->paginaInicio;
	}
}

/*int tieneEspacio(int paginas, int cantPaginas){
	if (palibreginas >= cantPaginas){
		return 1;
	}else{
		return 0;
	}
}*/

espacio_libre *crearElementoLibre(int inicio, int paginas){
	espacio_libre *libre = malloc(sizeof(espacio_libre));
	libre->cantidadPaginas = paginas;
	libre->paginaInicio = inicio;
	return libre;
}

espacio_ocupado *crearElementoOcupado(int pid, int inicio, int paginas){
	espacio_ocupado *ocupado = malloc(sizeof(espacio_ocupado));
	ocupado->pid = pid;
	ocupado->cantidadPaginas = paginas;
	ocupado->paginaInicio = inicio;
	return ocupado;
}

void agregarElementoALibres(int inicio, int paginas){

	list_add(listaLibre,crearElementoLibre(inicio, paginas));
}

void mostrarListaLibres(){

	printf("------------------------------------------------------\nLibres: \n");

	int index=0;
	espacio_libre *libre = malloc(sizeof(espacio_libre));

	libre = list_get(listaLibre,index);
	while(libre!=NULL){
		printf("Inicio: %d  //   Paginas: %d\n", libre->paginaInicio, libre->cantidadPaginas);
		index++;
		libre = list_get(listaLibre, index);
	}

	free(libre);
}

void mostrarListaOcupados(){

	printf("------------------------------------------------------\nOcupados: \n");

	int index=0;
	espacio_ocupado *ocupado = malloc(sizeof(espacio_ocupado));

	ocupado = list_get(listaOcupado,index);
	while(ocupado!=NULL){
		printf("Inicio: %d  //   Paginas: %d     //      PID: %d\n", ocupado->paginaInicio, ocupado->cantidadPaginas, ocupado->pid);
		index++;
		ocupado = list_get(listaOcupado, index);
	}

	free(ocupado);
}

void iniciarlizarListas(){

	list_add(listaLibre,crearElementoLibre(0,g_Cant_Pags-1));

}

void crearArchivoParticionSwap(){
	//Crea un archivo lleno de \0 para la particion Swap tomando los valores del archivo de configuracion
	system(string_from_format("dd if=/dev/zero of=%s bs=%i count=%i", g_Arch_Swap, g_Tam_Pags,g_Cant_Pags));
}

void escucharConexiones(){

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
	my_addr.sin_port = htons(g_Puerto_Swap);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		log_info(logger,"Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		log_info(logger,
				"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
		int desconexionCliente = 0;

	//while(g_Ejecutando) {
	while ((!desconexionCliente) & g_Ejecutando){
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		//socket_client = accept(socket_host,(struct sockaddr *) &client_addr, &size_addr);

		if ((socket_client = accept(socket_host,(struct sockaddr *) &client_addr, &size_addr)) != -1) {
		//printf("Memoria conectada\n");

			int longitudBuffer;

			// Es el encabezado del mensaje. Nos dice quien envia el mensaje
				int emisor = 0;

			// Dentro del buffer se guarda el mensaje recibido por el cliente.
				char* buffer;
				buffer = malloc(BUFFERSIZE * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

			// Cantidad de bytes recibidos.
				int bytesRecibidos;

			// Código de salida por defecto
				int code = 0;
				int cantRafaga=1,tamanio=0;
				char * mensaje;

				//while ((!desconexionCliente) & g_Ejecutando) {
					//	buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.
					if (buffer != NULL )
						free(buffer);
					buffer = string_new();

						//Recibimos los datos del cliente
						buffer = RecibirDatos(socket_client, buffer, &bytesRecibidos,&cantRafaga,&tamanio);

						printf("\nESTOY RECIBIENDO DE MEMORIA: %s\n", buffer);

						if (bytesRecibidos > 0) {
							//Analisamos que peticion nos está haciendo (obtenemos el comando)
							emisor = ObtenerComandoMSJ(buffer);
							int funcion;

							//Evaluamos los comandos
							switch (emisor) {

								case 3: //3 es memoria

									funcion = ObtenerComandoMSJ(buffer+1);
									switch (funcion){
										case 1:
											//CONEXION CON SWAP
											printf("Conexion con Memoria establecida\n");
											mensaje = "OK";
										break;

										case 2:
											//INICIO DE PROCESO
											mensaje = informarAgregarProceso(buffer);
										break;

										case 3:
											//PEDIDO DE PAGINA
											mensaje = informarLecturaPagina(buffer);
										break;

										case 4:
											//ESCRIBIR PAGINA
										break;

										case 5:
											//FINALIZAR PROCESO
											mensaje = informarQuitarProceso(buffer);
										break;
									}
							}

							longitudBuffer=strlen(mensaje);
							printf("\nRespuesta: %s\n",mensaje);
							// Enviamos datos al cliente.
							EnviarDatos(socket_client, mensaje,longitudBuffer);
						} else
							desconexionCliente = 1;
						//}
						CerrarSocket(socket_client);
					} else {
						Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
					}
		}

	CerrarSocket(socket_host);

}

/*void leerDatosDeConexion(char* buffer){
	int posActual = 0;
	int digitosCantNumIp = 0, tamanioDeIp;

	digitosCantNumIp = PosicionDeBufferAInt(buffer, 2);

	tamanioDeIp = ObtenerTamanio(buffer, 3, digitosCantNumIp);

	if (tamanioDeIp >= 10) {
		posActual = digitosCantNumIp;
	} else {
		posActual = 1 + digitosCantNumIp;
	}
	//Chequeo el tamaño de la ip xq si es mas chica de 10 puede generar fallas.
	ip_Memoria = DigitosNombreArchivo(buffer, &posActual);
	printf("Ip:%s\n",ip_Memoria);
	puerto_Memoria = DigitosNombreArchivo(buffer, &posActual);
	printf("Puerto:%s\n",puerto_Memoria);
}*/

char *leerPagina(int pid, int pagina){

	char *lectura = string_new();

	bool _trueOcupados(void *elem) {
			return (((espacio_ocupado*) elem)->pid == pid);
	}

	espacio_ocupado *ocupado = malloc(sizeof(espacio_ocupado));
	ocupado = list_find(listaOcupado, _trueOcupados);

	//VALIDAR POR SI NO EXISTE PID



	//OJO QUE LA PAGINA DE INICIO COMIENZA EN 1.
	//SI QUIERO LEER LA PAGINA 3, DEBO HACER PAGDEINICIO + PAGINA (EN ESTE CASO 3) - 1


	string_append(&lectura,string_itoa(ocupado->paginaInicio+pagina));

	return lectura;
}

char* informarLecturaPagina(char* buffer){
	char *pid, *nroPagina;
	int posActual = 2;

	char *msg = string_new();

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	nroPagina = DigitosNombreArchivo(buffer, &posActual);
	printf("Nro de pagina:%s\n", nroPagina);

	 //Error en la lectura -->  42 + PID + 0
	// Lectura exitosa --> 42 + PID + Pagina + Contenido
	string_append(&msg,"42");
	string_append(&msg,obtenerSubBuffer(pid));
	//0 SI HUBO ERROR
	string_append(&msg,obtenerSubBuffer(nroPagina));
	string_append(&msg,obtenerSubBuffer(leerPagina(CharAToInt(pid), CharAToInt(nroPagina))));

	return msg;
}

char* informarQuitarProceso(char* buffer){
	char *pid;
	int posActual = 2;

	char *msg = string_new();

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	quitarProceso(CharAToInt(pid));
	 // 44 + pid + resultado

	string_append(&msg,"44");
	string_append(&msg,obtenerSubBuffer(pid));
	string_append(&msg,obtenerSubBuffer(string_itoa(1)));

	return msg;
}

char* informarAgregarProceso(char* buffer){
	char *pid, *cantPaginas;
	int posActual = 2;
	int resultado;

	char *msg = string_new();

	pid = DigitosNombreArchivo(buffer, &posActual);
	printf("pid:%s\n", pid);

	cantPaginas = DigitosNombreArchivo(buffer, &posActual);
	printf("Cantidad de paginas:%s\n", cantPaginas);

	printf("Cantidad de paginas:%d\n", CharAToInt(cantPaginas));

	resultado = agregarProceso(CharAToInt(pid), CharAToInt(cantPaginas));

	//DEVUELVO 41 + pid + resultado
	if(resultado==1){
		printf("\nEL PROCESO %s SE INICIO\n", pid);
	}else{
		printf("\nEL PROCESO %s NO SE INICIO\n", pid);
	}

	mostrarListaLibres();

	string_append(&msg,"41");
	string_append(&msg,obtenerSubBuffer(pid));
	string_append(&msg,obtenerSubBuffer(string_itoa(resultado)));

	return msg;

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

	printf("\nSOY EL SWAP, ESTOY ENVIANDO: %s\n", buffer);

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
