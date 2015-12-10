#ifndef SCPFUNC_H_INCLUDED
#define SCPFUNC_H_INCLUDED



/**
 * Inserta un evento en la lista de eventos a procesar por el simulador.
 * @param eventos Lista de eventos del simulador
 * @param e Evento nuevo a ingresar
 */
void insertarEvento(EVENTOS * eventos, EVENTOS * e)
{
	EVENTOS act=*eventos,ant=NULL;

	while (act!=NULL && act->instante <= (*e)->instante)
	{
		ant=act;
		act = act->sig;
	}
	//El primero nunca va a ser porque ya estan cargados los arribos.

	ant->sig=*e;
	(*e)->sig = act;

}

/**
 * Mueve un proceso de una cola de origen hacia una cola de destino.
 * Dentro de las colas de procesos tanto de origen como de destino
 * tambien se tiene en cuenta al cpu como una cola de un único
 * proceso.
 * @param destino Cola destino a la que hay que agregar el proceso
 * @param origen Cola origen de la que hay que quitar el proceso
 * @param pid En el caso que el pid sea "" (una cadena vacía) moverá de origen el primer elemento, de lo contrario buscara el pid y ese es el que moverá
 */
void moverProceso(PROCESOS * destino,PROCESOS *origen, char pid[5])
{
	PROCESOS actD = *destino,ultD=NULL;

	PROCESOS actO = *origen,antO=NULL;

	// Siempre tengo que insertar al final del destino por eso lo recorro hasta el ultimo elemento
	while (actD != NULL)
	{
		ultD = actD;
		actD = actD->sig;
	}

	//Si no hay elementos en la cola de destino
	if (ultD == NULL)
	{
		// Si hay que mover el primer elemento de la cola origen:
		if (!strcmp(pid,""))
		{
			*destino = *origen;
			*origen = (*origen)->sig;
			(*destino)->sig = NULL;
		}
		// Si hay que buscar el elemento de la cola origen:
		else
		{
			// Avanzo con antO y actO en la cola de origen hasta encontrar el proceso
			while(actO != NULL && strcmp(actO->pid,pid))
			{
				antO = actO;
				actO = actO->sig;
			}

			//el primer elemento de la cola destino va a ser el actual de la cola origen
			*destino = actO;
			//Si el elemento de la cola origen no era el único
			if (antO!=NULL)
			{
				//Desvinculo al actual de la cola de origen
				antO->sig = actO->sig;
			}
			// Si el elemento de la cola origen era el único
			else
			{
			    *origen = actO->sig;
			}

			// Como es el unico elemento que quedo en la cola de destino, el sig es NULL
			(*destino)->sig = NULL;
		}
	}
	// Si hay elementos en la cola de destino tengo en ultD el ultimo elemento de destino
	else
	{
		// Si hay que mover el primer elemento de la cola origen:
		if (!strcmp(pid,""))
		{
			ultD->sig = *origen;
			*origen = (*origen)->sig;
		}
		//Si hay que buscar el elemento de la cola origen:
		else
		{
			// Avanzo con antO y actO en la cola de origen hasta encontrar el proceso
			while(actO != NULL && strcmp(actO->pid,pid))
			{
				antO = actO;
				actO = actO->sig;
			}

			//el ultimo elemento de la cola destino va a ser el actual de la cola origen
			ultD->sig = actO;
			//Desvinculo al actual de la cola de origen
			if (antO != NULL)
			{
				antO->sig = actO->sig;
			}
			else
			{
				*origen = (*origen)->sig;
			}
			ultD = ultD->sig;
			ultD->sig = NULL;


		}
	}
}


/**
 * Muestra los PID de los procesos de una lista de procesos.
 * @param procesos Lista de procesos a mostrar
 */
void mostrarProcesos(PROCESOS procesos)
{
	while (procesos != NULL)
	{
		printf("%s ",procesos->pid);
		procesos = procesos->sig;
	}
	printf("\n");
}

/**
 * LLeva adelante la simulación de la ejecución de los procesos de forma concurrente.
 *
 * @param procesos Lista dinámica de los procesos que se van a simular
 * @param scs Lista dinámica de los System Call que se van a simular
 * @param eventos Lista incial de eventos que se van a simular
 * @param pasoapaso Contiene 1 si se debe mostrar paso a paso la simulación, de lo contrario 0 (cero)
 * @param *rp Puntero a una lista de procesos (de salida)
 * @param *re Puntero a una lista de eventos (de salida)
 * @param *rs Puntero a una lista de SYSIND (de salida)
 */

