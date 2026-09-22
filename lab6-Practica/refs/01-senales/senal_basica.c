/*
 * SENALES - CASO 1: registrar manejadores con signal()
 *
 * Que demuestra:
 *   - registrar un mismo manejador para varias senales
 *   - distinguirlas por el parametro signo
 *   - esperar sin consumir CPU con pause()
 *   - que el proceso NO muere con Ctrl+C
 *
 * Probar:
 *   ./senal_basica            (en una terminal, anotar el PID)
 *   kill -2  <PID>            (o Ctrl+C en su terminal)
 *   kill -10 <PID>
 *   kill -12 <PID>
 *   kill -15 <PID>            (SIGTERM: sale ordenadamente)
 *
 * Numeros de senal mas usados en el curso:
 *   2  SIGINT   Ctrl+C
 *   5  SIGTRAP  (la usa el control)
 *   10 SIGUSR1  libre para el programador
 *   12 SIGUSR2  libre para el programador
 *   15 SIGTERM  terminar ordenadamente
 *   9  SIGKILL  NO se puede atrapar
 *   19 SIGSTOP  NO se puede atrapar
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* La firma es obligatoria: void nombre(int).
 * El kernel pasa el numero de senal en signo. */
static void manejador(int signo)
{
    /* printf dentro de un manejador es ACEPTABLE en laboratorio
     * pero no es async-signal-safe. Ver senal_flag.c para el patron seguro. */
    printf("senal %d recibida\n", signo);
    fflush(stdout);

    if (signo == SIGTERM)
    {
        printf("saliendo\n");
        exit(0);
    }
}

int main(void)
{
    /* signal(numero, funcion): sustituye la accion por defecto.
     * Devuelve el manejador anterior o SIG_ERR. */
    signal(SIGINT,  manejador);
    signal(SIGUSR1, manejador);
    signal(SIGUSR2, manejador);
    signal(SIGTERM, manejador);

    printf("PID %d esperando senales (2, 10, 12; 15 para salir)\n",
           (int) getpid());
    fflush(stdout);

    /* pause() duerme hasta que llegue una senal ATRAPADA.
     * Tras ejecutar el manejador, pause() retorna y el bucle repite. */
    while (1)
        pause();

    return 0;
}
