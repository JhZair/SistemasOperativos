/*
 * SENALES - CASO 3: sigaction() (la forma moderna y portable)
 *
 * El control menciona "signal / sigaction". sigaction:
 *   - tiene semantica definida (signal() varia entre sistemas)
 *   - permite saber QUIEN envio la senal (PID del emisor)
 *   - permite controlar si las llamadas al sistema se reinician (SA_RESTART)
 *
 * Probar:
 *   ./senal_sigaction
 *   kill -2 <PID>     -> imprime el PID de la shell que hizo el kill
 *   kill -5 <PID>     -> SIGTRAP, la senal del control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/* Con SA_SIGINFO la firma cambia: recibe ademas un siginfo_t*
 * con datos del emisor. */
static void manejador(int signo, siginfo_t *info, void *contexto)
{
    (void) contexto;
    printf("senal %d recibida, enviada por el PID %d\n",
           signo, (int) info->si_pid);
    fflush(stdout);
}

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));         /* limpiar TODA la estructura */
    sa.sa_sigaction = manejador;        /* manejador con informacion   */
    sa.sa_flags     = SA_SIGINFO;       /* activa la firma de 3 args   */
    sigemptyset(&sa.sa_mask);           /* no bloquear otras senales   */
                                        /* mientras corre el manejador */

    /* Para un manejador simple void f(int) seria:
     *   sa.sa_handler = f;
     *   sa.sa_flags   = 0;           (o SA_RESTART)
     */

    if (sigaction(SIGINT,  &sa, NULL) < 0) { perror("sigaction"); exit(1); }
    if (sigaction(SIGTRAP, &sa, NULL) < 0) { perror("sigaction"); exit(1); }
    if (sigaction(SIGUSR1, &sa, NULL) < 0) { perror("sigaction"); exit(1); }

    printf("PID %d esperando (kill -2 / -5 / -10)\n", (int) getpid());
    fflush(stdout);

    while (1)
        pause();

    return 0;
}
