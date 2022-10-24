#include "server.h"

void conectar_y_mostrar_mensajes_de_cliente(char *IP, char *PUERTO, t_log *logger)
{

	int server_fd = iniciar_servidor(IP, PUERTO); // socket(), bind()listen()
	log_info(logger, "Servidor listo para recibir al cliente");

	crear_hilos(server_fd);
}

int crear_hilos(int server_fd)
{

	while (1)
	{

		// esto se podria cambiar como int* cliente_fd= malloc(sizeof(int)); si lo ponemos, va antes del while
		int cliente_fd = esperar_cliente(server_fd);
		// aca hay un log que dice que se conecto un cliente
		log_info(logger, "consola conectada, paso a crear el hilo");

		pthread_t thr1;

		pthread_create(&thr1, NULL, (void *)mostrar_mensajes_del_cliente, cliente_fd);

		pthread_detach(&thr1);
	}
	return EXIT_SUCCESS;
}

void mostrar_mensajes_del_cliente(int cliente_fd)
{

	t_list *lista;
	int y = 1;
	while (y)
	{
		int cod_op = recibir_operacion(cliente_fd);

		switch (cod_op)
		{
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			log_info(logger, "Me llegaron los siguientes valores:");
			list_iterate(lista, (void *)iterator);
			break;
		case NEW:
			log_info(logger, "Llegaron las instrucciones y los segmentos");

			t_informacion info = recibir_informacion(cliente_fd);

			enviarResultado(cliente_fd, "Quedate tranqui Consola, llego todo lo que mandaste ;)\n");

			t_pcb *pcb = crear_pcb(&info, cliente_fd);

			pasar_a_new(pcb);
			log_debug(logger, "Estado Actual: NEW , proceso id: %d" ,pcb->id);

			printf("Cant de elementos de new: %d\n", list_size(LISTA_NEW));

			sem_post(&sem_hay_pcb_lista_new);
			sem_post(&sem_planif_largo_plazo);
			sem_post(&sem_agregar_pcb);

			break;

		case -1:
			/*while(cod_op_servidor =! -1){
				close(socket_cliente);
			}*/
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			y = 0;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

t_informacion recibir_informacion(cliente_fd)
{
	int size;
	void *buffer = recibir_buffer(&size, cliente_fd);
	t_informacion programa;
	int offset = 0;

	memcpy(&(programa.instrucciones_size), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(programa.segmentos_size), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	programa.instrucciones = list_create();
	t_instruccion *instruccion;

	programa.segmentos = list_create();
	uint32_t segmento;

	int k = 0;
	int l = 0;

	while (k < (programa.instrucciones_size))
	{
		instruccion = malloc(sizeof(t_instruccion));
		memcpy(instruccion, buffer + offset, sizeof(t_instruccion));
		offset += sizeof(t_instruccion);
		list_add(programa.instrucciones, instruccion);
		k++;
	}

	printf("Instrucciones:");
	for (int i = 0; i < programa.instrucciones_size; ++i)
	{
		t_instruccion *instruccion = list_get(programa.instrucciones, i);

		printf("\ninstCode: %d, Num: %d, RegCPU[0]: %d,RegCPU[1] %d, dispIO: %d",
			   instruccion->instCode, instruccion->paramInt, instruccion->paramReg[0], instruccion->paramReg[1], instruccion->paramIO);
	}

	while (l < (programa.segmentos_size))
	{
		// segmento = malloc(sizeof(uint32_t));
		memcpy(&segmento, buffer + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		list_add(programa.segmentos, segmento);
		l++;
	}

	printf("\n\nSegmentos:");

	printf("\n[%d,%d,%d,%d]\n", list_get(programa.segmentos, 0), list_get(programa.segmentos, 1), list_get(programa.segmentos, 2), list_get(programa.segmentos, 3));

	free(buffer);

	return programa;
}

void iterator(char *value)
{

	log_info(logger, "%s", value);
}

void planifLargoPlazo()
{

	sem_wait(&sem_planif_largo_plazo);
	sem_wait(&sem_agregar_pcb);
	printf("\nEntrando al planificador\n");
    log_info(logger,"");
	agregar_pcb();


	//sem_wait(&sem_eliminar_pcb)
	//eliminar_pcb();

    /*sem_wait(&sem_eliminar_pcb);
	eliminar_pcb();*/

}

void planifCortoPlazo()
{
	sem_wait(&sem_hay_pcb_lista_ready);
	printf("\nllego pcb a plani corto plazo\n");
	t_tipo_algoritmo algoritmo = obtenerAlgoritmo();
	
	sem_wait(&contador_pcb_running);
	
	switch (algoritmo)
	{
	case FIFO:
		log_debug(logger, "Implementando algoritmo FIFO");
		implementar_fifo();
		break;
	case RR:
		log_debug(logger, "Implementando algoritmo RR");
		implementar_rr();
		break;
	case FEEDBACK:
		log_debug(logger, "Implementando algoritmo FEEDBACK");
		/* code */
		break;

	default:
		break;
	}
}

void pasar_a_new(t_pcb *pcb)
{
	pthread_mutex_lock(&mutex_lista_new);
	list_add(LISTA_NEW, pcb);
	pthread_mutex_unlock(&mutex_lista_new);

	log_debug(logger, "Paso a NEW el proceso %d", pcb->id);
	
}

void pasar_a_ready(t_pcb *pcb) //Para 
{
	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
	pthread_mutex_unlock(&mutex_lista_ready);

	log_debug(logger, "Paso a READY el proceso %d", pcb->id);
}

void pasar_a_ready(t_pcb *pcb)
{
	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
	pthread_mutex_unlock(&mutex_lista_ready);

	log_debug(logger, "Paso a READY el proceso %d", pcb->id);
}

void pasar_a_exec(t_pcb *pcb)
{
	pthread_mutex_lock(&mutex_lista_exec);
	list_add(LISTA_EXEC, pcb);
	pthread_mutex_unlock(&mutex_lista_exec);

	log_debug(logger, "Paso a EXEC el proceso %d", pcb->id);
}

void pasar_a_block(t_pcb *pcb)
{
	pthread_mutex_lock(&mutex_lista_blocked);
	list_add(LISTA_BLOCKED, pcb);
	pthread_mutex_unlock(&mutex_lista_blocked);

	log_debug(logger, "Paso a BLOCK el proceso %d", pcb->id);
}

void pasar_a_exit(t_pcb *pcb)
{
	pthread_mutex_lock(&mutex_lista_exit);
	list_add(LISTA_EXIT, pcb);
	pthread_mutex_unlock(&mutex_lista_exit);

	log_debug(logger, "Paso a EXIT el proceso %d", pcb->id);
}

void iniciar_listas_y_semaforos()
{
	// listas
	LISTA_NEW = list_create();
	LISTA_READY = list_create();
	LISTA_EXEC = list_create();
	LISTA_BLOCKED = list_create();
	LISTA_SOCKETS = list_create();
	LISTA_EXIT = list_create();

	// mutex
	pthread_mutex_init(&mutex_creacion_ID, NULL);
	pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_ready, NULL);
	pthread_mutex_init(&mutex_lista_exec, NULL);
	pthread_mutex_init(&mutex_lista_blocked, NULL);

	// semaforos
	sem_init(&sem_ready, 0, 0);
	sem_init(&sem_bloqueo, 0, 0);
	sem_init(&sem_planif_largo_plazo, 0, 0);

	sem_init(&sem_hay_pcb_lista_new, 0, 0);
	sem_init(&sem_hay_pcb_lista_ready, 0, 0);
	sem_init(&sem_agregar_pcb, 0, 0);
	sem_init(&sem_eliminar_pcb, 0, 0);
	sem_init(&sem_pasar_pcb_running,0,0);
	sem_init(&sem_timer,0,0);
	sem_init(&sem_desalojar_pcb, 0, 0);
	sem_init(&sem_kill_trhread, 0, 0);
	sem_init(&contador_multiprogramacion, 0, configKernel.gradoMultiprogramacion);
	sem_init(&contador_pcb_running, 0, 1);
	
	
}

void agregar_pcb()
{

	while (1)
	{
		sem_wait(&sem_hay_pcb_lista_new);
		sem_wait(&contador_multiprogramacion);

		printf("Agregando un pcb a lista ready");

		pthread_mutex_lock(&mutex_lista_new);
		t_pcb *pcb = algoritmo_fifo(LISTA_NEW);
		printf("Cant de elementos de new: %d\n", list_size(LISTA_NEW));
		pthread_mutex_unlock(&mutex_lista_new);

		pasar_a_ready(pcb);

        log_debug(logger, "Estado Anterior: NEW , proceso id: %d", pcb->id);
	    log_debug(logger, "Estado Actual: READY , proceso id: %d", pcb->id);

		printf("Cant de elementos de ready: %d\n", list_size(LISTA_READY));

		sem_post(&sem_hay_pcb_lista_ready);

		// enviar_mensaje("hola  memoria, inicializa las estructuras", conexionMemoria);
	}
}

void eliminar_pcb()
{

	pthread_mutex_lock(&mutex_lista_exec);
	t_pcb *pcb = algoritmo_fifo(LISTA_EXEC);
	pthread_mutex_unlock(&mutex_lista_exec);

	pasar_a_exit(pcb);
	
	log_debug(logger,"Estado Anterior: EXEC , proceso id%d", pcb->id);
	log_debug(logger,"Estado Actual: EXIT , proceso id%d", pcb->id);

	enviar_mensaje("hola  memoria, libera las estructuras", conexionMemoria);
	sem_post(&contador_multiprogramacion);
}

void iteratorInt(int value)
{

	log_info(logger, "Segmento = %d", value);
}

t_pcb *crear_pcb(t_informacion *informacion, int socket)
{
	t_pcb *pcb = malloc(sizeof(t_pcb));

	pcb->socket = socket;
	pcb->program_counter = 0;
	pcb->informacion = informacion;
	pcb->registros.AX = 0;
	pcb->registros.BX = 0;
	pcb->registros.CX = 0;
	pcb->registros.DX = 0;

	pthread_mutex_lock(&mutex_creacion_ID);
	pcb->id = contadorIdPCB;
	contadorIdPCB++;
	pthread_mutex_unlock(&mutex_creacion_ID);

	return pcb;
}

t_tipo_algoritmo obtenerAlgoritmo()
{

	char *algoritmo = configKernel.algoritmo;

	
	t_tipo_algoritmo algoritmoResultado;

	if (!strcmp(algoritmo,"FIFO"))
	{
		algoritmoResultado = FIFO;
	}
	else if (!strcmp(algoritmo,"RR"))
	{
		algoritmoResultado = RR;
	}
	else
	{
		algoritmoResultado = FEEDBACK;
	}

	return algoritmoResultado;
}

t_pcb *algoritmo_fifo(t_list *lista)
{
	t_pcb *pcb = (t_pcb *)list_remove(lista, 0);
	return pcb;
}


void algoritmo_feedback()
{
	
}

void implementar_fifo()
{

	t_pcb *pcb = algoritmo_fifo(LISTA_READY);
	printf("\nAgregando UN pcb a lista exec");
	pasar_a_exec(pcb);
	printf("\nCant de elementos de exec: %d\n", list_size(LISTA_EXEC));

	log_debug(logger, "Estado Anterior: READY , proceso id: %d", pcb->id);
	log_debug(logger,"Estado Actual: EXEC , proceso id: %d", pcb->id);

	sem_post(&sem_pasar_pcb_running);
}


void implementar_rr(){
	t_pcb *pcb = algoritmo_fifo(LISTA_READY);
	pthread_t thrTimer;

	pthread_create(&thrTimer, NULL, (void *)hilo_timer, NULL);
	printf("\nAgregando UN pcb a lista exec rr");
	pasar_a_exec(pcb);
	printf("\nCant de elementos de exec: %d\n", list_size(LISTA_EXEC));

	sem_post(&sem_pasar_pcb_running);
	
	sem_post(&sem_timer);
	
	log_debug(logger, "Estado Anterior: READY , proceso id: %d", pcb->id);
	log_debug(logger, "Estado Actual: EXEC , proceso id: %d", pcb->id);

	
	pthread_detach(&thrTimer);
	sem_wait(&sem_kill_trhread);
	pthread_cancel(thrTimer);

}

void hilo_timer(){
	sem_wait(&sem_timer);
	printf("\nvoy a dormir, soy el timer\n");
	usleep(configKernel.quantum);
	printf("\nme desperte!\n");
	sem_post(&sem_desalojar_pcb);

	printf("\nenvie post desalojar pcb\n");
}