void simular(PROCESOS procesos,SCALLS scs, EVENTOS eventos, int pasoapaso, PROCESOS *rp, EVENTOS *re, SYSIND *rs)
{
	int reloj=0,relojAnt=0;
	char pid[5];
	char antPid[5]="";
	char evento[4];
	char semaforo[3];
	char continua;
	int cpuChanged,SC;

	PROCESOS cpu;
	PROCESOS p,pAct;
	EVENTOS e,ef,efp,auxEventos;

    SCALLS antSC,actSC,aux;

	PROCESOS listos = NULL;
	PROCESOS auxListos=NULL;

	PROCESOS finalizados = NULL;
	PROCESOS auxFin = NULL;

	PROCESOS bloqueadosIO = NULL;
	PROCESOS auxbloqIO = NULL;

	SEMAFORO v;
	SEMAFORO ll;
	SEMAFORO m;
	SEMAFORO * sem;

	PROCESOS auxbloqSYNC;


    /* ESTADO INICIAL */

	v.valor = 1;
	v.bloqueados = NULL;

	ll.valor = 0;
	ll.bloqueados = NULL;

	m.valor = 1;
	m.bloqueados = NULL;

	cpu = NULL; //el procesador esta idle


    // La simulación recorre la lista de eventos ordenados por instante de cada evento.
	while (eventos!=NULL)
	{


        cpuChanged = 0;
        SC = 0;

		reloj = eventos->instante;
		strcpy(pid,eventos->pid);
		strcpy(evento,eventos->codigo);
		strcpy(semaforo,eventos->semaforo);

		// Antes de procesar el evento, le sumamos a los tiempos de espera
		// Los que están en listos suman en Tiempo de Espera en cola de Listos
		auxListos = listos;
		while (auxListos != NULL)
		{
			auxListos->tEListos += reloj - relojAnt;
			auxListos = auxListos->sig;
		}

		// Los que están en bloqueados SYNC suman en Tiempo de Espera por sincronización
		auxbloqSYNC = v.bloqueados;
		while (auxbloqSYNC != NULL)
		{
			auxbloqSYNC->tESync += reloj- relojAnt;
			auxbloqSYNC = auxbloqSYNC->sig;
		}
		auxbloqSYNC = ll.bloqueados;
		while (auxbloqSYNC != NULL)
		{
			auxbloqSYNC->tESync += reloj- relojAnt;
			auxbloqSYNC = auxbloqSYNC->sig;
		}
		auxbloqSYNC = m.bloqueados;
		while (auxbloqSYNC != NULL)
		{
			auxbloqSYNC->tESync += reloj- relojAnt;
			auxbloqSYNC = auxbloqSYNC->sig;
		}

		// Los que están en bloqueados SYNC suman en Tiempo de Espera por sincronización
		auxbloqIO = bloqueadosIO;
		while (auxbloqIO != NULL)
		{
			auxbloqIO->tEDisp += reloj- relojAnt;
			auxbloqIO = auxbloqIO->sig;
		}


		//Procesar el evento
		if (!strcmp(evento,"ANP")) // ARRIBO DE NUEVO PROCESO
		{
			//Busco el proceso en la lista de procesos que se van a simular
			pAct = procesos;
			while (pAct!= NULL && strcmp(pAct->pid,eventos->pid))
			{
				pAct = pAct->sig;
			}
			//Copio el proceso simulando activarlo en el sistema
			p = (PROCESOS)malloc(sizeof(PROCESO));
			strcpy(p->pid,pAct->pid);
			p->tcpu = pAct->tcpu;
			p->necesidad = pAct->necesidad;
			p->arribo = pAct->arribo;
			p->contIos = 0;
			p->turnAround = 0; // Se calcula cuando se finaiza el proceso
			p->tResp = 0; // Se calcula cuando se termina la 1er IO
			p->tEListos = 0; // Se calcula en cada iteracion si el proceso esta en listos
			p->tESync = 0; // Se calcula en cada iteración si el proceso esta en bloq SYNC
			p->tETotal = 0; // Se calcula al final sumando los tiempos de espera
			p->tEDisp = 0; // Se calcula en cada iteración si el proceso esta en bloq IO
			p->sig = NULL;

			// Si la cola de listos esta vacía y no hay nadie en el procesador:
			// asigno al procesador el proceso, de lo contrario lo agrego a listos.
			if (listos == NULL && cpu == NULL)
			{
				moverProceso(&cpu,&p,"");
			}
			else
			{
				moverProceso(&listos,&p,"");
			}
		}
		else if (!strcmp(evento,"SIO")) // SOLICITUD DE ENTRADA SALIDA
		{
			// Envío el proceso que esta en el cpu a la lista de bloqueados por IO
			moverProceso(&bloqueadosIO,&cpu,"");

			// Verifico si hay alguien listo para ir al procesador.
			// Si no hay nadie listo: el cpu queda idle
			if (listos == NULL)
				cpu = NULL;
			else
				moverProceso(&cpu,&listos,"");

		}
		else if (!strcmp(evento,"FIO")) // FIN DE ENTRADA SALIDA
		{

			if (cpu != NULL)
			{
				moverProceso(&listos,&bloqueadosIO,pid);
				// Busco el proceso que acaba de terminar su IO en la cola de listos
				auxListos = listos;
				while (auxListos !=NULL && strcmp(auxListos->pid,pid))
				{
					auxListos = auxListos->sig;
				}
				// Si es el primer FIO entonces calculo el tiempo de respuesta del proceso
				if (++auxListos->contIos == 1)
				{
					auxListos->tResp = reloj - auxListos->arribo;
				}
			}
			else
			{
				moverProceso(&cpu,&bloqueadosIO,pid);
				if (++cpu->contIos == 1)
				{
					cpu->tResp = reloj - cpu->arribo;
				}
			}


		}
		else if (!strcmp(evento,"FPR"))// FIN DE PROCESO
		{
			// Calculo el Turn Around del proceso que finaliza
			cpu->turnAround = reloj - cpu->arribo;

			moverProceso(&finalizados,&cpu,"");

			// Si no hay nadie listo el cpu queda idle.
			if (listos == NULL)
				cpu = NULL;
			else
				moverProceso(&cpu,&listos,"");
		}

		else if (!strcmp(evento,"P")) // OPERACION P SOBRE UN SEMAFORO
		{

		    if (!strcmp(semaforo,"V"))
		    {
                sem = &v;
		    }
		    else if (!strcmp(semaforo,"LL"))
		    {
                sem = &ll;
		    }
		    else if (!strcmp(semaforo,"M"))
		    {
                sem = &m;
                SC = 1;
		    }

		    if (--(*sem).valor < 0)
		    {
		        moverProceso(&(*sem).bloqueados,&cpu,"");
                if (listos != NULL)
                    moverProceso(&cpu,&listos,"");
                else
                    cpu = NULL;
		    }
            else
                // Una operación P puede implicar que el proceso invocante NO se bloqueé; en ese caso vuelve a
                // la cola de listos con estricto ordenamiento FIFO.
                if (listos == NULL)
                {
                    cpuChanged = 1;
                }
                else
                {
                    moverProceso(&listos,&cpu,"");
                    moverProceso(&cpu,&listos,"");
                }



		}
		else if (!strcmp(evento,"V")) // OPERACION V SOBRE UN SEMAFORO
		{

            if (!strcmp(semaforo,"V"))
		    {
               sem = &v;
		    }
		    else if (!strcmp(semaforo,"LL"))
		    {
                sem = &ll;
		    }
		    else if (!strcmp(semaforo,"M"))
		    {
                sem = &m;
		    }

            // Una operación V provoca que el invocante abandone el procesador y vuelva a la cola de listos.
            // Si la operación V invocada despierta otro proceso, el invocante de la operación V es el que
            // primero arriba a la cola de listos.

            moverProceso(&listos,&cpu,"");
            if (listos != NULL)
            {
                moverProceso(&cpu,&listos,"");
                cpuChanged = 1;
            }
            else
                    cpu = NULL;

            // Ahora si verifico si la operacion V despierta a un proceso

            if (++(*sem).valor <= 0)
                moverProceso(&listos,&(*sem).bloqueados,"");

		}


		//Mostrar por pantalla el paso si corresponde
		if (pasoapaso)
		{

			//system("clear");
			printf("******************************************************************\n");
			printf("                  SIMULACIÓN PASO A PASO\n");
			printf("******************************************************************\n");
			printf("\n");

			printf("------------------------------------\n");
			printf("---- DATOS DEL EVENTO PROCESADO ----\n");
			printf("TIPO DE EVENTO: %s\n",evento);
			if(!strcmp(evento,"P") || !strcmp(evento,"V")) printf("SEMÁFORO: %s\n",semaforo);
			printf("PID: %s\n",pid);
			printf("------------------------------------\n");
			printf("\n");
			printf("---- ESTADO ACTUAL ----\n");
			printf("RELOJ: %d\n",reloj);
			if (cpu != NULL) printf("CPU: %s\t",cpu->pid); else printf("CPU: idle\t");if (SC) printf("# EJECUTANDO SECCIÓN CRÍTICA # \n"); else printf("\n");
			printf("V = %d - LL = %d - M = %d\n",v.valor,ll.valor,m.valor);
			printf("--------- COLAS DE PROCESOS --------\n");
			if (listos != NULL) {printf("Listos: ");mostrarProcesos(listos);} else printf("Listos: { }\n");
			if (finalizados != NULL) {printf("Finalizados: ");mostrarProcesos(finalizados);} else printf("Finalizados: { }\n");
			if (bloqueadosIO != NULL) {printf("Bloqueados por I/O: ");mostrarProcesos(bloqueadosIO);} else printf("Bloq. I/O: { }\n");
			if (v.bloqueados != NULL) {printf("Bloqueados Sem (v): ");mostrarProcesos(v.bloqueados);} else printf("Bloq. Sem (v): { }\n");
			if (ll.bloqueados != NULL) {printf("Bloqueados Sem (ll): ");mostrarProcesos(ll.bloqueados);} else printf("Bloq. Sem (ll): { }\n");
			if (m.bloqueados != NULL) {printf("Bloqueados Sem (m): ");mostrarProcesos(m.bloqueados);} else printf("Bloq. Sem (m): { }\n");
			printf("\n");
			printf("------------------------------------");
			printf("¿Desea continuar? s/n : ");
			fflush(stdin);
			continua=getchar();getchar();
			printf("\n");
			if (continua == 'n') pasoapaso = 0;
		}


		//Buscar el próximo system call del proceso que esta en ejecución.

		//** SOLO SI HAY UN PROCESO EN EJECUCIÓN!! **>
		if (cpu != NULL)
		{

            auxEventos = eventos;
            while(auxEventos != NULL && strcmp(cpu->pid,auxEventos->pid))
            {
                auxEventos = auxEventos->sig;
            }

            // Sólo si no hay un próximo evento para el proceso que esta en el procesador o el procesador cambio de proceso
			if (auxEventos == NULL || cpuChanged)
			{

			    // Recorro la lista de system calls en busca de un sytem call generado por el proceso que esta en ejecución
				actSC = scs;
				antSC = NULL;
				while (actSC != NULL && strcmp(actSC->pid,cpu->pid))
				{
					antSC = actSC;
					actSC = actSC->sig;
				}


				// Si hay un system call para el proceso que esta en ejecución
				// lo agrego a los eventos
				// y lo elimino de los system calls
				if (actSC != NULL)
				{


                    e = (EVENTOS) malloc(sizeof(EVENTO));
                    strcpy(e->codigo,actSC->tipo);
                    // El instante del evento a insertar es:
                    // El reloj actual + el instante de system call - (el tiempo que ya ejecuto)
                    e->instante = reloj + actSC->instante - (cpu->tcpu - cpu->necesidad);
                    strcpy(e->pid,actSC->pid);

                    if(!strcmp(e->codigo,"SIO"))
                    {
                        ef = (EVENTOS) malloc(sizeof(EVENTO));
                        strcpy(ef->codigo,"FIO");
                        ef->instante = reloj + actSC->instante - (cpu->tcpu - cpu->necesidad) + actSC->resolucion;
                        strcpy(ef->pid,actSC->pid);
                        insertarEvento(&eventos,&ef);
                    }

                    else if(!strcmp(e->codigo,"P") || !strcmp(e->codigo,"V"))
                    {
                        strcpy(e->semaforo,actSC->semaforo);
                    }

                    insertarEvento(&eventos,&e);

                    //actInstante = actSC->instante;


                    // Elimino el System Call de la lista
                    if (antSC == NULL) // el SC fue el primero de la lista
                    {
                        scs = scs->sig;
                        actSC = actSC->sig;
                    }
                    else
                    {
                        aux = actSC;
                        actSC = actSC->sig;
                        antSC->sig = actSC;
                        free(aux);
                    }

				}


                // Verifico si el proceso actual finaliza antes del próximo evento
                // En caso afirmativo creo el evento FPR que se insertará anterior al próximo evento actual
				if (eventos->sig != NULL)
				{

					if (cpu->necesidad - (eventos->sig->instante - reloj) <= 0 )
					{
						efp = (EVENTOS) malloc(sizeof(EVENTO));
						strcpy(efp->codigo,"FPR");
						// la necesidad es cero o negaitva por lo que esta suma en realidad resta ¡¡@K¡¡
						efp->instante = eventos->sig->instante + cpu->necesidad;
						strcpy(efp->pid,cpu->pid);
						insertarEvento(&eventos,&efp);
					}
				}
				else
				{
					efp = (EVENTOS) malloc(sizeof(EVENTO));
					strcpy(efp->codigo,"FPR");
					// la necesidad es cero o negaitva por lo que esta suma en realidad resta ¡¡@K¡¡
					efp->instante = reloj + cpu->necesidad;
					strcpy(efp->pid,cpu->pid);
					insertarEvento(&eventos,&efp);
				}

			}

            // Al proceso que se esta ejecutando le decremento la necesidad de acuerdo al tiempo que ejecutara hasta el proximo evento.
            cpu->necesidad -= eventos->sig->instante - reloj;


            // Guarda el PID del proceso en ejecución, para no volver a buscar otro SC para este proceso si sigue en ejecución.
			strcpy(antPid,cpu->pid);
		}



		//Antes de avanzar al siguiente evento, lo guardo para el trace.txt
		EVENTOS ultE;

		if (*re == NULL)
		{
			*re = eventos;
			eventos = eventos->sig;
			(*re)->sig = NULL;
		}
		else
		{
			ultE = *re;
			while (ultE->sig != NULL)
			{
				ultE = ultE->sig;
			}
			ultE->sig = eventos;
			eventos = eventos->sig;
			ultE = ultE->sig;
			ultE->sig = NULL;
		}


        //Guarda el instante anterior para calcular nuevos instantes
		relojAnt = reloj;

	}
	//Antes de terminar la simulación copio los procesos finalizados a procesos de salida
	auxFin = finalizados;
	while (auxFin != NULL)
	{
		auxFin->tETotal = auxFin->tEListos + auxFin->tESync + auxFin->tEDisp;
		auxFin = auxFin->sig;
	}
	*rp = finalizados;

}

