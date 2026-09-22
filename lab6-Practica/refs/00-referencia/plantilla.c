/*
 * PLANTILLA BASE
 * Esqueleto minimo para cualquier programa del curso.
 * Copiar, renombrar y borrar lo que no se use.
 *
 * Compilar:  gcc -Wall -Wextra plantilla.c -o plantilla
 */

#include <stdio.h>      /* printf, perror, fflush, snprintf          */
#include <stdlib.h>     /* exit, atoi                                 */
#include <string.h>     /* strlen, strcmp, memset                     */
#include <unistd.h>     /* fork, pipe, read, write, close, execv,     */
                        /* dup2, getpid, getppid, sleep, pause        */
#include <errno.h>      /* errno, EINTR, EEXIST, EAGAIN               */
#include <signal.h>     /* signal, sigaction, kill, SIGINT...         */
#include <sys/types.h>  /* pid_t, ssize_t, key_t                      */
#include <sys/wait.h>   /* wait, waitpid, WEXITSTATUS                 */

/* Patron universal de error del curso:
 * toda llamada al sistema devuelve -1 si falla y deja el motivo en errno.
 * perror() imprime "texto: motivo legible".
 * (sin static para que no avise si no se usa) */
void die(const char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char *argv[])
{
    (void) argc;    /* evita el aviso "unused parameter" si no se usan */
    (void) argv;

    printf("PID %d, padre %d\n", (int) getpid(), (int) getppid());
    fflush(stdout); /* SIEMPRE antes de fork() o execv() */

    return 0;
}
