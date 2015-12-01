/*

© 2013
Autores:

* F. Daguerre, Federico M. <fetudaguerre@gmail.com>
* Buscaglia, Alan <alanbuscaglia@gmail.com>
* Pennisi, Giuliana <guipennisi92@gmail.com>
* Aristoy, Gonzalo <gonzaloaristoy@hotmail.com>

Este código se escribió en el marco del Trabajo Práctico N 2
de la cátedra de Sistemas Operativos I de la Universidad CAECE
Mar del Plata. Este programa permite realizar la simulación
de la ejecución de procesos concurrentes en un entorno monoprocesador,
con IPC (sincronización entre los procesos).

Para más información dirijase a la documentación técnica
en el directorio docs.

Docentes:

* Valania, Paula
* Ojeda, German

Este programa es software libre: podés distribuirlo y/o modificarlo
bajo los términos de la Licencia Pública General de GNU como está
publicada por la Free Software Foundation, en su versión 3 o, si lo
preferís cualquiera posterior.

Este programa se distribuye con la esperanza que sea útil,
pero SIN NINGUNA GARANTÍA; ni siquiera la garantía implícita de
COMERCIALIZACIÓN o IDONEIDAD PARA UN PROPÓSITO PARTICULAR.
Véa la Licencia General Púbica de GNU para mas detalles.

Junto con este programa fuente se distribuye una copia de la
Licencia Púbica General de GNU. Si no es así puede encontrar una
copia en  <http://www.gnu.org/licenses/>

Si decides mejorar nuestro código te agradeceremos que nos envíes una copia.

*/

/*! \file main.c*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*! \typedef PROCESOIN
 * Estructura para leer una línea del archivo de entrada de procesos (INPUT)
 */
typedef struct procesoIN
{
	int pid; /**< PID */
	int arribo; /**< Instante de arribo al sistema */
	int Tcpu; /**< Tiempo que requiere de CPU */
	struct procesoIN *sig; /**< Puntero al próximo proceso */
}PROCESOIN;

/*! \typedef PROCESOSIN
 * Lista dinámica de procesos de entrada
 */
typedef PROCESOIN * PROCESOSIN;


/*! \typedef SCALL
 * Estructura para almacenar un System Call (INPUT)
 */
typedef struct scall
{
	int pid; /**< PID del proceso que solicita servicio */
	int instante; /**< Instante de ocurrencia de la solicitud relativo al tiempo de ejecución del proceso */
	int resolucion; /**< Tiempo que toma resolver la solicitud de servicio */
	char tipo[4]; /**< Tipo de solicitud: SIO, V, P */
	char semaforo[3]; /**< Cadena que almacena el semaforo asociado si el tipo es V o P: M/V/LL */
	struct scall * sig; /**< Puntero al próximo System Call */
}SCALL;

/*! \typedef SCALLS
 * Lista dinámica de System Calls
 */
typedef SCALL * SCALLS;


/*! \typedef PROCESO
 * Estructura para alamcenar un proceso que este activo en el sistema (OUTPUT)
 */
typedef struct proceso
{
	int pid; /**< Identificador único del proceso */
	int turnAround; /**< TurnAround del proceso */
	int tResp; /**< Tiempo de Respuesta del proceso */
	int tEListos; /**< Tiempo de Espera en cola de Listos del proceso */
	int tESync; /**< Tiempo de Espera en cola de bloqueados por sincronización del proceso */
	int tEMutex; /**< Tiempo de Espera en cola de bloqueados por exclusión mutua del proceso */
	int tETotal; /**< Tiempo de Espera Total del proceso */
	int tEDisp; /**< Tiempo de Uso de Dispositivos */
	int contIos; /**< Cuenta la cantidad de IOs del proceso (para el Tiempo de Respuesta) */
	struct proceso *sig; /**< Puntero al próximo proceso */
}PROCESO;

/*! \typedef PROCESOS
 * Lista dinámica de procesos
 *
 */
typedef PROCESO * PROCESOS;


/*! \typedef SYSIND
 * Estructura para almacenar información de indicadores del sistema (OUTPUT)
 */
typedef struct sysind
{
	float tAroundP; /**< Turn Around Promedio */
	float tEListosP; /**< Tiempo de Espera en cola de Listos Promedio */
	float tRespP; /**< Tiempo de Respuesta Promedio */
	float tESyncP; /**< Tiempo de Espera en cola de sincronización Promedio */
	float tEMutexP; /**< Tiempo de Espera en cola de bloqueados por exclusión mutua Promedio */
}SYSIND;


/*! \typedef EVENTO
 * Estructura para alamacenar un evento de ocurrencia en la simulación (OUTPUT)
 */
typedef struct evento
{
	int instante; /**< Instante de ocurrencia del evento */
	int pid; /**< PID que genero el evento */
	char codigo[4]; /**< Código del evento : ANP,SIO,FIO, FPR, P, V */
	char semaforo[6]; /**< Nombre del semáforo, en el caso de que el código se V o P */
	struct evento * sig; /**< Puntero al próximo evento */
}EVENTO;

/*! \typedef EVENTOS
 * Lista dinámica de eventos
 */
typedef EVENTO * EVENTOS;


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
 * @param pid En el caso que el pid sea 0 (cero) moverá de origen el primer elemento, de lo contrario buscara el pid y ese es el que moverá
 */