/**
 * Escribe los resultados de la simulación a archivos de texto.
 * @param p Lista de procesos
 * @param e Lista de eventos
 * @param s Estructura de Indicadores del Sistema
 *
 */
int escribirResultados(char * archp, char * archs, char * arche,PROCESOS p,SYSIND s, EVENTOS e)
{
	int cont=0;

	FILE *fe;
	fe = fopen(arche,"w");
	printf("Generando archivo de eventos procesados...\n");
	fprintf(fe,"Archivo generado automaticamente por el simulador...\n");
	fprintf(fe,"\n");
	fprintf(fe,"INSTANTE\tPID\tEVENTO\tSEMAFORO\n");
	while (e != NULL)
	{
		fprintf(fe,"%d\t\t%s\t%s\t%s\n",e->instante,e->pid,e->codigo,e->semaforo);
		e = e->sig;
	}
	fclose(fe);

	FILE *fp;
	fp = fopen(archp,"w");
	printf("Generando archivo de indicadores de procesos...\n");
	fprintf(fp,"Archivo generado automaticamente por el simulador...\n");
	fprintf(fp,"\n");
	fprintf(fp,"PID\t\tTURNAROUND\tT° RESP\t\tT° E LISTOS\tT° E SYNC\tT° E DISP\tT° E TOTAL\n");
	while (p != NULL)
	{
	    if (p->tResp == 0) p->tResp = p->turnAround;
		fprintf(fp,"%s\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",p->pid,p->turnAround,p->tResp,p->tEListos,p->tESync,p->tEDisp,p->tETotal);
		cont ++;
		s.tAroundP += p->turnAround;
		s.tRespP += p->tResp;
		s.tEListosP += p->tEListos;
		s.tESyncP += p->tESync;
		p = p->sig;
	}
	fclose(fp);
	if (s.tAroundP != 0) s.tAroundP = (float) s.tAroundP / cont;
	if (s.tRespP != 0) s.tRespP = (float) s.tRespP / cont;
	if (s.tEListosP != 0) s.tEListosP = (float) s.tEListosP / cont;
	if (s.tESyncP != 0) s.tESyncP = (float) s.tESyncP / cont;

	FILE *fs;
	fs = fopen(archs,"w");
	printf("Generando archivo de indicadores a nivel sistema...\n");
	fprintf(fs,"Archivo generado automaticamente por el simulador...\n");
	fprintf(fs,"\n");
	fprintf(fs,"TurnAround Promedio = %2.2f\n",s.tAroundP);
	fprintf(fs,"Tiempo de Respuesta Promedio = %2.2f\n",s.tRespP);
	fprintf(fs,"Tiempo de Espera en Listos Promedio = %2.2f\n",s.tEListosP);
	fprintf(fs,"Timepo de Espera por Sync Promedio = %2.2f\n",s.tESyncP);

	return 1;

}


