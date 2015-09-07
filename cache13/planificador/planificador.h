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
#define NOMBRE_ARCHIVO_CONSOLA  "Consola_fs.txt"	//Nombre de archivo de consola
#define NOMBRE_ARCHIVO_LOG 		"planificador.log"			//Nombre de archivo de log
#define MAXLINEA				1000				//Maximo de linea de consola				//Cantidad maxima de directorios
#define TAMANIO_IP				16					//un string ejempl 192.168.001.123
#define BUFFERSIZE 				50					//Tamaño del buffer

/*********************/
t_log* logger;								// Logger del commons
t_list *lista_cpu;							//Lista de Cpu
FILE* g_ArchivoConsola;						// Archivo donde descargar info impresa por consola
char* g_MensajeError;						//Mensaje de error global.
pthread_t hConsola, hOrquestadorConexiones;	// Definimos los hilos principales
int g_Puerto_Planificador;
char* g_Algoritmo_Planificador;
int g_Quantum_Planificador;
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

typedef struct {
	int id;
	char * ip;
	char * puerto;
	int estado;
} t_cpu;

t_cpu *cpu_create(int id, char *ipCpu, char* puertoCpu, int activo) {
	t_cpu *new = malloc(sizeof(t_cpu));
	new->id = id;
	new->ip = strdup(ipCpu);
	new->puerto = strdup(puertoCpu);
	new->estado = activo;
	return new;
}

void cpu_destroy(t_cpu* self) {
	free(self);
}

void Comenzar_Consola();
void HiloOrquestadorDeConexiones();
int operaciones_consola();
void RecorrerCpu();
void LevantarConfig();
void Error(const char* mensaje, ...);
char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio);
int AtiendeCliente(void * arg);
int AtiendeCpu(char* buffer);
void HiloOrquestadorDeConexiones();
int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar);
void CerrarSocket(int socket);
int iniciarPrograma(char* nombreProg,char* ip,char*puerto,char**buffer);
int conectarCpu(int * socket_Cpu, char* ipCpu, char* puertoCpu);
char* obtenerSubBuffer(char *nombre);
char* DigitosNombreArchivo(char *buffer,int *posicion);
