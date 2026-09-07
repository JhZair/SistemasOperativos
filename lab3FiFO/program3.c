#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CLAVE2     5678
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
    int msqid2;
    struct buzon rbuf;
    ssize_t n;

    if ((msqid2 = msgget(CLAVE2, IPC_CREAT | 0666)) < 0)
        die("msgget cola 2");

    printf("P3 listo. Esperando tipo 4...\n");
    fflush(stdout);

    while (1)
    {
        n = msgrcv(msqid2, &rbuf, MAXSIZE, TIPO_P2_P3, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("msgrcv");
        }

        printf("P3: recibido tipo 4: \"%s\"\n", rbuf.mtext);
        fflush(stdout);
    }

    return 0;
}
