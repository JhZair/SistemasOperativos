#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CLAVE   1234
#define MAXSIZE 128

struct buzon
{
    long mtype;
    char mtext[MAXSIZE];
};

static volatile sig_atomic_t salir = 0;

static void die(const char *s)
{
    perror(s);
    exit(1);
}

static void pedir_salida(int signo)
{
    (void) signo;
    salir = 1;
}

int main(void)
{
    int msqid;
    struct buzon rcvbuffer;
    ssize_t n;
    long contador = 0;

    signal(SIGINT, pedir_salida);

    if ((msqid = msgget(CLAVE, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("=========================================================\n");
    printf("[receptor] PID .............. %d\n", (int) getpid());
    printf("[receptor] cola IPC (msqid) . %d   (clave %d)\n", msqid, CLAVE);
    printf("[receptor] esperando mensajes de tipo 1, 2 o 3...\n");
    printf("[receptor] Ctrl+C para terminar y eliminar la cola\n");
    printf("=========================================================\n");
    fflush(stdout);

    while (!salir)
    {
        n = msgrcv(msqid, &rcvbuffer, MAXSIZE, 0, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("msgrcv");
        }

        contador++;
        printf("\n[receptor] #%ld  mensaje TIPO %ld  (%zd bytes)\n",
               contador, rcvbuffer.mtype, n);
        printf("[receptor] contenido: \"%s\"\n", rcvbuffer.mtext);

        switch (rcvbuffer.mtype)
        {
            case 1:
                printf("[receptor] procesando tarea de tipo 1...\n");
                break;
            case 2:
                printf("[receptor] procesando tarea de tipo 2...\n");
                break;
            case 3:
                printf("[receptor] procesando tarea de tipo 3...\n");
                break;
            default:
                printf("[receptor] tipo desconocido, se descarta\n");
                break;
        }

        fflush(stdout);
        sleep(5);
        printf("[receptor] listo. Siguiente mensaje.\n");
        fflush(stdout);
    }

    if (msgctl(msqid, IPC_RMID, NULL) < 0)
        die("msgctl");

    printf("\n[receptor] cola %d eliminada. %ld mensajes procesados.\n",
           msqid, contador);

    return 0;
}
