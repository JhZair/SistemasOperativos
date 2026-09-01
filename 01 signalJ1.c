/*
 * Laboratorio 01 - Sistemas Operativos
 * Signaling y Saltos No Locales (sigsetjmp / siglongjmp)
 *
 * Compilar:  gcc -Wall -Wextra ejercicio2.c -o ejercicio2
 * Ejecutar:  ./ejercicio2
 *
 * Terminal 1: ./ejercicio2        (muestra su PID)
 * Terminal 2: kill -SIGUSR1 <PID>
 *             kill -SIGUSR2 <PID>
 * Terminal 1: Ctrl+C
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

/* Códigos que identifican QUIÉN provocó el salto no local.
 * Deben ser distintos de 0, porque 0 es el valor que devuelve
 * sigsetjmp() cuando se ejecuta por primera vez (llamada directa). */
#define SALTO_SIGINT   1
#define SALTO_SIGUSR1  2
#define SALTO_SIGUSR2  3

/* Búfer donde sigsetjmp() guarda el contexto de ejecución:
 * puntero de pila, contador de programa, registros y máscara de señales. */
static sigjmp_buf punto_salto;

/* Única variable compartida entre los manejadores y main().
 * volatile  -> el compilador no puede cachearla en un registro,
 *              porque puede cambiar "por sorpresa" de forma asíncrona.
 * sig_atomic_t -> tipo cuya lectura/escritura es atómica: no puede
 *              quedar a medias si llega otra señal en ese instante. */
static volatile sig_atomic_t ultima_senal = 0;

/* ------------------------------------------------------------------ */
/* MANEJADOR 1: SIGINT (Ctrl+C)                                        */
/* ------------------------------------------------------------------ */
static void manejador_int(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_SIGINT);
}

/* ------------------------------------------------------------------ */
/* MANEJADOR 2: SIGUSR1 (kill -SIGUSR1 <PID>)                          */
/* ------------------------------------------------------------------ */
static void manejador_usr1(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_SIGUSR1);
}

/* ------------------------------------------------------------------ */
/* MANEJADOR 3: SIGUSR2 (kill -SIGUSR2 <PID>)                          */
/* ------------------------------------------------------------------ */
static void manejador_usr2(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_SIGUSR2);
}

int main(void)
{
    int origen;
    int pid = (int) getpid();

    /* Registro de los 3 manejadores. A partir de aquí, el kernel sabe
     * que si llega alguna de estas señales NO debe aplicar la acción
     * por defecto (terminar el proceso), sino llamar a nuestra función. */
    signal(SIGINT,  manejador_int);
    signal(SIGUSR1, manejador_usr1);
    signal(SIGUSR2, manejador_usr2);

    /* ================= ETIQUETA DE RETORNO ==========================
     * Primera vez (llamada directa)   -> devuelve 0
     * Vuelta desde siglongjmp(buf, n) -> devuelve n
     * El segundo argumento (1) indica que TAMBIÉN se guarde/restaure
     * la máscara de señales; sin él, tras el primer salto la señal
     * quedaría bloqueada para siempre. */
    origen = sigsetjmp(punto_salto, 1);

    if (origen == 0) {
        printf("Programa iniciado. PID = %d\n", pid);
        printf("  Ctrl+C               -> SIGINT  (%d)\n", SIGINT);
        printf("  kill -SIGUSR1 %-7d-> SIGUSR1 (%d)\n", pid, SIGUSR1);
        printf("  kill -SIGUSR2 %-7d-> SIGUSR2 (%d)  [cierre ordenado]\n",
               pid, SIGUSR2);
        printf("----------------------------------------------------\n");
    }
    else if (origen == SALTO_SIGINT) {
        printf("\n[SALTO %d] Se recibió la señal %d (SIGINT).\n",
               origen, (int) ultima_senal);
        printf("Interrupción de teclado ignorada: se reanuda el bucle.\n");
    }
    else if (origen == SALTO_SIGUSR1) {
        printf("\n[SALTO %d] Se recibió la señal %d (SIGUSR1).\n",
               origen, (int) ultima_senal);
        printf("Simulando recarga de configuración... hecho.\n");
    }
    else if (origen == SALTO_SIGUSR2) {
        printf("\n[SALTO %d] Se recibió la señal %d (SIGUSR2).\n",
               origen, (int) ultima_senal);
        printf("Cierre ordenado solicitado. Liberando recursos...\n");
        printf("Fin del programa.\n");
        return 0;
    }

    /* El bucle queda FUERA del if: así, venga de donde venga el salto,
     * la ejecución continúa normalmente desde la etiqueta. */
    while (1) {
        printf("Ejecutando código normal...\n");
        sleep(2);
    }

    return 0;   /* inalcanzable: solo se sale por SIGUSR2 */
}