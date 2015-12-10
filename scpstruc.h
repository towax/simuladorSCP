#ifndef SCPSTRUC_H_INCLUDED
#define SCPSTRUC_H_INCLUDED

/*! \typedef PROCESOIN
 * Estructura para leer una línea del archivo de entrada de procesos (INPUT)
 */
typedef struct procesoIN
{
	char pid[5]; /**< PID */
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
	char pid[5]; /**< PID del proceso que solicita servicio */
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
	char pid[5]; /**< Identificador único del proceso */
	int tcpu; /**< Tiempo de CPU. Necesidad total del proceso */
	int necesidad; /**< Necesidad actual del proceso */
	int arribo; /** Instante de tiempo de llegada del proceso */
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
	float tRespP; /**< Tiempo de Respuesta Promedio */
	float tEListosP; /**< Tiempo de Espera en cola de Listos Promedio */
	float tESyncP; /**< Tiempo de Espera en cola de sincronización Promedio */
	float tEMutexP; /**< Tiempo de Espera en cola de bloqueados por exclusión mutua Promedio */
}SYSIND;


/*! \typedef EVENTO
 * Estructura para almacenar un evento de ocurrencia en la simulación (OUTPUT)
 */
typedef struct evento
{
	int instante; /**< Instante de ocurrencia del evento */
	char pid[5]; /**< PID que genero el evento */
	char codigo[4]; /**< Código del evento : ANP,SIO,FIO, FPR, P, V */
	char semaforo[3]; /**< Nombre del semáforo, en el caso de que el código se V o P */
	struct evento * sig; /**< Puntero al próximo evento */
}EVENTO;

/*! \typedef EVENTOS
 * Lista dinámica de eventos
 */
typedef EVENTO * EVENTOS;


/*! \typedef SEMÁFORO
 * Estructura para almacenar un semáforo
 */
typedef struct semaforo
{
	int valor; /**< Valor asociado al semáforo */
	PROCESOS bloqueados; /**< Cola de procesos bloqueados en el semáforo */
}SEMAFORO;



#endif // SCPSTRUC_H_INCLUDED