void moverProceso(PROCESOS * destino,PROCESOS *origen, int pid)
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
		if (pid == 0)
		{
			*destino = *origen;
			*origen = (*origen)->sig;
			(*destino)->sig = NULL;
		}
		// Si hay que buscar el elemento de la cola origen:
		else
		{
			// Avanzo con antO y actO en la cola de origen hasta encontrar el proceso
			while(actO != NULL && actO->pid != pid)
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
				*origen = NULL;
			}

			// Como es el unico elemento que quedo en la cola de destino, el sig es NULL
			(*destino)->sig = NULL;
		}
	}
	// Si hay elementos en la cola de destino tengo en ultD el ultimo elemento de destino
	else
	{
		// Si hay que mover el primer elemento de la cola origen:
		if (pid == 0)
		{
			ultD->sig = *origen;
			*origen = (*origen)->sig;
		}
		//Si hay que buscar el elemento de la cola origen:
		else
		{
			// Avanzo con antO y actO en la cola de origen hasta encontrar el proceso
			while(actO != NULL && actO->pid != pid)
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
		printf("%d ",procesos->pid);
		procesos = procesos->sig;
	}
	printf("\n");
}

/**
 * Devuelve una lista de semáforos asociados a un proceso determinado por su PID.
 * @param procesos Lista de procesos activos donde buscar
 * @param pid PID del proceso del que se quieren obtener los semáforos asociados
 * @return Devuelve una lista de semáforos
 */
