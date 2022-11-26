#include "memoria.h"

int main(int argc, char **argv)
{

	// Parte Server
	logger = iniciar_logger("memoria.log", "MEMORIA", LOG_LEVEL_DEBUG);

	config = iniciar_config("memoria.config");

	// creo el struct
	extraerDatosConfig(config);

	FILE *swap = abrirArchivo(configMemoria.pathSwap);

	fclose(swap);

	iniciar_listas_y_semaforos();

	contadorIdTablaPag = 0;

	crear_hilos_memoria();

	log_destroy(logger);

	config_destroy(config);

	// crear una lista con el tablaño de los marcos/segmanetos para ir guardado y remplazando
	// en el caso de que esten ocupados , con los algoritmos correcpondientes

	// en elarchivo swap se van a guardar las tablas enteras que voy a leer segun en los bytes que esten
	// lo voy a buscar con el fseeck y ahi agregar , reemplazar , los tatos quedan ahi es como disco
}

t_configMemoria extraerDatosConfig(t_config *archivoConfig)
{
	configMemoria.puertoEscuchaUno = string_new();
	configMemoria.puertoEscuchaDos = string_new();
	configMemoria.algoritmoReemplazo = string_new();
	configMemoria.pathSwap = string_new();

	configMemoria.puertoEscuchaUno = config_get_string_value(archivoConfig, "PUERTO_ESCUCHA_UNO");
	configMemoria.puertoEscuchaDos = config_get_string_value(archivoConfig, "PUERTO_ESCUCHA_DOS");
	configMemoria.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEMORIA");
	configMemoria.algoritmoReemplazo = config_get_string_value(archivoConfig, "ALGORITMO_REEMPLAZO");
	configMemoria.pathSwap = config_get_string_value(archivoConfig, "PATH_SWAP");

	configMemoria.tamMemoria = config_get_int_value(archivoConfig, "TAM_MEMORIA");
	configMemoria.tamPagina = config_get_int_value(archivoConfig, "TAM_PAGINA");
	configMemoria.entradasPorTabla = config_get_int_value(archivoConfig, "ENTRADAS_POR_TABLA");
	configMemoria.marcosPorProceso = config_get_int_value(archivoConfig, "MARCOS_POR_PROCESO");
	configMemoria.retardoSwap = config_get_int_value(archivoConfig, "RETARDO_SWAP");
	configMemoria.tamanioSwap = config_get_int_value(archivoConfig, "TAMANIO_SWAP");

	return configMemoria;
}

void crear_hilos_memoria()
{
	pthread_t thrKernel, thrCpu;

	pthread_create(&thrKernel, NULL, (void *)iniciar_servidor_hacia_kernel, NULL);
	pthread_create(&thrCpu, NULL, (void *)iniciar_servidor_hacia_cpu, NULL);

	pthread_detach(thrKernel);
	pthread_join(thrCpu, NULL);
}

void iniciar_servidor_hacia_kernel()
{
	int server_fd = iniciar_servidor(IP_SERVER, configMemoria.puertoEscuchaUno);
	log_info(logger, "Servidor listo para recibir al kernel");
	socketAceptadoKernel = esperar_cliente(server_fd);
	char *mensaje = recibirMensaje(socketAceptadoKernel);

	log_info(logger, "Mensaje de confirmacion del Kernel: %s\n", mensaje);

	while (1)
	{
		t_paqueteActual *paquete = recibirPaquete(socketAceptadoKernel);
		printf("\nRecibi el paquete del kernel%d\n", paquete->codigo_operacion);
		t_pcb *pcb = deserializoPCB(paquete->buffer);
		switch (paquete->codigo_operacion)
		{
		case ASIGNAR_RECURSOS:
			printf("\nMI cod de op es: %d", paquete->codigo_operacion);
			pthread_t thrTablaPaginasCrear;
			printf("\nEntro a asignar recursos\n");
			pthread_create(&thrTablaPaginasCrear, NULL, (void *)crearTablasPaginas, (void *)pcb);
			pthread_detach(thrTablaPaginasCrear);
			break;

		case LIBERAR_RECURSOS:
			pthread_t thrTablaPaginasEliminar;

			pthread_create(&thrTablaPaginasEliminar, NULL, (void *)eliminarTablasPaginas, (void *)pcb);
			pthread_detach(thrTablaPaginasEliminar);
			break;
		case PASAR_A_EXIT: //solicitud de liberar las estructuras

			//liberar las estructuras y
			//enviar msj al kernel de que ya estan liberadas
			//serializarPCB(socketAceptadoKernel, pcb, PASAR_A_EXIT);
			break;
		}
	}
}

