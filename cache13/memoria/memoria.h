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

char* g_Ip_Memoria;
int g_Puerto_Memoria;
char* g_Ip_Swap;
char* g_Puerto_Swap;
int g_Max_Marcos_Proc;
int g_Cant_Marcos;
int g_Tam_Marcos;
int g_Entradas_Tlb;
char* g_Tlb_Habilitada;
char* g_Algoritmo_Reemplazo;
char* g_Algoritmo_TLB;
int g_Retardo_Memoria;
int g_Ejecutando = 1;						// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
t_list* lista_cpu; 							//Lista de Cpus conectadas.
t_list* lista_procesos;
t_list* marcos;
t_list* TLB;
#define BUFFERSIZE 200
pthread_t hOrquestadorConexiones;
int socket_swap;
char* memoriaPrincipal;
int punteroClock=0;      //puntero para el algoritmo clock modificado
int TLBhits =0;
int accesosTotal =0;
// TIPOS //
typedef enum {
	CantidadArgumentosIncorrecta,
	NoSePudoAbrirConfig,
	NoSePuedeObtenerPuerto,
	NoSePuedeObtenerNodos,
	NosePuedeCrearHilo,
	OtroError,
} t_error;							//Tipo error

typedef struct{
	int frameNro;
	int pid;
	int pagina;
	int usado;
} t_frame;


t_frame *frame_create(int tam_marcos, int nroMarco) {
	t_frame *new = malloc(sizeof(t_frame));
	new->frameNro = nroMarco;
	new->pid = -1;
	new->pagina = -1;
	new->usado=0;
	return new;
}

void frame_destroy(t_frame* self) {
	free(self);
}

typedef struct{
	int frameNro;
	int uso;
	int modificado;
} t_marcoProceso;


t_marcoProceso *marcoProceso_create(int nroMarco) {
	t_marcoProceso *new = malloc(sizeof(t_marcoProceso));
	new->frameNro = nroMarco;
	new->uso = 0;
	new->modificado = 0;
	return new;
}

void marcoProceso_destroy(t_marcoProceso* self) {
	free(self);
}

typedef struct{
	int pagN;
	int frame;
	int presenteEnMemoria;
} entrada_tablaPags;

entrada_tablaPags *entradaTablaPags_create(int i) {
	entrada_tablaPags *new = malloc(sizeof(entrada_tablaPags));
	new->pagN = i;
	new->frame = -1;
	new->presenteEnMemoria = 0;
	return new;
}

void entradaTablaPags_destroy(entrada_tablaPags* self) {
	free(self);
}

typedef struct{
	int pid;
	t_list * tablaPags;
	t_list * framesAsignados;
	int falloPag;
	int accesoSwap;
	//int framesAsignados;
}entrada_tablaProcesos;

entrada_tablaProcesos *entradaTablaProcesos_create(int pid) {
	entrada_tablaProcesos *new = malloc(sizeof(entrada_tablaProcesos));
	new->pid = pid;
	new->tablaPags = list_create();
	new->framesAsignados = list_create();
	//new->framesAsignados = 0;
	new->falloPag=0;
	new->accesoSwap=0;
	return new;
}

void entradaTablaProcesos_destroy(entrada_tablaProcesos* self) {
	free(self);
}



typedef struct {
	char * ip;
	char * puerto;
	int procesoActivo;
} t_cpu;


t_cpu *cpu_create(char *ipCpu, char* puertoCpu) {
	t_cpu *new = malloc(sizeof(t_cpu));
	new->ip = strdup(ipCpu);
	new->puerto = strdup(puertoCpu);
	new->procesoActivo = 0;
	return new;
}

void cpu_destroy(t_cpu* self) {
	free(self);
}


typedef struct {
	int pid;
	int pagina;
	int frame;
} entrada_tlb;

entrada_tlb * entrada_tlb_create(){
	entrada_tlb *new = malloc(sizeof(entrada_tlb));
	new->pid = -1;
	new->pagina = -1;
	new->frame = -1;
	return new;
}


sem_t sem_swap;
int leerCpuError(char*ip, char*puerto);
int conectarConSwap(int *socket_swap);
void ConectarseConSwap(int g_puerto_memoria);
void LevantarConfig();
void Error(const char* mensaje, ...);
char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio);
int AtiendeCliente(void * arg);
void HiloOrquestadorDeConexiones();
int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar);
void CerrarSocket(int socket);
void HiloOrquestadorDeConexiones();
char* DigitosNombreArchivo(char *buffer, int *posicion);
void informarConexionCPU(char* buffer);
void informarFinDelProceso(char* buffer);
void informarInicio(char* buffer);
void informarLeer(char* buffer);
void informarEscribir(char* buffer);
char* obtenerSubBuffer(char *nombre);
void inicioProcesoSwap(int pid, int cant_pag);
void finProcesoSwap(int pid);
void leerSwap(int pid, int num_pag);
void escribirSwap(int pid, int num_pag, char* contenido);
void escribirSwapReemplazo(int pid, int num_pag, char* contenido);
void resultadoInicioSwap(char* buffer);
void resultadoLecturaSwap(char* buffer);
void resultadoEscrituraSwap(char* buffer);
void resultadoFinSwap(char* buffer);
int conectarConCpu(int *socket_cpu, char*ip, char*puerto);
t_cpu* buscarCPUporPid(int pid);
t_cpu* buscarCPUporPuerto(char* puerto);
int finProcesoCpu(char*ip, char*puerto);
int inicioProcesoCpu(char*ip, char*puerto,char* resultado);
int escribirCpu(char*ip, char*puerto,char* resultado);
int leerCpu(char*ip, char*puerto,char* pagina,char* contenido);
void mensajeDeSwap(char * buffer);
void inicializarListaMarcos(t_list * marcos, int g_Cant_Marcos);
entrada_tablaProcesos * buscarPorId(int id);
entrada_tablaPags * buscarPagina(entrada_tablaProcesos * proc, int numPag);
t_frame * buscarFrameLibre();
t_frame * buscarFramePorNumero(int nroFrame);
char* leerEnMP(int nroMarco);
int grabarEnMemoria(int nroMarco,  char * texto);
void correrAlgoritmo(entrada_tablaProcesos* proceso, entrada_tablaPags* tPaginas, char* contenido, int operacion);
t_marcoProceso * buscarMarcoProceso(t_list* listaFrames, int nroFrame);
void sacarMarcoProceso(t_list* listaFrames, int nroFrame);
entrada_tablaPags * buscarPaginaPorMarco(entrada_tablaProcesos * proc, int marco);
void mostrarTabaPaginas(int pid);
void crearTLB(int g_Entradas_Tlb);
entrada_tlb * buscarEnTLB(int id, int pagina);
entrada_tlb * sacarDeTLB();
void AtenderSenial(int s);
void * SENIAL();
void limpiarMemoria(void * arg);
void tlbFlush(void * arg);
void calcularTlbHits (void* arg);
