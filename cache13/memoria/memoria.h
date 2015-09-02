// Bibliotecas //
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define COLOR_VERDE   			"\x1b[32m"
#define DEFAULT   				"\x1b[0m"
#define PATH_CONFIG 			"config.cfg"		//Ruta del config
#define NOMBRE_ARCHIVO_LOG 		"memoria.log"			//Nombre de archivo de log		//Cantidad maxima de directorios
#define TAMANIO_IP				16					//un string ejempl 192.168.001.123


/*********************/
t_log* logger;								// Logger del commons
char* g_MensajeError;						//Mensaje de error global.
pthread_t hConsola;	// Definimos los hilos principales

int g_Puerto_Memoria;
char* g_Ip_Swap;
char* g_Puerto_Swap;
int g_Max_Marcos_Proc;
int g_Cant_Marcos;
int g_Tam_Marcos;
int g_Entradas_Tlb;
char* g_Tlb_Habilitada;
int g_Retardo_Memoria;
int g_Ejecutando = 1;						// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.

// TIPOS //
typedef enum {
	CantidadArgumentosIncorrecta,
	NoSePudoAbrirConfig,
	NoSePuedeObtenerPuerto,
	NoSePuedeObtenerNodos,
	NosePuedeCrearHilo,
	OtroError,
} t_error;							//Tipo error

void LevantarConfig();
void Error(const char* mensaje, ...);

