/*
 * ============================================================================
 *  CONTROL 01 - SISTEMAS OPERATIVOS 2026-II
 *  proceso_c.c  ->  ejecutable independiente lanzado por el clon de B
 * ============================================================================
 *
 *  Este programa NO se ejecuta a mano. Lo lanza el clon de B con:
 *      execv("./proceso_c", { "./proceso_c", "<fd>", NULL })
 *
 *  Tras el execv:
 *    - su PID es el MISMO que tenia el clon de B (exec no crea proceso)
 *    - su padre sigue siendo B
 *    - conserva abierto el descriptor de lectura del pipe
 *    - NO conserva ninguna variable: por eso necesita que le digan por
 *      argv que numero de descriptor tiene el pipe
 *
 *  Requisitos que cumple este archivo:
 *    [C2] recibe por argv el descriptor de lectura del pipe
 *    [C3] permanece a la escucha de la tuberia
 *    [C4] al recibir un mensaje, lo imprime con su PID y el canal
 *
 *  Compilar ANTES de ejecutar proceso_a:
 *    gcc -Wall -Wextra proceso_c.c -o proceso_c
 * ============================================================================
 */

#include <stdio.h>      /* printf, fprintf, perror, fflush  */
#include <stdlib.h>     /* atoi                              */
#include <unistd.h>     /* read, close, getpid, getppid      */
#include <errno.h>      /* errno, EINTR                      */
#include <sys/types.h>  /* ssize_t                           */

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;

    /* argv[0] = "./proceso_c" (lo puso el clon)
     * argv[1] = el descriptor en texto, por ejemplo "3" */
    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <descriptor de lectura>\n", argv[0]);
        fprintf(stderr, "este programa lo lanza proceso_a, no se ejecuta solo\n");
        return 1;
    }

    /* [C2] Texto -> numero. Camino inverso del snprintf del clon */
    fd = atoi(argv[1]);

    printf("[C %d] soy proceso_c (padre B = %d), escucho el PIPE por el fd %d\n",
           (int) getpid(), (int) getppid(), fd);
    fflush(stdout);

    /* [C3] Escuchar la tuberia. read es BLOQUEANTE: C duerme hasta que
     * A escriba algo.
     *   read > 0  -> llegaron datos
     *   read = 0  -> EOF: todos los escritores cerraron (A termino)
     *   read < 0  -> error */
    while (1)
    {
        n = read(fd, buf, MAXBUF - 1);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;           /* interrumpido por una senal: reintentar */
            perror("read");
            return 1;
        }

        if (n == 0)
            break;                  /* EOF */

        buf[n] = '\0';              /* read no pone el terminador */

        /* [C4] Imprimir el mensaje con PID y canal de recepcion */
        printf("[C %d] recibido por PIPE (fd %d, %zd bytes): \"%s\"\n",
               (int) getpid(), fd, n, buf);
        fflush(stdout);
    }

    printf("[C %d] EOF: A cerro el pipe, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