void iniciar_servidor_hacia_cpu()
{

	int server_fd = iniciar_servidor(IP_SERVER, configMemoria.puertoEscuchaDos); // socket(), bind()listen()
	
	log_info(logger, "Servidor listo para recibir al cpu");

	int socketAceptadoCPU = esperar_cliente(server_fd);
	char *mensaje = recibirMensaje(socketAceptadoCPU);

	log_info(logger, "Mensaje de confirmacion del CPU: %s\n", mensaje);

	t_paqt paqueteCPU;
	recibirMsje(socketAceptadoCPU, &paqueteCPU);

	if(paqueteCPU.header.cliente == CPU){
		
		log_debug(logger,"HANSHAKE se conecto CPU");

		conexionCPU(socketAceptadoCPU);
	}
    mostrar_mensajes_del_cliente(socketAceptadoCPU);

}

void conexionCPU(int socketAceptado){ // void*

	t_paqt paquete;

	int pid;
	int pagina;
	int idTablaPagina;
	t_direccionFisica* direccionFisica;

	MSJ_MEMORIA_CPU_ACCESO_TABLA_DE_PAGINAS* infoMemoriaCpuTP;
	MSJ_MEMORIA_CPU_LEER* infoMemoriaCpuLeer;

	int y = 1;
	while(y){
		direccionFisica = malloc(sizeof(t_direccionFisica));
		infoMemoriaCpuTP = malloc(sizeof(MSJ_MEMORIA_CPU_ACCESO_TABLA_DE_PAGINAS));
		infoMemoriaCpuLeer = malloc(sizeof(MSJ_MEMORIA_CPU_LEER));

		recibirMsje(socketAceptado, &paquete);

		switch(paquete.header.tipoMensaje) {
			case CONFIG_DIR_LOG_A_FISICA:
				configurarDireccionesCPU(socketAceptado);
				break;
			case ACCESO_MEMORIA_TABLA_DE_PAG:
				infoMemoriaCpuTP = paquete.mensaje;
				idTablaPagina = infoMemoriaCpuTP->idTablaDePaginas;
				pagina = infoMemoriaCpuTP->pagina;
				accesoMemoriaTP(idTablaPagina, pagina, socketAceptado);
				break;
			case ACCESO_MEMORIA_LEER:
				infoMemoriaCpuLeer = paquete.mensaje;
				direccionFisica->nroMarco = infoMemoriaCpuLeer->nroMarco;
				direccionFisica->desplazamientoPagina = infoMemoriaCpuLeer->desplazamiento;
				pid = infoMemoriaCpuLeer->pid;
				//accesoMemorialeer(direccionFisica, pid, socketAceptado);
				break;
			default: // TODO CHEKEAR: SI FINALIZO EL CPU ANTES QUE MEMORIA, SE PRODUCE UNA CATARATA DE LOGS. PORQUE? NO HAY PORQUE
				log_error(logger, "No se reconoce el tipo de mensaje, tas metiendo la patita");
				break;
		}

	}
}

void configurarDireccionesCPU(int socketAceptado){
	//SE ENVIAN LAS ENTRADAS_POR_TABLA y TAM_PAGINA AL CPU PARA PODER HACER LA TRADUCCION EN EL MMU
	log_debug(logger,"Se envian las ENTRADAS_POR_TABLA y TAM_PAGINA al CPU ");

	MSJ_MEMORIA_CPU_INIT* infoAcpu = malloc(sizeof(MSJ_MEMORIA_CPU_INIT));

	infoAcpu->cantEntradasPorTabla = configMemoria.entradasPorTabla;

	infoAcpu->tamanioPagina = configMemoria.tamPagina;


	//usleep(configMemoria.retardoMemoria * 1000); // CHEQUEAR, SI LO DESCOMENTAS NO PASA POR LAS OTRAS LINEAS

	enviarMsje(socketAceptado, MEMORIA, infoAcpu, sizeof(MSJ_MEMORIA_CPU_INIT), CONFIG_DIR_LOG_A_FISICA);

	free(infoAcpu);

	log_debug(logger,"Informacion de la cantidad de entradas por tabla y tamaño pagina enviada al CPU");
}

