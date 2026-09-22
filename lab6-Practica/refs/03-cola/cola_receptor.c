/*
 * COLAS DE MENSAJES - RECEPTOR
 *
 * Uso:  ./cola_receptor [tipo]
 *       ./cola_receptor        -> tipo 0: el primero de la cola, sea cual sea
 *       ./cola_receptor 3      -> solo mensajes de tipo 3
 *       ./cola_receptor -3     -> el de MENOR tipo entre 1..3 (prioridad)
 *
 * msgrcv(msqid, &msg, MAXTEXT, tipo, banderas)
 *   MAXTEXT = capacidad maxima que acepto (no lo que envio).
 *   banderas 0 = BLOQUEANTE: duerme hasta que haya mensaje.
 *   IPC_NOWAIT = si no hay, falla con ENOMSG.
 *   Devuelve los bytes copiados en mtext, o -1.
 *
 * EINTR: si llega una senal mientras espera, msgrcv falla con EINTR
 *   y NUNCA se reinicia solo (ni con SA_RESTART). Hay que reintentar.
 *
 * Ctrl+C: sale ordenadamente y borra la cola con msgctl(IPC_RMID).
 *   Las colas System V SOBREVIVEN al proceso: si no se borran quedan
 *   en el kernel. Ver con "ipcs -q", borrar con "ipcrm -Q 1234".
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "cola_comun.h"

static volatile sig_atomic_t salir = 0;

static void die(const char *s) { perror(s); exit(1); }
static void h_salir(int s)     { (void) s; salir = 1; }

int main(int argc, char *argv[])
{
    int msqid;
    long tipo = 0;
    struct mensaje m;
    ssize_t n;

    if (argc == 2)
        tipo = atol(argv[1]);

    signal(SIGINT, h_salir);

    if ((msqid = msgget(CLAVE, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("PID %d, cola %d (clave %d), esperando tipo %ld (0 = cualquiera)\n",
           (int) getpid(), msqid, CLAVE, tipo);
    printf("Ctrl+C para salir y borrar la cola\n");
    fflush(stdout);

    while (!salir)
    {
        n = msgrcv(msqid, &m, MAXTEXT, tipo, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;           /* fue una senal, no un error */
            die("msgrcv");
        }

        printf("recibido <- tipo %ld, %zd bytes: \"%s\"\n", m.mtype, n, m.mtext);
        fflush(stdout);
    }

    if (msgctl(msqid, IPC_RMID, NULL) < 0)
        die("msgctl");
    printf("cola %d eliminada\n", msqid);
    return 0;
}
