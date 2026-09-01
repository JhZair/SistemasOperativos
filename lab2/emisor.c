#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLAVE   1234
#define MAXSIZE 128

struct buzon
{
    long mtype;
    char mtext[MAXSIZE];
};

static sigjmp_buf punto1;
static sigjmp_buf punto2;
static sigjmp_buf punto3;

static volatile sig_atomic_t ultima_senal = 0;

static int msqid = -1;

static void die(const char *s)
{
    perror(s);
    exit(1);
}

static void manejador1(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto1, 1);
}

static void manejador2(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto2, 2);
}

static void manejador3(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto3, 3);
}

static void enviar_mensaje(int tipo)
{
    struct buzon sbuf;
    size_t buflen;

    sbuf.mtype = (long) tipo;
    snprintf(sbuf.mtext, MAXSIZE,
             "senal %d atendida por el handler %d",
             (int) ultima_senal, tipo);

    buflen = strlen(sbuf.mtext) + 1;

    if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
        die("msgsnd");

    printf("[emisor] senal %d -> salto no local -> mensaje tipo %d enviado\n",
           (int) ultima_senal, tipo);
    fflush(stdout);
}

int main(void)
{
    int pid = (int) getpid();

    if ((msqid = msgget(CLAVE, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("=========================================================\n");
    printf("[emisor] PID .............. %d\n", pid);
    printf("[emisor] cola IPC (msqid) . %d   (clave %d)\n", msqid, CLAVE);
    printf("---------------------------------------------------------\n");
    printf("  kill -2  %-6d  -> SIGINT   -> mensaje tipo 1\n", pid);
    printf("  kill -10 %-6d  -> SIGUSR1  -> mensaje tipo 2\n", pid);
    printf("  kill -12 %-6d  -> SIGUSR2  -> mensaje tipo 3\n", pid);
    printf("=========================================================\n");
    fflush(stdout);

    if (sigsetjmp(punto1, 1) != 0)
        enviar_mensaje(1);

    if (sigsetjmp(punto2, 1) != 0)
        enviar_mensaje(2);

    if (sigsetjmp(punto3, 1) != 0)
        enviar_mensaje(3);

    signal(SIGINT,  manejador1);
    signal(SIGUSR1, manejador2);
    signal(SIGUSR2, manejador3);

    printf("[emisor] esperando senales...\n");
    fflush(stdout);

    while (1)
        pause();

    return 0;
}
