// Bibliotecas //
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
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
#define COLOR_AMARILLO			"\x1b[33m"
#define COLOR_ROJO				"\x1b[31m"
#define DEFAULT   				"\x1b[0m"
#define PATH_CONFIG 			"config.cfg"		//Ruta del config
#define NOMBRE_ARCHIVO_CONSOLA  "Consola_planificador.txt"	//Nombre de archivo de consola
#define NOMBRE_ARCHIVO_LOG 		"planificador.log"			//Nombre de archivo de log
#define MAXLINEA				1000				//Maximo de linea de consola				//Cantidad maxima de directorios
#define TAMANIO_IP				16					//un string ejempl 192.168.001.123
#define BUFFERSIZE 				50					//Tamaño del buffer
/**************Mensajes con CPU**************/
#define CONEXION_CPU 			1					//Tamaño del buffer
#define PROCESO_FIN				2					//Tamaño del buffer
#define PROCESO_BLOQ			3					//Tamaño del buffer
#define FIN_QUANTUM 			4					//Tamaño del buffer
#define EJECUCION_CPU			5					//Tamaño del buffer


/*********************/
t_log* logger;								// Logger del commons
t_list *lista_cpu;							//Lista de Cpu conectadas.
t_list *lista_procesos;						//Lista de proceso de activos del planificador.
t_list *cola_bloqueados;      				//Cola de procesos en entrada y salida.
t_list *cola_listos;      					//Cola de procesos en estado listo.
t_list *lista_ejecucion;      				//Lista de procesos en ejecucion
FILE* g_ArchivoConsola;						// Archivo donde descargar info impresa por consola
char* g_MensajeError;						//Mensaje de error global.
pthread_t hConsola, hOrquestadorConexiones;	// Definimos los hilos principales
int g_Puerto_Planificador;
char* g_Algoritmo_Planificador;
int g_Quantum_Planificador;
int g_Ejecutando = 1;						// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
int pidProcesos = 0;							//Variable global para ir asignandole pid a los procesos.
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
	int pid;
	char * ruta;
	int proxInst;
	int estado; //0 ready, 1 running, 2 bloqueado
	double tespera;
	double tejecucion;
	double trespuesta;
} t_pcb;

t_pcb *pcb_create(int pid, char *ruta, int proxInst, int estado) {
	t_pcb *new = malloc(sizeof(t_pcb));
	new->pid = pid;
	new->ruta = strdup(ruta);
	new->proxInst= proxInst;
	new->estado = estado;
	new->tespera = 0;
	new->tejecucion = 0;
	new->trespuesta = 0;
	return new;
}

void pcb_destroy(t_pcb* self) {
	free(self);
}

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

typedef struct {
	int pid;
	time_t tiempoIngreso;
} t_cola;

t_cola *cola_create(int pid, time_t tiempo) {
	t_cola *new = malloc(sizeof(t_cola));
	new->pid = pid;
	new->tiempoIngreso = tiempo;
	return new;
}

void cola_destroy(t_cola* self) {
	free(self);
}

void Comenzar_Consola();
void HiloOrquestadorDeConexiones();
int operaciones_consola();
void recorrerCpu();
void LevantarConfig();
void Error(const char* mensaje, ...);
char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio);
int AtiendeCliente(void * arg);
int AtiendeCpu(char* buffer);
void HiloOrquestadorDeConexiones();
int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar);
void CerrarSocket(int socket);
int iniciarPrograma(t_pcb* proceso,char* ip,char*puerto,char**buffer);
int conectarCpu(int * socket_Cpu, char* ipCpu, char* puertoCpu);
char* obtenerSubBuffer(char *nombre);
char* DigitosNombreArchivo(char *buffer,int *posicion);
t_cpu* buscarCpuLibre();
t_pcb* crearPcbProceso(char* archivo);
char* obtenerRutaArchivo(char* archivo);
char* obtenerEstado(int estado);
void recorrerProcesos();
void agregarAColaListos(int pid);
void eliminarDeColaListos(int pid);
void agregarAListaEjecucion(int pid);

int correrPrograma(t_pcb* la_pcb);
t_pcb* buscarPCBporPid(int pid);
int ultimaInstruccion(char* ruta);
void finalizarProceso(int pid);
