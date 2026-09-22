/*
 * FORK - CASO 1: crear un hijo e identificar quien es quien
 *
 * fork() clona el proceso. Tras la llamada hay DOS procesos ejecutando
 * la misma linea siguiente. Se distinguen por el valor de retorno:
 *
 *   en el PADRE  -> devuelve el PID del hijo (un numero > 0, ej. 4817)
 *   en el HIJO   -> devuelve 0
 *   si falla     -> devuelve -1 (solo en el padre, no hubo hijo)
 *
 * NO devuelve 1 en el padre. Error frecuente en apuntes.
 *
 * El hijo hereda: codigo, datos (copia), descriptores abiertos,
 * manejadores de senal, directorio actual, el buffer de printf...
 *
 * Probar: ./fork_basico
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;

    printf("antes del fork: PID %d\n", (int) getpid());
    fflush(stdout);         /* SIN esto, el texto pendiente en el buffer */
                            /* se duplica en el hijo */

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0)
    {
        /* ---- HIJO ---- */
        printf("HIJO : PID %d, mi padre es %d, fork me dio %d\n",
               (int) getpid(), (int) getppid(), (int) pid);
        fflush(stdout);
        exit(0);            /* el hijo termina aqui; no sigue a la parte del padre */
    }

    /* ---- PADRE ---- */
    printf("PADRE: PID %d, mi hijo es %d, fork me dio %d\n",
           (int) getpid(), (int) pid, (int) pid);
    fflush(stdout);

    /* wait(NULL): espera a que termine CUALQUIER hijo.
     * Sin wait, el hijo terminado queda como ZOMBI hasta que el padre muera.
     * Si el padre muere antes que el hijo, el hijo queda HUERFANO y lo
     * adopta init (PID 1). */
    wait(NULL);
    printf("PADRE: el hijo termino\n");

    return 0;
}