SEMAFOROS obtenerSemaforos(PROCESOS procesos,int pid)
{

	while(procesos!=NULL && procesos->pid!=pid)
	{
		procesos=procesos->sig;
	}
	return procesos->semaforos;
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
	int pid;
	int actInstante;
	int pidb;
	int antPid=0;
	char evento[4];
	int semT;
	char continua;

	PROCESOS cpu;
	PROCESOS p,pAct;
	EVENTOS e,ef;
	SEMAFOROS semAct;
	PROCESOS listos = NULL,auxListos;
	PROCESOS finalizados = NULL,auxFin;
	PROCESOS bloqueadosIO = NULL,auxbloqIO;
	PROCESOS bloqueadosSYNC = NULL,auxbloqSYNC;


	SCALLS antSC,actSC,aux;

	cpu = NULL; //el procesador esta idle
	while (eventos!=NULL)
	{




		reloj = eventos->instante;
		// PARECIERA QUE LA CONSIGNA OLVIDA DE SOLICITAR MOSTRAR QUIEN ESTA EN EL CPU. Y EN CAMBIO PIDE PID GENERADOR DEL EVENTO
		pid = eventos->pid;
		strcpy(evento,eventos->codigo);


		// Antes de procesar el evento, le sumamos a los tiempos de espera
		// Los que están en listos suman en Tiempo de Espera en cola de Listos
		auxListos = listos;
		while (auxListos != NULL)
		{
			auxListos->tEListos += reloj - relojAnt;
			auxListos = auxListos->sig;
		}

		// Los que están en bloqueados SYNC suman en Tiempo de Espera por sincronización
		auxbloqSYNC = bloqueadosSYNC;
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
			while (pAct!= NULL && pAct->pid != eventos->pid)
			{
				pAct = pAct->sig;
			}
			//Copio el proceso simulando activarlo en el sistema
			p = (PROCESOS)malloc(sizeof(PROCESO));
			p->pid = pAct->pid;
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
			p->semaforos = pAct->semaforos; // esto esta bien copiado?
			p->sig = NULL;

			// Si la cola de listos esta vacía y no hay nadie en el procesador:
			// asigno al procesador el proceso, de lo contrario lo agrego a listos.
			if (listos == NULL && cpu == NULL)
			{
				moverProceso(&cpu,&p,0);
			}
			else
			{
				moverProceso(&listos,&p,0);
			}
		}
		else if (!strcmp(evento,"SIO")) // SOLICITUD DE ENTRADA SALIDA
		{
			// Envío el proceso que esta en el cpu a la lista de bloqueados por IO
			moverProceso(&bloqueadosIO,&cpu,0);

			// Verifico si hay alguien listo para ir al procesador.
			// Si no hay nadie listo: el cpu queda idle
			if (listos == NULL)
				cpu = NULL;
			else
				moverProceso(&cpu,&listos,0);

		}
		else if (!strcmp(evento,"FIO")) // FIN DE ENTRADA SALIDA
		{

			if (cpu != NULL)
			{
				moverProceso(&listos,&bloqueadosIO,pid);
				// BUsco el proceso que acaba de terminar su IO en la cola de listos
				auxListos = listos;
				while (auxListos !=NULL && auxListos->pid != pid)
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

			moverProceso(&finalizados,&cpu,0);

			// Si no hay nadie listo el cpu queda idle.
			if (listos == NULL)
				cpu = NULL;
			else
				moverProceso(&cpu,&listos,0);
		}
		else if (!strcmp(evento,"P")) // OPERACION P SOBRE UN SEMAFORO
		{
			semAct = cpu->semaforos;//obtenerSemaforos(procesos,eventos->pid);
			while(semAct != NULL && semAct->nombre != eventos->semaforo)
			{
				semAct = semAct->sig;
			}
			if (--semAct->valor < 0)
			{
				moverProceso(&bloqueadosSYNC,&cpu,0);
			}

			if (cpu == NULL && listos != NULL)
			{
				moverProceso(&cpu,&listos,0);
			}
		}
		else if (!strcmp(evento,"V")) // OPERACION V SOBRE UN SEMAFORO
		{
			pAct = bloqueadosSYNC;
			while (pAct != NULL)
			{
				semAct = pAct->semaforos;
				while(semAct !=NULL && semAct->nombre != eventos->semaforo)
				{
					semAct = semAct->sig;
				}
				if(semAct != NULL && ++semAct->valor >= 0)
				{
					semAct = pAct->semaforos;
					semT=1;
					while (semAct !=NULL)
					{
						if (semAct->tipo == 'P' && semAct->valor < 0)
						{

							semT = 0;
							semAct = NULL;
						}
						semAct = semAct->sig;
					}
					if (semT)
					{
						pidb = pAct->pid;
						moverProceso(&listos,&bloqueadosSYNC,pidb);
					}
					pAct = pAct->sig;
				}
				else
				{
				pAct = pAct->sig;
				}
			}

		}


		//Mostrar por pantalla el paso si corresponde
		if (pasoapaso)
		{

			system("clear");
			printf("******************************************************************\n");
			printf("                  SIMULACIÓN PASO A PASO\n");
			printf("******************************************************************\n");
			printf("\n");
			printf("RELOJ: %d\n",reloj);
			if (cpu!=NULL) printf("CPU: %d\n",cpu->pid); else printf("CPU: idle\n");
			printf("------------------------------------\n");
			printf("---- DATOS DEL EVENTO PROCESADO ----\n");
			printf("TIPO DE EVENTO: %s\n",evento);
			printf("PID: %d\n",pid);
			printf("------------------------------------\n");
			printf("--------- COLAS DE PROCESOS --------\n");
			if (listos != NULL) {printf("Listos: ");mostrarProcesos(listos);}
			if (finalizados != NULL) {printf("Finalizados: ");mostrarProcesos(finalizados);}
			if (bloqueadosIO != NULL) {printf("Bloqueados por I/O: ");mostrarProcesos(bloqueadosIO);}
			if (bloqueadosSYNC != NULL) {printf("Bloqueados por sincronización: ");mostrarProcesos(bloqueadosSYNC);}
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



			if (cpu->pid != antPid || eventos->sig == NULL)
			{
				actSC = scs;
				antSC = NULL;
				while (actSC != NULL && actSC->pid != cpu->pid)
				{
					antSC = actSC;
					actSC = actSC->sig;
				}


				// Si hay un system call para el proceso que esta en ejecución
				// lo agrego a los eventos
				// y lo elimino del los system calls
				if (actSC!=NULL)
				{

					do
					{
						e = (EVENTOS) malloc(sizeof(EVENTO));
						strcpy(e->codigo,actSC->tipo);
						e->instante = reloj + actSC->instante - (cpu->tcpu - cpu->necesidad);
						e->pid = actSC->pid;

						if(!strcmp(e->codigo,"SIO"))
						{
							ef = (EVENTOS) malloc(sizeof(EVENTO));
							strcpy(ef->codigo,"FIO");
							ef->instante = reloj + actSC->instante - (cpu->tcpu - cpu->necesidad) + actSC->resolucion;
							ef->pid = actSC->pid;
							insertarEvento(&eventos,&ef);
						}
						else if(!strcmp(e->codigo,"P"))
						{
							e->semaforo = actSC->semaforo;
						}
						else if(!strcmp(e->codigo,"V"))
						{
							e->semaforo = actSC->semaforo;
						}
						insertarEvento(&eventos,&e);

						actInstante = actSC->instante;

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
					while ((actSC != NULL && actInstante == actSC->instante && !strcmp(actSC->tipo,"V"))); //|| (eventos->sig == NULL && actSC->sig != NULL));
				}

				if (eventos->sig != NULL)
				{

					cpu->necesidad -= eventos->sig->instante - reloj;

					if (cpu->necesidad <= 0)
					{
						e = (EVENTOS) malloc(sizeof(EVENTO));
						strcpy(e->codigo,"FPR");
						// la necesidad es cero o negaitva por lo que esta suma en realidad resta ¡¡@K¡¡
						e->instante = eventos->sig->instante + cpu->necesidad;
						e->pid = cpu->pid;
						insertarEvento(&eventos,&e);
					}
				}
				else
				{
					e = (EVENTOS) malloc(sizeof(EVENTO));
					strcpy(e->codigo,"FPR");
					// la necesidad es cero o negaitva por lo que esta suma en realidad resta ¡¡@K¡¡
					e->instante = reloj + cpu->necesidad;
					e->pid = cpu->pid;
					insertarEvento(&eventos,&e);
				}

			}


			antPid = cpu->pid;
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
	int cont;

	FILE *fe;
	fe = fopen(arche,"w");
	printf("Generando archivo de eventos procesados...\n");
	fprintf(fe,"Archivo generado automaticamente por el simulador...\n");
	fprintf(fe,"\n");
	fprintf(fe,"INSTANTE\t\tPID\t\tEVENTO\t\tSEMAFORO\n");
	while (e != NULL)
	{
		fprintf(fe,"%d\t\t%d\t\t%s\t\t%c\n",e->instante,e->pid,e->codigo,e->semaforo);
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
		fprintf(fp,"%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",p->pid,p->turnAround,p->tResp,p->tEListos,p->tESync,p->tEDisp,p->tETotal);
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
	printf("simulador [-p arhivo de entrada de procesos] [-io archivo de entrada de solicitudes de I/O] [-prec archivo de entrada de precedencia de procesos] [-rs archivo de resultados a nivel sistema] [-rp archivo de resultados de procesos] [-ev archivo de salida de eventos tratados]\n");
	printf("\n");
	printf("* Los parámetros son todos opcionales. No importa el orden.\n");
	printf("* En el caso de que no se indique algun parámetro estos se tomarán por defecto de acuerdo a:\n");
	printf("	-p 		: procesos.txt\n");
	printf("	-io 	: io.txt\n");
	printf("	-prec 	: precedencia.txt\n");
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
	printf("	2) Generar directamente resultados de la simulación -> Genera los resultados y finaliza el programa\n");
	printf("\n");
	printf("	3) Salir inmediatamente! -> Finaliza el programa :) \n");
	printf("\n");
	printf("\n");
	printf("* Este programa tiene fines educativos y esta licenciado por sus autores con la licencia GNU GPL v3 o posterior\n");
	printf("* Para ver una copia de la licencia puede entrar en http://www.gnu.org/licenses/gpl-3.0.txt\n");
	printf("* Para reportar bugs o invitarnos una cerveza ;):\n\t** Federico M. F. Daguerre - <fetudaguerre@gmail.com>\n\t** Alan Buscaglia - <alanbuscaglia@gmail.com>\n\t** Giuliana Pennisi - <giupennisi92@gmail.com>\n\t** Gonzalo Aristoy - <gonzaloaristoy@hotmail.com>\n");
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
		-p nombre archivo		Para especificar archivo de procesos
		-io nombre archivo		Para especificar archivo de solicitudes de I/O de cada proceso
		-prec nombre archivo	Para especificar archivo precedencia de los procesos
	   Datos de salida
		-rs nombre archivo		Para especificar archivo de resultados a nivel sistema
		-rp nombre archivo		Para especificar archivo de resultados a nivel proceso
		-ev nombre archivo		Para especificar archivo de eventos tratados
	 */


	// Si solo envío un parámetro y ademas el parámetro es --help o -h
	if (argc == 2 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")))
	{
		imprimirAyuda();
		return 0;
	}

	// Si se envío algun parámetro, estos tienen que venir "paramtero" "valor"
	// Por lo que si, restando el argv[0] la cantidad de parámetros es impar: error
	// O si la cantidad de parametros es mayor a 10: error
	if ((argc > 1 && (argc -1)  % 2 != 0) || argc > 10)
	{
		printf("Error: Cantidad de parámetros incorrectos.\n");
		imprimirAyuda();
		return 0;
	}
	int i;
	// Verifico que las opciones de parámetros sean válidas
	for (i=1; i<argc; i+=2)
	{
		if (strcmp(argv[i],"-p") && strcmp(argv[i],"-io") && strcmp(argv[i],"-prec") && strcmp(argv[i],"-rs") && strcmp(argv[i],"-rp") && strcmp(argv[i],"-ev"))
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
int cargarArchivos(char *p, char *io, char *prec,PROCESOSIN *procesos, IOSIN *ios, PRECSIN *precs)
{

	FILE * fp,* fio,* fprec;
	PROCESOSIN ultimoProceso,antP;
	IOSIN ultimoIo,antIo;
	PRECSIN ultimoPrec,antPr;


	// Cargo la lista de los procesos de entrada desde el archivo de procesos de entrada.
	fp = fopen(p,"r");
	if (fp!=NULL || !feof(fp))
	{

		*procesos = (PROCESOSIN)malloc(sizeof(PROCESOIN));
		if(fscanf(fp,"%d %d %d",&((*procesos)->pid),&((*procesos)->arribo),&((*procesos)->Tcpu))==3)
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
			if (fscanf(fp,"%d %d %d",&(ultimoProceso->pid),&(ultimoProceso->arribo),&(ultimoProceso->Tcpu))!=3)
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
	fio = fopen(io,"r");
	if (fio!=NULL && !feof(fio))
	{
		*ios = (IOSIN)malloc(sizeof(IOIN));
		if(fscanf(fio,"%d %d %d",&((*ios)->pid),&((*ios)->instante),&((*ios)->tiempo))==3)
		{
			(*ios)->sig = NULL;
			ultimoIo = *ios;
		}
		else
		{
			free(*ios);
		}
		while(!feof(fio))
		{
			ultimoIo->sig = (IOSIN) malloc(sizeof(IOIN));
			antIo = ultimoIo;
			ultimoIo = ultimoIo->sig;
			if(fscanf(fio,"%d %d %d",&(ultimoIo->pid),&(ultimoIo->instante),&(ultimoIo->tiempo))!=3)
			{
				free(ultimoIo);
				ultimoIo = antIo;
			}
			ultimoIo->sig = NULL;

		}
	}
	else
	{
		return 0;
	}
	fclose(fio);

	// Cargo la lista de los precs de entrada desde el archivo de precs de entrada
	fprec = fopen(prec,"r");
	if (fprec!=NULL && !feof(fprec))
	{
		*precs = (PRECSIN) malloc(sizeof(PRECIN));
		if(fscanf(fprec,"%d %d",&((*precs)->precedente),&((*precs)->precedido))==2)
		{
			(*precs)->sig = NULL;
			ultimoPrec = *precs;
		}
		else
		{
			free(*precs);
		}
		while(!feof(fprec))
		{
			ultimoPrec->sig = (PRECSIN) malloc(sizeof(PRECIN));
			antPr = ultimoPrec;
			ultimoPrec = ultimoPrec->sig;
			if(fscanf(fprec,"%d %d",&(ultimoPrec->precedente),&(ultimoPrec->precedido))!=2)
			{
				free(ultimoPrec);
				ultimoPrec = antPr;
			}
			ultimoPrec->sig = NULL;
		}
	}
	else
	{
		return 0;
	}
	fclose(fprec);

	return 1;
}

/**
 * Crea la lista de System Calls para ser utilizada durante la simulación.
 * @param procesos Lista de procesos con la información de sincronización
 * @param ios Lista de I/O de los procesos
 * @return Devuelve una lista de System Calls
 */
SCALLS systemCalls(PROCESOS procesos, IOSIN ios)
{

	IOSIN ioAct;
	SCALLS ultimoSc,sc;
	SEMAFOROS semAct;
	sc = NULL;

	while (procesos != NULL)
	{

		//Primero inserto en la lista de system calls las operaciones P sobre los semaforos del proceso

		semAct = procesos->semaforos;

		while (semAct != NULL && semAct->tipo == 'P')
		{
			// Si es el primer elemento de la lista
			if (sc == NULL)
			{
				sc = (SCALLS)malloc(sizeof(SCALL));
				sc->pid=procesos->pid;
				sc->instante=0;
				sc->resolucion=0;
				strcpy(sc->tipo,"P");
				sc->semaforo = semAct->nombre;
				sc->sig=NULL;
				ultimoSc = sc;
			}
			else // Si no es el primer elemento de la lista
			{
				ultimoSc->sig = (SCALLS) malloc(sizeof(SCALL));
				ultimoSc = ultimoSc->sig;
				ultimoSc->pid=procesos->pid;
				ultimoSc->instante=0;
				ultimoSc->resolucion=0;
				strcpy(ultimoSc->tipo,"P");
				ultimoSc->semaforo = semAct->nombre;
				ultimoSc->sig = NULL;
			}
			semAct = semAct->sig;

		}
		// Luego inserto en la lista de system calls las solicitudes de IO de los procesos
		ioAct = ios;
		while(ioAct != NULL) //Lo mismo: si no esta ordenado recorro todo
		{
			if (ioAct->pid == procesos->pid)
			{
				//Si es el primer elemento de la lista
				if (sc==NULL)
				{
					sc = (SCALLS)malloc(sizeof(SCALL));
					sc->pid=procesos->pid;
					sc->instante=ioAct->instante;
					sc->resolucion=ioAct->tiempo;
					strcpy(sc->tipo,"SIO");
					sc->semaforo = '-';
					sc->sig=NULL;
					ultimoSc = sc;
				}
				else // Si no es el primer elemento de la lista
				{
					ultimoSc->sig = (SCALLS) malloc(sizeof(SCALL));
					ultimoSc = ultimoSc->sig;
					ultimoSc->pid=procesos->pid;
					ultimoSc->instante=ioAct->instante;
					ultimoSc->resolucion=ioAct->tiempo;
					strcpy(ultimoSc->tipo,"SIO");
					ultimoSc->semaforo = '-';
					ultimoSc->sig = NULL;
				}
			}
			ioAct = ioAct->sig;
		}

		//Por último inserto en la lista de system calls las operaciones V sobre los semaforos del proceso
		while (semAct != NULL && semAct->tipo == 'V')
		{
			/* APARENTEMENTE NUNCA VA A HABER UNA OPERACION V COMO PRIMER SYSTEM CALL DE UN PROCESO*****************/
			/* POR LO TANTO, NO SE CONSIDERA QUE PUEDA SER EL PRIMER ELEMENTO DE LA LISTA *************************/

			ultimoSc->sig = (SCALLS) malloc(sizeof(SCALL));
			ultimoSc = ultimoSc->sig;
			ultimoSc->pid=procesos->pid;
			//El instante de los V es el ultimo instante de ejecucion del proceso, que antes de empezar es igual a la necesidad.
			ultimoSc->instante=procesos->necesidad;
			ultimoSc->resolucion=0;
			strcpy(ultimoSc->tipo,"V");
			ultimoSc->semaforo = semAct->nombre; // HAY QUE REVISAR BIEN COMO ES EL TEMA DEL SEMAFORO ASOCIADO!!!
			// LA OPERACION V DEBERIA SER SOBRE EL SEMAFORO QUE CORRESPONDE....
			ultimoSc->sig = NULL;
			semAct = semAct->sig;
		}
		procesos = procesos->sig;
	}

	return sc;
}

/**
 * A partir del archivo de Precedencia y un PID se generan los semaforos asociados al proceso
 * @param precsIN Lista de Precedencia de los procesos.
 * @param pid PID del proceso del cual quiero obtener los semáforos asociados.
 * @return Devuelve una lista de semáforos asociados al PID.
 */
SEMAFOROS cargarSemaforos(PRECSIN precsIN,int pid)
{
	char nombre='a';
	SEMAFOROS ss=NULL,ult;
	while (precsIN!=NULL)
	{
		if (precsIN->precedente == pid)
		{
			if (ss == NULL)
			{
				ss = (SEMAFOROS)malloc(sizeof(SEMAFORO));
				ss->tipo = 'V';
				ss->nombre = nombre;
				ss->valor = 0;
				ss->sig = NULL;
				ult = ss;
			}
			else
			{
				ult->sig = (SEMAFOROS)malloc(sizeof(SEMAFORO));
				ult = ult->sig;
				ult->tipo = 'V';
				ult->nombre = nombre;
				ult->valor = 0;
				ult->sig = NULL;
			}
		}

		else if (precsIN->precedido == pid)
		{
			if (ss == NULL)
			{
				ss = (SEMAFOROS)malloc(sizeof(SEMAFORO));
				ss->tipo = 'P';
				ss->nombre = nombre;
				ss->valor = 0;
				ss->sig = NULL;
				ult = ss;
			}
			else
			{
				ult->sig = (SEMAFOROS)malloc(sizeof(SEMAFORO));
				ult = ult->sig;
				ult->tipo = 'P';
				ult->nombre = nombre;
				ult->valor = 0;
				ult->sig = NULL;
			}
		}
		nombre++;
		precsIN = precsIN->sig;
	}
	return ss;
}


/**
 * Función para validar los datos de entrada
 */

int validarDatos(PROCESOSIN p,IOSIN ios, PRECSIN pr)
{
	IOSIN iosAux;
	int iosAct;
	PROCESOSIN pAct;
	PRECSIN prAux;
	int prAct;

	// Buscando IOS de procesos que no esten en la lista de procesos
	iosAux = ios;
	while (iosAux != NULL)
	{
		iosAct = iosAux->pid;
		while (iosAux != NULL && iosAct == iosAux->pid)
		{
			iosAux = iosAux->sig;
		}
		pAct = p;
		while (pAct != NULL && iosAct != pAct->pid)
		{
			pAct = pAct->sig;
		}
		if (pAct == NULL) return 0; // NO lo encontro
	}

	// Buscando precendentes que no esten en la lista de procesos
	prAux = pr;
	while (prAux != NULL)
	{
		prAct = prAux->precedente;
		while (prAux != NULL && prAct == prAux->precedente)
		{
			prAux = prAux->sig;
		}
		pAct = p;
		while (pAct != NULL && prAct != pAct->pid)
		{
			pAct = pAct->sig;
		}
		if (pAct == NULL) return 0; // NO lo encontro
	}

	//Buscando precedidos que no esten en la lista de procesos
	prAux = pr;
	while (prAux != NULL)
	{
		prAct = prAux->precedido;
		while (prAux != NULL && prAct == prAux->precedido)
		{
			prAux = prAux->sig;
		}
		pAct = p;
		while (pAct != NULL && prAct != pAct->pid)
		{
			pAct = pAct->sig;
		}
		if (pAct == NULL) return 0; // NO lo encontro
	}


	//Buscando IOS que tengan instantes de solicitud incompatibles con la duración del proceso
	iosAux =ios;
	while (iosAux != NULL)
	{
		iosAct = iosAux->pid;
		while (iosAux->sig != NULL && iosAct == iosAux->sig->pid)
		{
			iosAux = iosAux->sig;
		}
		pAct = p;
		while (pAct != NULL && iosAux->pid != pAct->pid)
		{
			pAct = pAct->sig;
		}
		if (iosAux->instante >= pAct->Tcpu) return 0; // Hace solicitud en el ultimo o proximos instantes (incompatible)
		if (iosAux != NULL) iosAux = iosAux->sig;
	}



	return 1;
}
/**
 * Función principal del programa.
 * @param argc Cantidad de argumentos recibidos en la ejecución
 * @param argv Arreglo con los argumentos recibidos en la ejecución
 * @return Devuelve 0 (cero) si el programa finaliza con éxito
 */
int main(int argc, char *argv[])
{


	PROCESOS rp=NULL;
	EVENTOS re=NULL;
	SYSIND rs; // No lo inicializo porque es solo un struct

	/*
	 *
	 * Verificación de los parámetros de entrada
	 *
	 */
	printf("Verificando parámetros...\n");
	if (!comprobarParametros(argc,argv)) return 1;


	/*
	 *
	 * Carga de los archivos de entrada del programa.
	 *
	 */


	// Variables para almacenar las posiciones de los archivos de entrada
	int ap=0,aio=0,aprec=0;

	PROCESOSIN procesosIN=NULL;
	IOSIN iosIN=NULL;
	PRECSIN precsIN=NULL;


	printf("Cargando archivos de entrada...\n");
	//Busco las posiciones de los parámetros donde estan los archivos de entrada.
	int i;
	for (i=1; i<argc; i+=2)
	{
		if (!strcmp(argv[i],"-p")) ap=i+1;
		else
			if (!strcmp(argv[i],"-io")) aio=i+1;
			else
				if (!strcmp(argv[i],"-prec")) aprec=i+1;
	}

	//printf("Parámetros de entrada: %d - %d - %d\n",ap,aio,aprec);

	// Verifico todas las combinaciones de opciones de parámetros de entrada disponibles
	// en base a si un parámetro de entrada esta o no.
	if (ap == 0 && aio == 0 && aprec == 0)
	{
		if (!cargarArchivos("procesos.txt","io.txt","precedencia.txt",&procesosIN,&iosIN,&precsIN)) return 1;
	}
	else
	{
		if (ap == 0 && aio == 0 && aprec != 0)
		{
			if (!cargarArchivos("procesos.txt","io.txt",argv[aprec],&procesosIN,&iosIN,&precsIN)) return 1;
		}
		else
		{
			if (ap == 0 && aio !=0 && aprec == 0)
			{
				if (!cargarArchivos("procesos.txt",argv[aio],"precedencia.txt",&procesosIN,&iosIN,&precsIN)) return 1;
			}
			else
			{
				if (ap == 0 && aio != 0 && aprec != 0)
				{
					if (!cargarArchivos("procesos.txt",argv[aio],argv[aprec],&procesosIN,&iosIN,&precsIN)) return 1;
				}
				else
				{
					if (ap != 0 && aio == 0 && aprec == 0)
					{
						if (!cargarArchivos(argv[ap],"io.txt","precedencia.txt",&procesosIN,&iosIN,&precsIN)) return 1;
					}
					else
					{
						if (ap != 0 && aio == 0 && aprec != 0)
						{
							if (!cargarArchivos(argv[ap],"io.txt",argv[aprec],&procesosIN,&iosIN,&precsIN)) return 1;
						}
						else
						{
							if (ap != 0 && aio != 0 && aprec == 0)
							{
								if (!cargarArchivos(argv[ap],argv[aio],"precedencia.txt",&procesosIN,&iosIN,&precsIN)) return 1;
							}
							else
							{
								if (ap != 0 && aio != 0 && aprec != 0)
								{
									if (!cargarArchivos(argv[ap],argv[aio],argv[aprec],&procesosIN,&iosIN,&precsIN)) return 1;
								}
							}
						}
					}
				}
			}
		}
	}


	/*
	 * Validación de los datos
	 */

	printf("Validando los datos de entrada...\n");
	if (!validarDatos(procesosIN,iosIN,precsIN))
	{
		printf("Los datos de entrada son inválidos!\n");
		return 1;
	}
	else
	{
		printf("Los datos fueron validados con éxito!\n");
	}

	/*
	 * Comprobación de las listas de entrada (solo en desarrollo - comentar para publicación)
	 */
	PROCESOSIN pin=procesosIN;
	IOSIN iosin=iosIN;
	PRECSIN precsin=precsIN;

	while(pin != NULL)
	{
		printf("PROCESO\n");
		printf("PID %d\n",pin->pid);
		printf("ARRIBO %d\n",pin->arribo);
		printf("T CPU %d\n",pin->Tcpu);
		printf("\n");
		pin = pin->sig;
	}

	while(iosin != NULL)
	{
		printf("I/O\n");
		printf("PID %d\n",iosin->pid);
		printf("INSTANTE %d\n",iosin->instante);
		printf("T RES %d\n",iosin->tiempo);
		printf("\n");
		iosin = iosin->sig;
	}

	while(precsin != NULL)
	{
		printf("PRECEDENCIA\n");
		printf("PRECEDENTE %d\n",precsin->precedente);
		printf("PRECEDIDO %d\n",precsin->precedido);
		printf("\n");
		precsin = precsin->sig;
	}

	/*
	 *
	 * Inicializar los procesos que se van a simular
	 *
	 */

	pin = procesosIN;
	PROCESOS procesos=NULL,ultimo;

	while(pin != NULL)
	{
		if (procesos == NULL)
		{
			procesos = (PROCESOS)malloc(sizeof(PROCESO));
			procesos->pid = pin->pid;
			procesos->tcpu = pin->Tcpu;
			procesos->necesidad = pin->Tcpu;
			procesos->arribo = pin->arribo;
			procesos->turnAround = 0;
			procesos->tResp = 0;
			procesos->tEListos = 0;
			procesos->tESync = 0;
			procesos->tETotal = 0;
			procesos->tEDisp = 0;
			procesos->semaforos = cargarSemaforos(precsIN,pin->pid);
			procesos->sig = NULL;
			ultimo = procesos;
		}
		else
		{
			ultimo->sig = (PROCESOS)malloc(sizeof(PROCESO));
			ultimo = ultimo->sig;
			ultimo->pid = pin->pid;
			ultimo->tcpu = pin->Tcpu;
			ultimo->necesidad = pin->Tcpu;
			ultimo->arribo = pin->arribo;
			ultimo->turnAround = 0;
			ultimo->tResp = 0;
			ultimo->tEListos = 0;
			ultimo->tESync = 0;
			ultimo->tETotal = 0;
			ultimo->tEDisp = 0;
			ultimo->semaforos = cargarSemaforos(precsIN,pin->pid);
			ultimo->sig = NULL;
		}
		pin = pin->sig;
	}

	SEMAFOROS s;
	ultimo = procesos;
	while(ultimo!=NULL)
	{
		printf("PROCESO\n");
		printf("PID: %d\n",ultimo->pid);
		printf("NEC: %d\n",ultimo->necesidad);
		printf("Semaforos\n");
		s = ultimo->semaforos;
		while (s!=NULL)
		{
			printf("Tipo: %c\n",s->tipo);
			printf("Nombre: %c\n",s->nombre);
			printf("Estado: %d\n\n",s->valor);
			s = s->sig;
		}

		ultimo = ultimo->sig;
	}

	/*
	 *
	 * Creación de la lista de System Calls
	 *
	 */


	SCALLS systemcalls,actSC;
	EVENTOS eventos;


	systemcalls = systemCalls(procesos,iosIN);
	actSC = systemcalls;

	/* Comprobación de la lista de system calls * --- SOLO EN DESARROLLO - COMENTAR PARA PUBLICACIÓN */
	printf("\nTABLA DE SYSTEM CALLS\n");
	printf("----------------------\n");
	printf("PID\t\tINSTATNTE\tRESOL\t\tTIPO\t\tSEMAFORO\n");
	while (actSC != NULL)
	{
		printf("%d\t\t%d\t\t%d\t\t%s\t\t%c\n",actSC->pid,actSC->instante,actSC->resolucion,actSC->tipo,actSC->semaforo);
		actSC = actSC->sig;
	}


	/*
	 *
	 * Inicializar la lista de eventos con los arribos de los procesos
	 *
	 */
	EVENTOS ant=NULL,act,nuevo;
	eventos = NULL;
	pin=procesosIN;
	while(pin != NULL)
	{
		act = eventos;
		while(act != NULL && act->instante <= pin->arribo)
		{
			ant = act;
			act = act->sig;
		}
		if (eventos == NULL)
		{
			eventos = (EVENTOS) malloc(sizeof(EVENTO));
			eventos->pid=pin->pid;
			eventos->instante=pin->arribo;
			strcpy(eventos->codigo,"ANP");
			eventos->semaforo = '-';
			eventos->sig=NULL;
			//ultEvento = eventos;
		}
		else
		{
			nuevo = (EVENTOS) malloc(sizeof(EVENTO));
			//ultEvento = ultEvento->sig;
			nuevo->pid=pin->pid;
			nuevo->instante=pin->arribo;
			strcpy(nuevo->codigo,"ANP");
			nuevo->semaforo = '-';
			nuevo->sig=act;
			ant->sig = nuevo;
		}

		pin = pin->sig;
	}


	//Muestro la tabla inicial de eventos

	act = eventos;
	printf("TABLA DE EVENTOS INICIAL\n");
	printf("------------------------\n");
	while (act!=NULL)
	{
		printf("%d\t\t%d\t\t%s\t\t%c\n",act->pid,act->instante,act->codigo,act->semaforo);
		act = act->sig;
	}







	/*
	 *
	 * Menú de opciones del programa.
	 *
	 * */

	int opcion;
	int arp=0,are=0,ars=0;

	do
	{
		printf("Presione una tecla para volver al menú\n");
		getchar();getchar();
		system("clear");
		printf("******************************************************************\n");
		printf("           SIMULACIÓN DE PLANEAMIENTO A CORTO PLAZO\n");
		printf("******************************************************************\n");
		printf("\n");
		printf("\n");
		printf("Menu\n");
		printf("----\n\n\n");
		printf("1) Simulación paso a paso.\n");
		printf("2) Generar directamente resultados de la simulación\n");
		printf("3) Salir inmediatamente!\n");
		printf("\n");
		printf("\n");
		do
		{
			printf("Seleccione una opcion: ");
			scanf("%d",&opcion);
		}
		while (opcion != 1 && opcion != 2 && opcion != 3);
		switch (opcion)
		{
			case 1:
				system("clear");
				printf("Iniciando simulación paso a paso...\n");
				simular(procesos,systemcalls,eventos,1,&rp,&re,&rs);
				printf("Simulación paso a paso finalizada.\n");
				break;

			case 2:
				system("clear");
				printf("Iniciando simulación...\n");
				simular(procesos,systemcalls,eventos,0,&rp,&re,&rs);
				printf("Simulación finalizada.\n");
				break;
		}
		if (opcion != 3)
		{
			printf("Generando resultados...\n");
			//Busco las posiciones de los parámetros donde estan los archivos de entrada.
			for (i=1; i<argc; i+=2)
			{
				if (!strcmp(argv[i],"-rp")) arp=i+1;
				else
					if (!strcmp(argv[i],"-rs")) ars=i+1;
					else
						if (!strcmp(argv[i],"-ev")) are=i+1;
			}

			// Verifico todas las combinaciones de opciones de parámetros de salida disponibles
			// en base a si un parámetro de entrada esta o no.
			if (arp == 0 && ars == 0 && are == 0)
			{
				if (!escribirResultados("proc-ind.txt","sys-ind.txt","trace.txt",rp,rs,re)) return 1;
			}
			else
			{
				if (arp == 0 && ars == 0 && are != 0)
				{
					if (!escribirResultados("proc-ind.txt","sys-ind.txt",argv[are],rp,rs,re)) return 1;
				}
				else
				{
					if (arp == 0 && ars !=0 && are == 0)
					{
						if (!escribirResultados("proc-ind.txt",argv[ars],"trace.txt",rp,rs,re)) return 1;
					}
					else
					{
						if (arp == 0 && ars != 0 && are != 0)
						{
							if (!escribirResultados("proc-ind.txt",argv[ars],argv[are],rp,rs,re)) return 1;
						}
						else
						{
							if (arp != 0 && ars == 0 && are == 0)
							{
								if (!escribirResultados(argv[arp],"sys-ind.txt","trace.txt",rp,rs,re)) return 1;
							}
							else
							{
								if (arp != 0 && ars == 0 && are != 0)
								{
									if (!escribirResultados(argv[arp],"sys-ind.txt",argv[are],rp,rs,re)) return 1;
								}
								else
								{
									if (arp != 0 && ars != 0 && are == 0)
									{
										if (!escribirResultados(argv[arp],argv[ars],"trace.txt",rp,rs,re)) return 1;
									}
									else
									{
										if (arp != 0 && ars != 0 && are != 0)
										{
											if (!escribirResultados(argv[arp],argv[ars],argv[are],rp,rs,re)) return 1;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	while (opcion != 3);
	printf("Saliendo del progrma...\n");
	return 0;
}
