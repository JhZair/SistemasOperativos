#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/* CAMBIO V2: C recibe por la COLA. Tiene que declarar la MISMA
 * estructura y usar el MISMO tipo que A. */
#define TIPO_MSG 1
#define MAXTEXT  256

struct mensaje
{
    long mtype;
    char mtext[MAXTEXT];
};

int main(int argc, char *argv[])
{
    int msqid;
    struct mensaje m;
    ssize_t n;

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <clave_cola>\n", argv[0]);
        return 1;
    }

    /* CAMBIO V2: argv[1] es la CLAVE. Sin IPC_CREAT: la cola ya existe
     * (la creo A), solo la localizamos. */
    if ((msqid = msgget((key_t) atoi(argv[1]), 0666)) < 0)
    {
        perror("msgget");
        return 1;
    }
    printf("[C %d] escuchando la COLA %d (clave %s)\n",
           (int) getpid(), msqid, argv[1]);
    fflush(stdout);

    while (1)
    {
        n = msgrcv(msqid, &m, MAXTEXT, TIPO_MSG, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EIDRM || errno == EINVAL)
                break;                      /* A borro la cola */
            perror("msgrcv");
            return 1;
        }
        printf("[C %d] recibido por COLA (tipo %ld): \"%s\"\n",
               (int) getpid(), m.mtype, m.mtext);
        fflush(stdout);
    }

    printf("[C %d] cola eliminada, salgo\n", (int) getpid());
    return 0;
}
