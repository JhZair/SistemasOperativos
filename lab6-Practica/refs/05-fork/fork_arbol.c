/*
 * FORK - CASO 3: arbol de tres niveles (padre -> hijo -> nieto)
 *
 * Es la jerarquia del control: A crea a B, B crea a su clon.
 * Cada nivel hace su propio fork() y espera a su descendiente.
 *
 * Verlo desde otra terminal mientras corre:
 *   ps -o pid,ppid,cmd | grep fork_arbol
 *   pstree -p <PID del padre>
 *
 * Probar: ./fork_arbol
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t hijo, nieto;

    printf("PADRE  %d\n", (int) getpid());
    fflush(stdout);

    hijo = fork();
    if (hijo < 0) { perror("fork"); exit(1); }

    if (hijo == 0)
    {
        /* ---- HIJO ---- */
        printf("HIJO   %d (padre %d)\n", (int) getpid(), (int) getppid());
        fflush(stdout);

        nieto = fork();
        if (nieto < 0) { perror("fork"); exit(1); }

        if (nieto == 0)
        {
            /* ---- NIETO ---- */
            printf("NIETO  %d (padre %d)\n", (int) getpid(), (int) getppid());
            fflush(stdout);
            sleep(3);   /* tiempo para verlo con ps / pstree */
            exit(0);
        }

        wait(NULL);     /* el hijo espera al nieto */
        printf("HIJO   %d: mi nieto termino\n", (int) getpid());
        exit(0);
    }

    wait(NULL);         /* el padre espera al hijo */
    printf("PADRE  %d: mi hijo termino\n", (int) getpid());
    return 0;
}
