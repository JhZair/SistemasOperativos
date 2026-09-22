/*
 * FORK - CASO 4: que pasa con las senales al hacer fork()
 *
 * REGLA: el hijo HEREDA los manejadores registrados antes del fork.
 * Consecuencia: si registras signal(SIGINT, h) y luego haces fork,
 * un "kill -2" al hijo tambien ejecuta h... en el hijo.
 *
 * Para que solo el padre reaccione hay dos opciones:
 *   a) registrar los manejadores DESPUES del fork (solo en el padre)
 *   b) en el hijo, restaurar: signal(SIGINT, SIG_DFL)
 *
 * Este programa muestra ambas cosas. Probar:
 *   ./fork_senales
 *   kill -10 <PID padre>  -> el padre imprime
 *   kill -10 <PID hijo>   -> el hijo NO tiene manejador: SIGUSR1 lo mata
 *   ps -o pid,stat,cmd -p <PID hijo>
 *                         -> aparece con STAT "Z" y "<defunct>": es un ZOMBI.
 *                            Murio, pero el padre no hizo wait() y el kernel
 *                            guarda su entrada hasta que alguien la recoja.
 *                            Un zombi NO consume CPU ni memoria, solo un PID.
 *   kill -15 <PID padre>  -> termina todo (init adopta y limpia al zombi)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static void h_padre(int s)
{
    printf("PADRE %d: recibi la senal %d\n", (int) getpid(), s);
    fflush(stdout);
}

static void h_fin(int s)
{
    (void) s;
    printf("PADRE: saliendo\n");
    exit(0);
}

int main(void)
{
    pid_t hijo;

    fflush(stdout);
    hijo = fork();
    if (hijo < 0) { perror("fork"); exit(1); }

    if (hijo == 0)
    {
        /* Hijo: no registra nada. Un SIGUSR1 le aplica la accion
         * por defecto (terminar). Se queda esperando. */
        printf("HIJO  %d sin manejadores\n", (int) getpid());
        fflush(stdout);
        while (1)
            pause();
    }

    /* Registrar DESPUES del fork: el hijo no los hereda */
    signal(SIGUSR1, h_padre);
    signal(SIGINT,  h_padre);
    signal(SIGTERM, h_fin);

    printf("PADRE %d con manejadores (kill -10 %d / kill -10 %d)\n",
           (int) getpid(), (int) getpid(), (int) hijo);
    fflush(stdout);

    while (1)
        pause();

    return 0;
}
