/*
 * SENALES - CASO 4: enviar senales desde C con kill()
 *
 * kill() no solo mata: envia CUALQUIER senal a CUALQUIER proceso.
 * Es el equivalente en C del comando kill de la terminal.
 *
 * Uso:  ./senal_enviar <PID> <numero_senal>
 * Ej:   ./senal_enviar 4321 10
 *
 * Casos especiales de kill():
 *   kill(pid, 0)   -> no envia nada, solo comprueba si el proceso existe
 *   kill(getpid(), SIGUSR1) -> enviarse una senal a si mismo (= raise())
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    pid_t destino;
    int   senal;

    if (argc != 3)
    {
        fprintf(stderr, "uso: %s <PID> <senal>\n", argv[0]);
        return 1;
    }

    destino = (pid_t) atoi(argv[1]);
    senal   = atoi(argv[2]);

    if (kill(destino, 0) < 0)
    {
        perror("el proceso no existe o no hay permiso");
        return 1;
    }

    if (kill(destino, senal) < 0)
    {
        perror("kill");
        return 1;
    }

    printf("senal %d enviada al PID %d\n", senal, (int) destino);
    return 0;
}
