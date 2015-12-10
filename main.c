/*

© 2015

Autores:
* Guayarello, Antonella.
* Moreira, Facundo.
* Massaccesi, Matias.
* Fritz, Axel.

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
* Jacué, Marcos

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

*/

/*! \file main.c*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "scpstruc.h"
#include "scpfunc.h"



/**
 * Función principal del programa.
 * @param argc Cantidad de argumentos recibidos en la ejecución
 * @param argv Arreglo con los argumentos recibidos en la ejecución
 * @return Devuelve 0 (cero) si el programa finaliza con éxito
 */
int main(int argc, char *argv[])
{

    EVENTOS eventos;
	PROCESOS rp=NULL;
	EVENTOS re=NULL;
	SYSIND rs;
	PROCESOSIN pin;

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
	int ap = 0,asc = 0,buf = 0;

    // Inicializo las listas para cargar los archivos de entrada
	PROCESOSIN procesosIN = NULL;
	SCALLS scallsIN = NULL;


	printf("Cargando archivos de entrada...\n");

	// Busco las posiciones de los valores de los parámetros
	int i;
	for (i=1; i<argc; i+=2)
	{
		if (!strcmp(argv[i],"-p")) ap=i+1;
		else
			if (!strcmp(argv[i],"-sc")) asc=i+1;
			else
				if (!strcmp(argv[i],"-buf")) buf=i+1;
	}

	printf("Parámetros de entrada: %d - %d - %d\n",ap,asc,buf);

	// Verifico todas las combinaciones de opciones de parámetros de entrada (solo archivos) disponibles
	// en base a si un parámetro de entrada esta o no.
	if (ap == 0 && asc == 0)
	{
		if (!cargarArchivos("procs.txt","scs.txt",&procesosIN,&scallsIN)) return 1;
	}
	else
	{
		if (ap == 0 && asc != 0)
		{
			if (!cargarArchivos("procs.txt",argv[asc],&procesosIN,&scallsIN)) return 1;
		}
		else
		{
			if (ap != 0 && asc ==0)
			{
				if (!cargarArchivos(argv[ap],"scs.txt",&procesosIN,&scallsIN)) return 1;
			}
			else
			{
				if (ap != 0 && asc != 0)
				{
					if (!cargarArchivos(argv[ap],argv[asc],&procesosIN,&scallsIN)) return 1;
				}
			}
		}
	}


	/*
	 * Validación de los datos
	 */

	printf("Validando los datos de entrada...\n");
	if (!validarDatos(procesosIN,scallsIN))
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
//	pin = procesosIN;
//	SCALLS scalls = scallsIN;
//
//	while(pin != NULL)
//	{
//		printf("PROCESO\n");
//		printf("PID %s\n",pin->pid);
//		printf("ARRIBO %d\n",pin->arribo);
//		printf("T CPU %d\n",pin->Tcpu);
//		printf("\n");
//		pin = pin->sig;
//	}
//
//	while(scalls != NULL)
//	{
//		printf("SCALL\n");
//		printf("PID %s\n",scalls->pid);
//		printf("INSTANTE %d\n",scalls->instante);
//		printf("T RES %d\n",scalls->resolucion);
//		printf("TIPO %s\n",scalls->tipo);
//      printf("SEMAFORO %s\n",scalls->semaforo);
//		printf("\n");
//		scalls = scalls->sig;
//	}



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
			strcpy(procesos->pid,pin->pid);
			procesos->tcpu = pin->Tcpu;
			procesos->necesidad = pin->Tcpu;
			procesos->arribo = pin->arribo;
			procesos->turnAround = 0;
			procesos->tResp = 0;
			procesos->tEListos = 0;
			procesos->tESync = 0;
			procesos->tETotal = 0;
			procesos->tEDisp = 0;
			procesos->sig = NULL;
			ultimo = procesos;
		}
		else
		{
			ultimo->sig = (PROCESOS)malloc(sizeof(PROCESO));
			ultimo = ultimo->sig;
			strcpy(ultimo->pid,pin->pid);
			ultimo->tcpu = pin->Tcpu;
			ultimo->necesidad = pin->Tcpu;
			ultimo->arribo = pin->arribo;
			ultimo->turnAround = 0;
			ultimo->tResp = 0;
			ultimo->tEListos = 0;
			ultimo->tESync = 0;
			ultimo->tETotal = 0;
			ultimo->tEDisp = 0;
			ultimo->sig = NULL;
		}
		pin = pin->sig;
	}

    /*
	 * Comprobación de las listas de procesos de sistema (solo en desarrollo - comentar para publicación)
	 */
//	ultimo = procesos;
//	while(ultimo!=NULL)
//	{
//		printf("PROCESO\n");
//		printf("PID: %s\n",ultimo->pid);
//		printf("NEC: %d\n",ultimo->necesidad);
//
//		ultimo = ultimo->sig;
//	}


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
			strcpy(eventos->pid,pin->pid);
			eventos->instante=pin->arribo;
			strcpy(eventos->codigo,"ANP");
			eventos->sig=NULL;
		}
		else
		{
			nuevo = (EVENTOS) malloc(sizeof(EVENTO));
			strcpy(nuevo->pid,pin->pid);
			nuevo->instante=pin->arribo;
			strcpy(nuevo->codigo,"ANP");
			nuevo->sig=act;
			ant->sig = nuevo;
		}

		pin = pin->sig;
	}



    /*
	 * Comprobación de la lista inicial de eventos (solo en desarrollo - comentar para publicación)
	 */
//	act = eventos;
//	printf("TABLA DE EVENTOS INICIAL\n");
//	printf("------------------------\n");
//	while (act!=NULL)
//	{
//		printf("%s\t\t%d\t\t%s\t\t%s\n",act->pid,act->instante,act->codigo,act->semaforo);
//		act = act->sig;
//	}




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
				simular(procesos,scallsIN,eventos,1,&rp,&re,&rs);
				printf("Simulación paso a paso finalizada.\n");
				break;

			case 2:
				system("clear");
				printf("Iniciando simulación...\n");
				simular(procesos,scallsIN,eventos,0,&rp,&re,&rs);
				printf("Simulación finalizada.\n");
				break;
		}
		if (opcion != 3)
		{

			printf("Generando resultados...\n");
			//Busco las posiciones de los parámetros donde estan los archivos de salida.
			for (i=1; i<argc; i+=2)
			{
				if (!strcmp(argv[i],"-rp")) arp=i+1;
				else
					if (!strcmp(argv[i],"-rs")) ars=i+1;
					else
						if (!strcmp(argv[i],"-ev")) are=i+1;
			}

			// Verifico todas las combinaciones de opciones de parámetros de salida (solo archivos) disponibles
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
