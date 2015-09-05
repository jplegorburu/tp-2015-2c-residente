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
#define NOMBRE_ARCHIVO_LOG 		"cpu.log"			//Nombre de archivo de log		//Cantidad maxima de directorios
#define TAMANIO_IP				16					//un string ejempl 192.168.001.123


/*********************/
t_log* logger;								// Logger del commons
char* g_MensajeError;						//Mensaje de error global.
char* g_Puerto_Planificador;
char* g_Ip_Planificador;
char* g_Puerto_Memoria;
char* g_Ip_Memoria;
int g_Cant_Hilos;
int g_Retardo;
int g_Ejecutando = 1;						// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
int g_Puerto_CPU = 4500;
pthread_t hOrquestadorConexiones; 			//Hilo de conexion
#define BUFFERSIZE 50
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
int conectarPlanificador(int *socket_plani);
char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio);
char* obtenerSubBuffer(char *nombre);
int ChartToInt(char x);
int PosicionDeBufferAInt(char* buffer, int posicion);
int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar);
int cuentaDigitos(int valor);
int ObtenerTamanio (char *buffer , int posicion, int dig_tamanio);
int AtiendeCliente(void * arg);
void HiloOrquestadorDeConexiones();
void CerrarSocket(int socket);
int ObtenerComandoMSJ(char* buffer);
void conectarseMemoria();
void conectarsePlanificador();
int conectarMemoria(int *socket_memoria);