/**
 * Imprime la ayuda del sistema.
 * La ayuda se ejecuta con el parámetro -h o --help pasado como argumento del programa en su ejecución
 */
void imprimirAyuda()
{
	printf("Modo de uso:\n");
	printf("simulador [-p archivo_entrada_de_procesos] [-sc archivo_entrada_de_system_calls] [-buf buffer] [-rs archivo_resultados_nivel_sistema] [-rp archivo_resultados_de_procesos] [-ev archivo_salida_eventos_tratados]\n");
	printf("\n");
	printf("* Los parámetros son todos opcionales. No importa el orden.\n");
	printf("* En el caso de que no se indique algun parámetro estos se tomarán por defecto de acuerdo a:\n");
	printf("	-p 		: procs.txt\n");
	printf("	-sc 	: scs.txt\n");
	printf("	-buf 	: 50\n");
	printf("	-rs 	: sys-ind.txt\n");
	printf("	-rp 	: proc-ind.txt\n");
	printf("	-ev 	: trace.txt\n");
	printf("\n");
	printf("\n");
	printf("* El programa simulador cuenta con un menú de opciones entre las que se puede elejir:\n");
	printf("	1) Simulación paso a paso -> Le permite visualizar en cada instante:\n");
	printf("					- Reloj (Instante de tiempo)\n");
	printf("					- CPU (Proceso que se esta ejecutando)\n");
	printf("					- Datos del evento procesado (Tipo de evento y PID que lo generó)\n");
	printf("					- Colas de procesos, si tienen elementos (Listos, Bloqueados por IO, Bloqueados por SYNC, Finalizados)\n");
	printf("		** Además le permite en cualquier instante no continuar y generar directamente los resultados.\n");
	printf("\n");
	printf("	2) Generar directamente resultados de la simulación -> Genera los resultados\n");
	printf("\n");
	printf("	3) Salir inmediatamente! -> Finaliza el programa :) \n");
	printf("\n");
	printf("\n");
	printf("* Este programa tiene fines educativos y esta licenciado por sus autores con la licencia GNU GPL v3 o posterior\n");
	printf("* Para ver una copia de la licencia puede entrar en http://www.gnu.org/licenses/gpl-3.0.txt\n");
	printf("\n");
	printf("*************************************************************\n");
}

