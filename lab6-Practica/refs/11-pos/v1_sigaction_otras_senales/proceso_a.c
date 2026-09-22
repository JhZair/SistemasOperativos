#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define CLAVE_COLA 2026
#define TIPO_MSG   1
#define MAXTEXT    256

#define SALTO_COLA 1
#define SALTO_PIPE 2
#define SALTO_FIN  3

struct mensaje
{
    long mtype;
    char mtext[MAXTEXT];
};

static sigjmp_buf punto_salto;
static volatile sig_atomic_t ultima_senal = 0;

static int   fd[2];
static int   msqid = -1;
static pid_t pid_b = -1;
static int   contador_cola = 0;
static int   contador_pipe = 0;

static void die(const char *s)
{
    perror(s);
    exit(1);
}

/* CAMBIO V1: manejadores de 3 argumentos (SA_SIGINFO) para saber
 * que proceso envio la senal (info->si_pid) */
static volatile sig_atomic_t pid_emisor = 0;

static void manejador_usr1(int signo, siginfo_t *info, void *ctx)
{
    (void) ctx;
    ultima_senal = signo;
    pid_emisor = info->si_pid;
    siglongjmp(punto_salto, SALTO_COLA);
}

static void manejador_usr2(int signo, siginfo_t *info, void *ctx)
{
    (void) ctx;
    ultima_senal = signo;
    pid_emisor = info->si_pid;
    siglongjmp(punto_salto, SALTO_PIPE);
}

/* CAMBIO V1: registrar con sigaction en vez de signal */
static void registrar(int senal, void (*h)(int, siginfo_t *, void *))
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = h;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(senal, &sa, NULL) < 0)
        die("sigaction");
}

static void manejador_term(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_FIN);
}

static void enviar_por_cola(void)
{
    struct mensaje m;
    size_t len;

    contador_cola++;
    m.mtype = TIPO_MSG;
    snprintf(m.mtext, MAXTEXT, "mensaje %d de A por senal %d",
             contador_cola, (int) ultima_senal);
    len = strlen(m.mtext) + 1;

    if (msgsnd(msqid, &m, len, IPC_NOWAIT) < 0)
    {
        perror("msgsnd");
        return;
    }
    printf("[A %d] senal %d del PID %d -> enviado por COLA: \"%s\"\n",
           (int) getpid(), (int) ultima_senal, (int) pid_emisor, m.mtext);
    fflush(stdout);
}

static void enviar_por_pipe(void)
{
    char buf[MAXTEXT];

    contador_pipe++;
    snprintf(buf, MAXTEXT, "mensaje %d de A por senal %d",
             contador_pipe, (int) ultima_senal);

    if (write(fd[1], buf, strlen(buf)) < 0)
    {
        perror("write");
        return;
    }
    printf("[A %d] senal %d del PID %d -> enviado por PIPE: \"%s\"\n",
           (int) getpid(), (int) ultima_senal, (int) pid_emisor, buf);
    fflush(stdout);
}

static void terminar(void)
{
    close(fd[1]);
    msgctl(msqid, IPC_RMID, NULL);
    waitpid(pid_b, NULL, 0);
    printf("[A %d] fin\n", (int) getpid());
    exit(0);
}

static void clon_de_b(void)
{
    char texto_fd[16];

    close(fd[1]);
    snprintf(texto_fd, sizeof(texto_fd), "%d", fd[0]);

    char *args[] = { "./proceso_c", texto_fd, NULL };

    fflush(stdout);
    execv("./proceso_c", args);
    die("execv");
}

static void proceso_b(void)
{
    pid_t pid_clon;
    struct mensaje m;
    ssize_t n;

    signal(SIGUSR1, SIG_IGN);   /* CAMBIO V1: ignorar las senales nuevas */
    signal(SIGUSR2, SIG_IGN);

    pid_clon = fork();
    if (pid_clon < 0)
        die("fork clon");
    if (pid_clon == 0)
        clon_de_b();

    close(fd[0]);
    close(fd[1]);

    printf("[B %d] esperando en la cola\n", (int) getpid());
    fflush(stdout);

    while (1)
    {
        n = msgrcv(msqid, &m, MAXTEXT, TIPO_MSG, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EIDRM || errno == EINVAL)
                break;
            die("msgrcv");
        }
        printf("[B %d] recibido por COLA: \"%s\"\n", (int) getpid(), m.mtext);
        fflush(stdout);
    }

    waitpid(pid_clon, NULL, 0);
    exit(0);
}

int main(void)
{
    int origen;

    if (pipe(fd) < 0)
        die("pipe");
    if ((msqid = msgget(CLAVE_COLA, IPC_CREAT | 0666)) < 0)
        die("msgget");
    fflush(stdout);

    pid_b = fork();
    if (pid_b < 0)
        die("fork B");
    if (pid_b == 0)
        proceso_b();

    close(fd[0]);
    signal(SIGPIPE, SIG_IGN);

    origen = sigsetjmp(punto_salto, 1);

    if (origen == SALTO_COLA)
        enviar_por_cola();
    else if (origen == SALTO_PIPE)
        enviar_por_pipe();
    else if (origen == SALTO_FIN)
        terminar();

    registrar(SIGUSR1, manejador_usr1);     /* CAMBIO V1: senal 10 */
    registrar(SIGUSR2, manejador_usr2);     /* CAMBIO V1: senal 12 */
    signal(SIGTERM, manejador_term);

    if (origen == 0)
    {
        printf("[A %d] listo. kill -10 / -12 / -15 %d\n",
               (int) getpid(), (int) getpid());
        fflush(stdout);
    }

    while (1)
        pause();

    return 0;
}
