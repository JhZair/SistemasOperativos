#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLAVE1     1234
#define CLAVE2     5678
#define TIPO_P1_P2 3
#define TIPO_P2_P3 4
#define MAXSIZE    256

struct buzon
{
    long mtype;
    char mtext[MAXSIZE];
};

static void die(const char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    int msqid1;
    int msqid2;
    struct buzon rbuf;
    struct buzon sbuf;
    ssize_t n;
    size_t buflen;

    if ((msqid1 = msgget(CLAVE1, IPC_CREAT | 0666)) < 0)
        die("msgget cola 1");

    if ((msqid2 = msgget(CLAVE2, IPC_CREAT | 0666)) < 0)
        die("msgget cola 2");

    printf("P2 listo. Esperando tipo 3...\n");
    fflush(stdout);

    while (1)
    {
        n = msgrcv(msqid1, &rbuf, MAXSIZE, TIPO_P1_P2, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("msgrcv");
        }

        printf("P2: recibido tipo 3: \"%s\"\n", rbuf.mtext);
        fflush(stdout);

        sbuf.mtype = TIPO_P2_P3;
        snprintf(sbuf.mtext, MAXSIZE, "%s", rbuf.mtext);
        buflen = strlen(sbuf.mtext) + 1;

        if (msgsnd(msqid2, &sbuf, buflen, IPC_NOWAIT) < 0)
        {
            perror("msgsnd");
            continue;
        }

        printf("P2: enviado tipo 4: \"%s\"\n", sbuf.mtext);
        fflush(stdout);
    }

    return 0;
}