/**
 * Comprueba los parámetros de entrada del sistema.
 * Entre las comprobaciones que realiza controla la cantidad, la validez y si algun parámetro esta repetido.
 * @param argc Cantidad de argumentos recibidos en la ejecución
 * @param argv Arreglo con los argumentos recibidos en la ejecución
 * @return Devuelve 1 si se comprueban los parámetros correctamente o 0 (cero) en otro caso
 */
int comprobarParametros(int argc, char *argv[])
{

	/*
	 * Datos de entrada
        -p nombre archivo   Para especificar archivo de procesos
        -sc nombre archivo  Para especificar archivo de solicitudes al SO de cada proceso
        -buf n              Para especificar el tamaño del buffer a compartir entre procesos

	   Datos de salida
		-rs nombre archivo  Para especificar archivo de resultados a nivel sistema
        -rp nombre archivo  Para especificar archivo de resultados a nivel proceso
        -ev nombre archivo  Para especificar archivo de eventos tratados

	 */


	// Si solo envío un parámetro y ademas el parámetro es --help o -h
	if (argc == 2 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")))
	{
		imprimirAyuda();
		return 0;
	}

	// Si se envío algun parámetro, estos tienen que venir "paramtero" "valor"
	// Por lo que si, restando el argv[0] la cantidad de parámetros es impar: error
	// O si la cantidad de parametros es mayor a 13: error
	if ((argc > 1 && (argc -1)  % 2 != 0) || argc > 13)
	{
		printf("Error: Cantidad de parámetros incorrectos.\n");
		imprimirAyuda();
		return 0;
	}
	int i;
	// Verifico que las opciones de parámetros sean válidas
	for (i=1; i<argc; i+=2)
	{
		if (strcmp(argv[i],"-p") && strcmp(argv[i],"-sc") && strcmp(argv[i],"-buf") && strcmp(argv[i],"-rs") && strcmp(argv[i],"-rp") && strcmp(argv[i],"-ev"))
		{
			printf("Error: El parámetro %s es inválido!.\n",argv[i]);
			imprimirAyuda();
			return 0;
		}
	}

	// Verifico que no hay parámetros repetidos
	int j=1,k;
	while (j<argc)
	{
		k=j+2;
		while (k<argc && strcmp(argv[j],argv[k])) k+=2;
		if (k<argc)
		{
			printf("Error:  Parámetro %s repetido.\n",argv[j]);
			imprimirAyuda();
			return 0;
		}
		j+=2;
	}

	return 1;
}

/**
 * Carga los archivos de texto a lista dinámicas para ser utilizados por el sistema.
 * @param p Ruta del archivo de entrada de Procesos
 * @param io Ruta del archivo de entrada de I/O
 * @param prec Ruta del archivo de entrada de Precedencia
 * @param procesos Lista dinámica donde se cargan los Procesos
 * @param ios Lista dinámica donde se cargan las I/O
 * @param precs Lista dinámica donde se carga la Precedencia
 * @return Devuelve 1 si se cargaron los 3 archivos de forma correcta o 0 (cero) en otro caso
 */
int cargarArchivos(char *p, char *sc, PROCESOSIN *procesos, SCALLS *scalls)
{

	FILE * fp, * fsc;
	PROCESOSIN ultimoProceso,antP;
	SCALLS ultimoSc,antSc;


	// Cargo la lista de los procesos de entrada desde el archivo de procesos de entrada.
	fp = fopen(p,"r");
	if (fp!=NULL || !feof(fp))
	{

		*procesos = (PROCESOSIN)malloc(sizeof(PROCESOIN));
		if(fscanf(fp,"%s %d %d",(*procesos)->pid,&((*procesos)->arribo),&((*procesos)->Tcpu))==3)
		{
			(*procesos)->sig = NULL;
			ultimoProceso = *procesos;
		}
		else
		{
			free(*procesos);
		}
		while(!feof(fp))
		{
			ultimoProceso->sig = (PROCESOSIN)malloc(sizeof(PROCESOIN));
			antP = ultimoProceso;
			ultimoProceso = ultimoProceso->sig;
			if (fscanf(fp,"%s %d %d",ultimoProceso->pid,&(ultimoProceso->arribo),&(ultimoProceso->Tcpu))!=3)
			{
				free(ultimoProceso);
				ultimoProceso = antP;
			}
			ultimoProceso->sig=NULL;
		}
	}
	else
	{
		return 0;
	}
	fclose(fp);

	// Cargo la lista de los io de entrada desde el archivo de io de entrada.
	fsc = fopen(sc,"r");
	if (fsc!=NULL && !feof(fsc))
	{
		*scalls = (SCALLS)malloc(sizeof(SCALL));
		if(fscanf(fsc,"%s %d %d %s %s",(*scalls)->pid,&((*scalls)->instante),&((*scalls)->resolucion),(*scalls)->tipo,(*scalls)->semaforo)==5)
		{
			(*scalls)->sig = NULL;
			ultimoSc = *scalls;
		}
		else
		{
			free(*scalls);
		}
		while(!feof(fsc))
		{
			ultimoSc->sig = (SCALLS) malloc(sizeof(SCALL));
			antSc = ultimoSc;
			ultimoSc = ultimoSc->sig;
			if(fscanf(fsc,"%s %d %d %s %s",ultimoSc->pid,&(ultimoSc->instante),&(ultimoSc->resolucion),ultimoSc->tipo,ultimoSc->semaforo)!=5)
			{
				free(ultimoSc);
				ultimoSc = antSc;
			}
			ultimoSc->sig = NULL;

		}
	}
	else
	{
		return 0;
	}
	fclose(fsc);


	return 1;
}


/**
 * Función para validar los datos de entrada
 */

int validarDatos(PROCESOSIN p,SCALLS scalls)
{
	SCALLS scallsAux,scallsAct,scallsAnt;
	PROCESOSIN pAct;

    //Buscar PIDs que no comiencen ni en 0 ni en 1
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] != '0' && pAct->pid[0] != '1')
            return 0;  // No empieza con 0 ni con 1
        pAct = pAct->sig;
    }


	// Buscar SCALLS de procesos que no esten en la lista de procesos
	scallsAux = scalls;
	while (scallsAux != NULL)
	{
		scallsAct = scallsAux;
		while (scallsAux != NULL && !strcmp(scallsAct->pid,scallsAux->pid))
		{
			scallsAux = scallsAux->sig;
		}
		pAct = p;
		while (pAct != NULL && strcmp(scallsAct->pid,pAct->pid))
		{
			pAct = pAct->sig;
		}
		if (pAct == NULL) return 0; // NO lo encontro

	}


    // Buscar procesos productores que no informen el par de solicitudes P(V) y V(LL)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '0')
        {
            int valido = 0;
            scallsAux = scalls;
            while (scallsAux != NULL && valido!=2)
            {
                if (valido == 0 && !strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"P") && !strcmp(scallsAux->semaforo,"V")) valido++;
                if (valido == 1 && !strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"V") && !strcmp(scallsAux->semaforo,"LL")) valido++;
                if (valido == 2 )
                {
                    scallsAux = NULL; // Para forzar la salida del ciclo
                }
                else
                {
                    scallsAux = scallsAux->sig;
                }

            }

            if (valido != 2 ) return 0; // encontro uno que no es valido
        }

        pAct = pAct->sig;

    }


    //Buscar procesos productores que no hagan un P(M) inmediatamente despues de un P(V) (mismo instante)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '0')
        {
            scallsAux = scalls;
            while (scallsAux != NULL)
            {
                if (!strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"P") && !strcmp(scallsAux->semaforo,"V"))
                {
                    if (!strcmp(pAct->pid,scallsAux->pid) && scallsAux->instante == scallsAux->sig->instante && (strcmp(scallsAux->sig->tipo,"P") || strcmp(scallsAux->sig->semaforo,"M"))) return 0;
                }
                scallsAux = scallsAux->sig;
            }

        }

        pAct = pAct->sig;
    }


    //Buscar procesos productores que no hagan un V(M) inmediatamente antes de un V(LL) (mismo instante)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '0')
        {
            scallsAnt = NULL;
            scallsAux = scalls;
            while (scallsAux != NULL)
            {
                if (!strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"V") && !strcmp(scallsAux->semaforo,"LL"))
                {
                    if (scallsAnt == NULL || strcmp(pAct->pid,scallsAnt->pid) || (strcmp(scallsAnt->tipo,"V") || strcmp(scallsAnt->semaforo,"M"))) return 0;
                }
                scallsAnt = scallsAux;
                scallsAux = scallsAux->sig;
            }

        }

        pAct = pAct->sig;
    }


    // Buscar procesos cosumidores que no informen el par de solicitudes P(LL) y V(V)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '1')
        {
            int valido = 0;
            scallsAux = scalls;
            while (scallsAux != NULL && valido!=2)
            {
                if (valido == 0 && !strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"P") && !strcmp(scallsAux->semaforo,"LL")) valido++;
                if (valido ==1 && !strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"V") && !strcmp(scallsAux->semaforo,"V")) valido++;
                if (valido == 2 )
                {
                    scallsAux = NULL; // Para forzar la salida del ciclo
                }
                else
                {
                    scallsAux = scallsAux->sig;
                }

            }

            if (valido != 2) return 0; // encontro uno que no es valido
        }

        pAct = pAct->sig;

    }


    //Buscar procesos consumidores que no hagan un P(M) inmediatamente despues de un P(LL) (mismo instante)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '1')
        {
            scallsAux = scalls;
            while (scallsAux != NULL)
            {
                if (!strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"P") && !strcmp(scallsAux->semaforo,"LL"))
                {
                    if (scallsAux->sig != NULL && !strcmp(pAct->pid,scallsAux->sig->pid) && scallsAux->instante == scallsAux->sig->instante && (strcmp(scallsAux->sig->tipo,"P") || strcmp(scallsAux->sig->semaforo,"M"))) return 0;
                }
                scallsAux = scallsAux->sig;
            }

        }

        pAct = pAct->sig;
    }


    //Buscar procesos consumidores que no hagan un V(M) inmediatamente antes de un V(V) (mismo instante)
    pAct = p;
    while (pAct != NULL)
    {
        if (pAct->pid[0] == '1')
        {
            scallsAnt = NULL;
            scallsAux = scalls;
            while (scallsAux != NULL)
            {
                if (!strcmp(pAct->pid,scallsAux->pid) && !strcmp(scallsAux->tipo,"V") && !strcmp(scallsAux->semaforo,"V"))
                {
                    if (scallsAnt == NULL || strcmp(pAct->pid,scallsAux->pid) || (strcmp(scallsAnt->tipo,"V") || strcmp(scallsAnt->semaforo,"M"))) return 0;
                }
                scallsAnt = scallsAux;
                scallsAux = scallsAux->sig;
            }

        }

        pAct = pAct->sig;
    }

	//Buscando SCALLS que tengan instantes de solicitud incompatibles con la duración del proceso
	scallsAux = scalls;
	while (scallsAux != NULL)
	{
		scallsAct = scallsAux;
		if (scallsAct->instante == 0) return 0; // hace solicitud en el primer instante
		while (scallsAux != NULL && !strcmp(scallsAct->pid,scallsAux->pid))
		{
			scallsAux = scallsAux->sig;
		}
		pAct = p;
		while (pAct != NULL && strcmp(scallsAct->pid,pAct->pid))
		{
			pAct = pAct->sig;
		}
		if (scallsAct->instante >= pAct->Tcpu) return 0; // Hace solicitud en el ultimo o proximos instantes

	}

	return 1;
}


#endif // SCPFUNC_H_INCLUDED