void crearTablasPaginas(void *pcb)
{
	t_pcb *pcbActual = (t_pcb *)pcb;
	for (int i = 0; i < list_size(pcbActual->tablaSegmentos); i++)
	{
		t_tabla_paginas *tablaPagina = malloc(sizeof(t_tabla_paginas));
		t_tabla_segmentos *tablaSegmento = list_get(pcbActual->tablaSegmentos, i);

		tablaPagina->paginas = list_create();
		pthread_mutex_lock(&mutex_creacion_ID_tabla);
		tablaSegmento->indiceTablaPaginas = contadorIdTablaPag;
		tablaPagina->idTablaPag = contadorIdTablaPag;
		contadorIdTablaPag++;
		pthread_mutex_unlock(&mutex_creacion_ID_tabla);

		tablaPagina->idPCB = pcbActual->id;

		for (int i = 0; i < configMemoria.entradasPorTabla; i++)
		{

			t_pagina *pagina = malloc(sizeof(t_pagina));

			pagina->modificacion = 0;
			pagina->presencia = 0;
			pagina->uso = 0;
			pagina->nroMarco = 0;
			pagina->nroPagina = i;

			list_add(tablaPagina->paginas, pagina);
		}
		printf("\n  estoy agregando tabla a la lista ");
		agregar_tabla_paginas(tablaPagina);

	}

	printf("\nEnvio recursos a kernel\n");
	serializarPCB(socketAceptadoKernel, pcbActual, ASIGNAR_RECURSOS);
	printf("\nEnviados\n");
	free(pcbActual);
}

void eliminarTablasPaginas(void *pcb)
{
	t_pcb *pcbActual = (t_pcb *)pcb;

	// eliminar los recurso de swap
}

FILE *abrirArchivo(char *filename)
{
	if (filename == NULL)
	{
		log_error(logger, "Error: debe informar un path correcto.");
		exit(1);
	}

	truncate(filename, configMemoria.tamanioSwap);
	return fopen(filename, "w+");
}
void agregar_tabla_paginas(t_tabla_paginas *tablaPaginas)
{
	pthread_mutex_lock(&mutex_lista_tabla_paginas);
	list_add(LISTA_TABLA_PAGINAS, tablaPaginas);
	pthread_mutex_unlock(&mutex_lista_tabla_paginas);
}


void accesoMemoriaLeer(t_direccionFisica* df, int pid, int socketAceptado){
	log_debug(logger,"[INIT - ACCESO_MEMORIA_READ] DIR_FISICA: %d%d",
				df->nroMarco, df->desplazamientoPagina);

	int nroFrame = df->nroMarco;
	int desplazamiento = df->desplazamientoPagina;
	int tamanioFrame = configMemoria.tamPagina;  // Cambiar cuando este resuelta la inicializacion de estructuras. Ambos pueden ser variables definidas en utils
	int cantidadTotalDeFrames = 512; // 512 nro de ejemplo, cambiar cuando esten inicializadas las estructuras . Ambos pueden ser variables definidas en utils
	void* memoriaRAM; // sacarlo cuando se definan las estructuras
	void* aLeer = malloc(tamanioFrame-desplazamiento);
	int valorLeido;
	MSJ_STRING* msjeError;

	//valido que el offset sea valido
	if(desplazamiento>tamanioFrame){
		usleep(configMemoria.retardoMemoria * 1000);
		msjeError = malloc(sizeof(MSJ_STRING));
		string_append(&msjeError->cadena, "ERROR_DESPLAZAMIENTO");
		enviarMsje(socketAceptado, MEMORIA, msjeError, sizeof(MSJ_STRING), ACCESO_MEMORIA_LEER);
		free(msjeError);
		log_error(logger,"[ACCESO_MEMORIA_READ] OFFSET MAYOR AL TAMANIO DEL FRAME.  DIR_FISICA: %d%d",
					df->nroMarco, df->desplazamientoPagina);
		return;
	}

	//valido que el nro frame sea valido
	if(nroFrame>cantidadTotalDeFrames){
		usleep(configMemoria.retardoMemoria * 1000);
		msjeError = malloc(sizeof(MSJ_STRING));
		string_append(&msjeError->cadena, "ERROR_NRO_FRAME");
		enviarMsje(socketAceptado, MEMORIA, msjeError, sizeof(MSJ_STRING), ACCESO_MEMORIA_LEER);
		free(msjeError);
		log_error(logger,"[ACCESO_MEMORIA_READ] NRO DE FRAME INEXISTENTE.  DIR_FISICA: %d%d",
					df->nroMarco, df->desplazamientoPagina);
		return;
	}

	pthread_mutex_lock(&mutex_void_memoria_ram);
		memcpy(aLeer, memoriaRAM+(nroFrame*tamanioFrame)+desplazamiento, tamanioFrame-desplazamiento);
	pthread_mutex_unlock(&mutex_void_memoria_ram);
	char** cosa2 = string_array_new();
	cosa2 = string_split((char*)aLeer, "*");
	char* leidoStringArray = string_new();
	int size =string_array_size(cosa2) -1;
	for(int i = 0; i < size; i++){
		string_append(&leidoStringArray, cosa2[i]);
	}
	valorLeido = atoi(leidoStringArray);

	log_debug(logger, "Valor Leido: %s", leidoStringArray);

	usleep(configMemoria.retardoMemoria * 1000);

	/***********************************************/
	t_tabla_paginas* tablaPaginas;
	t_pagina* pagina;
	bool update = false;
	pthread_mutex_lock(&mutex_lista_tabla_paginas);
		for(int i=0; i<list_size(LISTA_TABLA_PAGINAS) && !update; i++){
			tablaPaginas = list_get(LISTA_TABLA_PAGINAS, i);
			for(int j=0; j<list_size(tablaPaginas->paginas) && !update; j++){
				pagina = list_get(tablaPaginas->paginas, j);
				if(pagina->nroMarco == nroFrame){
					pagina->uso = 1;
					update = true;
				}
			}
		}
	pthread_mutex_unlock(&mutex_lista_tabla_paginas);

	/***********************************************/
	MSJ_INT* mensajeRead = malloc(sizeof(MSJ_INT));
	mensajeRead->numero = valorLeido;
	enviarMsje(socketAceptado, MEMORIA, mensajeRead, sizeof(MSJ_INT), ACCESO_MEMORIA_LEER);
	free(leidoStringArray);
	free(cosa2);
	free(mensajeRead);

	log_debug(logger,"ACCESO_MEMORIA_READ DIR_FISICA: frame%d offset%d",
				df->nroMarco, df->desplazamientoPagina);
}

