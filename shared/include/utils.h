#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<semaphore.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<assert.h>
#include "../globals.h"


typedef enum
{
	MENSAJE,
	PAQUETE,
	NEW
,
	PROGRAMA
}op_code;

typedef enum
{
	SET, 
	ADD, 
	MOV_IN, 
	MOV_OUT, 
	IO,
    EXIT
} t_instCode;

typedef enum
{
  AX,
  BX,
  CX,
  DX,
  DESCONOCIDO
} t_registro;

typedef enum
{
  DISCO,
  TECLADO,
  PANTALLA
} t_IO;


//Utils del cliente
typedef struct
{
	uint32_t size; // Tamaño del payload
	void* stream;  // Payload
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct 
{
    t_instCode instCode;
    uint32_t paramInt;
	t_IO paramIO;
	t_registro paramReg[2]; 
} t_instruccion;  //__attribute__((packed)) 


typedef struct 
{
	char** segmento;
	uint32_t segmentos_size;
} t_segmento;

typedef struct 
{	
	t_list* instrucciones;
	uint32_t instrucciones_size;
	t_list* segmentos;
	uint32_t segmentos_size;
} t_informacion;

int size_char_array(char**) ;
typedef struct
{
    uint8_t id;
    //t_list instrucciones;
    uint8_t program_counter;
    uint8_t registro_CPU;
    //t_list segmentos;

} t_pcb;

enum tipo_mensaje{
    PCB
};

typedef enum
{
	AGREGAR_PCB,
	ELIMINAR_PCB
}t_cod_planificador;


int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_buffer* cargar_buffer_a_t_pcb(t_pcb t_pcb);
void cargar_buffer_a_paquete(t_buffer* buffer, int conexion);
t_pcb* deserializar_pcb(t_buffer* buffer); 
void deserializar_paquete (int conexion);
//Utils del servidor


extern t_log* logger;
extern t_cod_planificador* cod_planificador;
//extern t_log* loggerKernel;

int iniciar_servidor(char* , char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void* recibir_buffer(int*, int);


t_pcb* crear_pcb();

// LISTAS
extern t_list* LISTA_NEW;
extern t_list* LISTA_READY;
extern t_list* LISTA_EXEC;
extern t_list* LISTA_BLOCKED;
extern t_list* LISTA_EXIT;
extern t_list* LISTA_SOCKETS;


// MUTEX
extern pthread_mutex_t mutex_creacion_ID;
extern pthread_mutex_t mutex_lista_new;
extern pthread_mutex_t mutex_lista_ready;
extern pthread_mutex_t mutex_lista_exec;
extern pthread_mutex_t mutex_lista_blocked;
extern pthread_mutex_t mutex_lista_exit;


// SEMAFOROS
extern sem_t sem_planif_largo_plazo;
extern sem_t contador_multiprogramacion;
extern sem_t sem_ready;
extern sem_t sem_bloqueo;
extern sem_t sem_procesador;

#endif /* UTILS_H_ */