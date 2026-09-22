/*
 * SENALES - CASO 2: el patron seguro (bandera + trabajo en main)
 *
 * Dentro de un manejador solo es seguro:
 *   - escribir en una variable volatile sig_atomic_t
 *   - llamar a write(), _exit(), kill() y pocas mas
 * NO es seguro: printf, malloc, msgsnd, exit, fopen...
 *
 * El manejador solo anota que senal llego. main() hace el trabajo.
 * Es el patron que hay que usar cuando tras la senal se hace IPC.
 *
 * Probar: igual que senal_basica.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* volatile:      el compilador no puede cachearla en un registro
 * sig_atomic_t:  escritura y lectura indivisibles */
static volatile sig_atomic_t senal_pendiente = 0;

static void manejador(int signo)
{
    senal_pendiente = signo;
}

int main(void)
{
    signal(SIGINT,  manejador);
    signal(SIGUSR1, manejador);
    signal(SIGTERM, manejador);

    printf("PID %d\n", (int) getpid());
    fflush(stdout);

    while (1)
    {
        pause();    /* retorna cuando un manejador termina */

        /* Aqui estamos en el flujo normal: todo es seguro */
        switch (senal_pendiente)
        {
            case SIGINT:
                printf("main: atendiendo SIGINT\n");
                break;
            case SIGUSR1:
                printf("main: atendiendo SIGUSR1\n");
                break;
            case SIGTERM:
                printf("main: saliendo\n");
                exit(0);
        }
        fflush(stdout);
        senal_pendiente = 0;
    }

    return 0;
}
