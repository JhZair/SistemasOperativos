/*
 * EXECV - CASO 1: las variantes de exec y que hacen
 *
 * exec REEMPLAZA la imagen del proceso actual por otro programa.
 * Mismo PID, mismos descriptores abiertos, pero codigo, datos y pila
 * nuevos. Si exec tiene exito, NUNCA retorna: la linea siguiente no
 * existe. Solo retorna (-1) si fallo.
 *
 * Se conservan tras exec:  descriptores abiertos (salvo FD_CLOEXEC),
 *                          PID, PPID, directorio actual, senales IGNORADAS
 * Se pierden tras exec:    manejadores de senal (vuelven a SIG_DFL),
 *                          memoria, variables, buffer de printf
 *
 * Variantes (la letra dice como se pasan los argumentos):
 *   execl ("/bin/ls", "ls", "-l", NULL)          l = Lista de argumentos
 *   execv ("/bin/ls", args)                      v = Vector (arreglo)
 *   execlp("ls", "ls", "-l", NULL)               p = busca en el PATH
 *   execvp("ls", args)
 *
 * El primer argumento es la RUTA del ejecutable.
 * El segundo (o args[0]) es lo que el programa vera como argv[0]:
 * por convencion su propio nombre. Por eso "se pone dos veces".
 * El NULL final es obligatorio: marca el fin de la lista.
 *
 * Probar: ./exec_simple
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;

    printf("padre %d: voy a crear un hijo que se convierta en 'ls -l'\n",
           (int) getpid());
    fflush(stdout);         /* obligatorio: exec descarta el buffer */

    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        char *args[] = { "ls", "-l", "/tmp", NULL };

        printf("hijo %d: antes de exec\n", (int) getpid());
        fflush(stdout);

        execv("/bin/ls", args);

        /* Solo se llega aqui si execv FALLO */
        perror("execv");
        exit(1);
    }

    wait(NULL);
    printf("padre: el hijo (ya convertido en ls) termino\n");
    return 0;
}
