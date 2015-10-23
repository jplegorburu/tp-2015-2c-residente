// Bibliotecas //
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
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
#define NOMBRE_ARCHIVO_LOG 		"swap.log"			//Nombre de archivo de log		//Cantidad maxima de directorios
#define TAMANIO_IP				16					//un string ejempl 192.168.001.123
#define BUFFERSIZE 				50					//Tamaño del buffer

/*********************/
t_log* logger;								// Logger del commons
char* g_MensajeError;
int g_Puerto_Swap;
char* g_Arch_Swap;
int g_Cant_Pags;
int g_Tam_Pags;
int g_Retardo_Compact;
int g_Ejecutando = 1;						// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
t_list* listaOcupado;
t_list* listaLibre;

FILE *archivoSwap;

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
		int paginaInicio;
		int cantidadPaginas;
	} espacio_ocupado;

typedef struct {
		int paginaInicio;
		int cantidadPaginas;
	} espacio_libre;


//MANEJO DE BUFFER
int ObtenerComandoMSJ(char* buffer);
char* obtenerSubBuffer(char *nombre);
char* DigitosNombreArchivo(char *buffer, int *posicion);
int CharAToInt(char* x);
char* obtenerSubBuffer(char *nombre);
int PosicionDeBufferAInt(char* buffer, int posicion);
int ObtenerTamanio (char *buffer , int posicion, int dig_tamanio);

//MANEJO DE ARCHIVO SWAP
void crearArchivoParticionSwap();
void abrirArchivoParticionSwap();
void cerrarArchivoParticionSwap();
int setPaginaDeArchivo(int pid, int pagina, char*datos);
char* getPaginaDeArchivo(int numero);
void quitarProcesoDeArchivo(int inicio, int cantPaginas);
char *leerPagina(int pid, int pagina);

//ARCHIVO DE CONFIGURACION Y ERRORES
void LevantarConfig();
void Error(const char* mensaje, ...);

//SOCKETS
void escucharConexiones();
char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos,int *cantRafaga,int *tamanio);
int EnviarDatos(int socket, char *buffer, int cantidadDeBytesAEnviar);
void CerrarSocket(int socket);

//MANEJO DE LISTAS Y ESPACIOS
void iniciarlizarListas();
bool hayLugar(int cantPaginas);
void mostrarListaLibres();
void mostrarListaOcupados();
void agregarElementoALibres(int inicio, int paginas);
void agregarElementoAOcupados(int pid, int inicio, int paginas);
bool hayLugarCompactando(int paginasNecesarias);
int hayLugarLibreSinCompactar(int paginasNecesarias);
espacio_ocupado *crearElementoOcupado(int pid, int inicio, int paginas);
espacio_libre *crearElementoLibre(int inicio, int paginas);
void quitarEspacioLibre(int inicio, int paginas);
void agregarProcesoYActualizarListas(int pid, int inicio, int paginas);
void compactar();

//MANEJO DE PROCESOS
int agregarProceso(int pid, int paginas);
int quitarProceso(int pid);

//METODOS DE COMUNICACION
char* informarAgregarProceso(char* buffer);
char* informarQuitarProceso(char* buffer);
char* informarLecturaPagina(char* buffer);
char* informarEscrituraPagina(char* buffer);
char* informarConexionConMemoria(char*buffer);
