#include "cpu.h"

int main(char **argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-test") == 0)
		return run_tests();
	else
	{

		logger = iniciar_logger("cpu.log", "CPU", LOG_LEVEL_DEBUG);

		config = iniciar_config("cpu.config");

		extraerDatosConfig(config);

		pthread_t thrDispatchKernel, thrInterruptKernel, thrMemoria;

		pthread_create(&thrDispatchKernel, NULL, (void *)iniciar_servidor_dispatch, NULL);
		pthread_create(&thrInterruptKernel, NULL, (void *)iniciar_servidor_interrupt, NULL);
		pthread_create(&thrMemoria, NULL, (void *)conectar_memoria, NULL);

		pthread_join(thrDispatchKernel, NULL);
		pthread_join(thrInterruptKernel, NULL);
		pthread_join(thrMemoria, NULL);

		log_destroy(logger);
		config_destroy(config);
	}
}



t_configCPU extraerDatosConfig(t_config *archivoConfig)
{

	configCPU.reemplazoTLB = string_new();
	configCPU.ipMemoria = string_new();
	configCPU.puertoMemoria = string_new();
	configCPU.puertoEscuchaDispatch = string_new();
	configCPU.puertoEscuchaInterrupt = string_new();

	configCPU.ipCPU = config_get_string_value(archivoConfig, "IP_CPU");

	configCPU.ipMemoria = config_get_string_value(archivoConfig, "IP_MEMORIA");
	configCPU.puertoMemoria = config_get_string_value(archivoConfig, "PUERTO_MEMORIA");
	configCPU.reemplazoTLB = config_get_string_value(archivoConfig, "REEMPLAZO_TLB");
	configCPU.puertoEscuchaDispatch = config_get_string_value(archivoConfig, "PUERTO_ESCUCHA_DISPATCH");
	configCPU.puertoEscuchaInterrupt = config_get_string_value(archivoConfig, "PUERTO_ESCUCHA_INTERRUPT");
	configCPU.retardoInstruccion = config_get_int_value(archivoConfig, "RETARDO_INSTRUCCION");
	configCPU.entradasTLB = config_get_int_value(archivoConfig, "ENTRADAS_TLB");

	return configCPU;
}

void iniciar_servidor_dispatch()
{
	int server_fd = iniciar_servidor(IP_SERVER, configCPU.puertoEscuchaDispatch); // socket(), bind()listen()
	log_info(logger, "Servidor listo para recibir al dispatch kernel");

	int cliente_fd = esperar_cliente(server_fd);
	
	while(1){
		t_paquete *paquete = recibirPaquete(cliente_fd);
		if (paquete == NULL) {
			continue;
		}
		t_pcb *pcb = deserializoPCB(paquete->buffer);
		// free(paquete->buffer->stream);
		// free(paquete->buffer);
		// free(paquete);

		printf("se recibio pcb de running de kernel\n");

		printf("\n%d.\n", pcb->id);
		printf("\n%d.\n", pcb->program_counter);

		imprimirInstruccionesYSegmentos(pcb->informacion);
	
		printf("\n%d.\n", pcb->socket);

		printf("\n%d.\n", pcb->registros.AX);

		cicloInstruccion(pcb);
		

	//hacer cosas
	/*hacer_cosas_con_pcb(

		sem_post(&sem_pasar_pcb_kernel);
	)*/


		//sem_wait(&sem_pasar_pcb_kernel);
		//serializarPCB(conexion, pcb, EXIT_PCB);

	}
}

void iniciar_servidor_interrupt()
{
	int server_fd = iniciar_servidor(IP_SERVER, configCPU.puertoEscuchaInterrupt);
	log_info(logger, "Servidor listo para recibir al interrupt kernel");

	int cliente_fd = esperar_cliente(server_fd);
	mostrar_mensajes_del_cliente(cliente_fd);
}

void conectar_memoria()
{
	conexion = crear_conexion(configCPU.ipMemoria, configCPU.puertoMemoria);
	enviar_mensaje("hola memoria, soy el cpu", conexion);
}

void cicloInstruccion(t_pcb* pcb) {
	
	t_instruccion* insActual = list_get(pcb->informacion->instrucciones, pcb->program_counter); 
	log_info(logger,"insActual->instCode: %i", insActual->instCode);
	
	// fetch
	fetch(pcb);
	
	// decode
	if(insActual->instCode == MOV_IN || insActual->instCode == MOV_OUT) {
		log_debug(logger, "Requiere acceso a Memoria");
		//Hacer algo en proximo Checkpoint
	}

	//execute
	log_debug(logger, "Instrucción Ejecutada: 'PID:  %i - Ejecutando: %i %i %i'", 
			pcb->id, insActual->instCode, insActual->paramReg[0], insActual->paramInt); //log minimo y obligatorio
	
	//bool retornePCB = false;
	switch(insActual->instCode){
		case SET:
			log_debug(logger,"SET");
			usleep(configCPU.retardoInstruccion);
			switch (insActual->paramReg[0])
			{
			case AX:
				pcb->registros.AX = insActual->paramInt;
				break;
			case BX:
				pcb->registros.BX = insActual->paramInt;
			case CX:
				pcb->registros.CX = insActual->paramInt;
			case DX:
				pcb->registros.DX = insActual->paramInt;
			default:
				break;
			}
			char* string = string_new();
        	string_append(&string, devolverString(insActual->paramReg[0]));
        	
			log_debug(logger, "registro CPU %s valor a asignar  = %i",devolverString(insActual->paramReg[0]), insActual->paramInt);
			free(string);
			break;

	}
	
}

void fetch(t_pcb* pcb){
	
	uint32_t index = pcb->program_counter;
	pcb->program_counter += 1;

	log_info(logger,"insActual->pc: %i", index);
	log_info(logger," Valor nuevo Program counter: %i", pcb->program_counter);

}

char* devolverString(t_registro registroCPU){
	switch (registroCPU)
	{
	case AX:
		return "AX";
		break;
	case BX:
		return "BX";
		break;
	case CX:
		return "CX";
		break;
	case DX:
		return "DX";
		break;	
	default:
		break;
	}
}