/*Acceso a tabla de páginas
El módulo deberá responder el número de marco correspondiente, en caso de no encontrarse, 
se deberá retornar Page Fault.*/

void accesoMemoriaTP(int idTablaPagina, int nroPagina, int socketAceptado){
	//CPU SOLICITA CUAL ES EL MARCO DONDE ESTA LA PAGINA DE ESA TABLA DE PAGINA
	log_debug(logger,"ACCEDIENDO A TABLA DE PAGINA ID: %d NRO_PAGINA: %d",
				idTablaPagina, nroPagina);

	int marcoBuscado;
	t_pagina* pagina;
	t_tabla_paginas* tabla_de_paginas;
	int indice;
	bool corte = false;
	//busco la pagina que piden
	pthread_mutex_lock(&mutex_lista_tabla_paginas);
		for(int i=0; i<list_size(LISTA_TABLA_PAGINAS)&& !corte; i++){
			tabla_de_paginas = list_get(LISTA_TABLA_PAGINAS, i);
			if(tabla_de_paginas->idTablaPag == idTablaPagina){
				for(int j=0; j<list_size(tabla_de_paginas->paginas) && !corte; j++){
					pagina = list_get(tabla_de_paginas->paginas, j);
					if(pagina->nroPagina == nroPagina){
						log_debug(logger, "BUSCO PAGINA %d", pagina->nroPagina);
						indice = j;
						corte = true;
					}
				}
			}
		}
	pthread_mutex_unlock(&mutex_lista_tabla_paginas);

	if(corte==true){ // REVISAR
	pthread_mutex_unlock(&mutex_lista_tabla_paginas);
	marcoBuscado = pagina->nroMarco;
	log_debug(logger,"[ACCESO_TABLA_PAGINAS] LA PAGINA ESTA EN RAM");
	
	MSJ_INT* mensaje = malloc(sizeof(MSJ_INT));
	mensaje->numero = marcoBuscado;
	
	enviarMsje(socketAceptado, MEMORIA, mensaje, sizeof(MSJ_INT), RESPUESTA_MEMORIA_MARCO_BUSCADO);
	free(mensaje);
	log_debug(logger,"[FIN - TRADUCCION_DIR_SEGUNDO_PASO] FRAME BUSCADO = %d ,DE LA PAGINA: %d DE TABLA 2DO NIVEL: %d ENVIADO A CPU",
				marcoBuscado, pagina, idTablaPagina);
	}
	else{ //la pag no esta en ram. Retornar PAGE FAULT
		
		MSJ_INT* mensaje = malloc(sizeof(MSJ_INT));
		mensaje->numero = PAGE_FAULT;
		enviarMsje(socketAceptado, MEMORIA, mensaje, sizeof(MSJ_INT), PAGE_FAULT);
		free(mensaje);
		log_debug(logger,"PAGE FAULT");
	}
}
