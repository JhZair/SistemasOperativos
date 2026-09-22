/*
 * EXECV - CASO 2: pasar un descriptor de pipe al programa nuevo
 *
 * Problema: el programa lanzado con exec no sabe que descriptor le
 * corresponde. Solucion: pasarselo como argumento de texto.
 *
 *   padre:  snprintf(texto, "%d", fd[0])   numero -> texto
 *   hijo:   fd = atoi(argv[1])             texto  -> numero
 *
 * Por que funciona: los descriptores SOBREVIVEN a exec (no tienen
 * FD_CLOEXEC), asi que el numero sigue siendo valido en el programa
 * nuevo aunque este no sepa nada de pipe().
 *
 * Compilar ambos:
 *   gcc -Wall -Wextra exec_padre.c -o exec_padre
 *   gcc -Wall -Wextra exec_hijo.c  -o exec_hijo
 * Probar: ./exec_padre     (lanza el a exec_hijo solo)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    pid_t pid;
    const char *msg = "descriptor pasado por argv";

    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    fflush(stdout);
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        char texto_fd[16];

        close(fd[1]);                   /* el nuevo programa solo leera */

        snprintf(texto_fd, sizeof(texto_fd), "%d", fd[0]);

        char *args[] = { "./exec_hijo", texto_fd, NULL };
        /*                  argv[0]     argv[1]  fin   */

        execv("./exec_hijo", args);
        perror("execv");                /* solo si fallo */
        exit(1);
    }

    close(fd[0]);
    write(fd[1], msg, strlen(msg));
    printf("PADRE %d: escrito en fd %d, el hijo leera del fd %d\n",
           (int) getpid(), fd[1], fd[0]);
    close(fd[1]);                       /* EOF para el lector */

    wait(NULL);
    return 0;
}
