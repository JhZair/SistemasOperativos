#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIFO_PATH  "/tmp/fifoLab"
#define CLAVE1     1234
#define TIPO_P1_P2 3
#define MAXSIZE    256

struct buzon
{
    long mtype;
    char mtext[MAXSIZE];
};

static sigjmp_buf punto_fifo;
static sigjmp_buf punto_msg;

static volatile sig_atomic_t ultima_senal = 0;

static int  msqid1 = -1;
static char ultimo_dato[MAXSIZE] = "sin datos del fifo";

static void die(const char *s)
{
    perror(s);
    exit(1);
}

static void manejador_fifo(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_fifo, 1);
}

static void manejador_msg(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_msg, 2);
}

static void leer_fifo(void)
{
    int fd;
    ssize_t n;
    char buf[MAXSIZE];

    if ((fd = open(FIFO_PATH, O_RDONLY)) < 0)
    {
        perror("open fifo");
        return;
    }

    n = read(fd, buf, MAXSIZE - 1);
    close(fd);

    if (n <= 0)
    {
        printf("P1: senal 2 -> FIFO vacio\n");
        fflush(stdout);
        return;
    }

    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = '\0';

    snprintf(ultimo_dato, MAXSIZE, "%s", buf);

    printf("P1: senal 2 -> leido del FIFO: \"%s\"\n", ultimo_dato);
    fflush(stdout);
}

static void enviar_mensaje(void)
{
    struct buzon sbuf;
    size_t buflen;

    sbuf.mtype = TIPO_P1_P2;
    snprintf(sbuf.mtext, MAXSIZE, "%s", ultimo_dato);
    buflen = strlen(sbuf.mtext) + 1;

    if (msgsnd(msqid1, &sbuf, buflen, IPC_NOWAIT) < 0)
    {
        perror("msgsnd");
        return;
    }

    printf("P1: senal 10 -> enviado tipo 3: \"%s\"\n", sbuf.mtext);
    fflush(stdout);
}

int main(void)
{
    int pid = (int) getpid();

    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST)
        die("mkfifo");

    if ((msqid1 = msgget(CLAVE1, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("P1 listo. PID %d\n", pid);
    printf("  kill -2  %d  -> leer FIFO\n", pid);
    printf("  kill -10 %d  -> enviar a P2\n", pid);
    fflush(stdout);

    if (sigsetjmp(punto_fifo, 1) != 0)
        leer_fifo();

    if (sigsetjmp(punto_msg, 1) != 0)
        enviar_mensaje();

    signal(SIGINT,  manejador_fifo);
    signal(SIGUSR1, manejador_msg);

    while (1)
        pause();

    return 0;
}
