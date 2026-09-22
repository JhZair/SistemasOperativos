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
#include <sys/stat.h>     /* CAMBIO V4: mkfifo */
#include <fcntl.h>        /* CAMBIO V4: open */

#define CLAVE_COLA 2026
#define TIPO_MSG   1
#define MAXTEXT    256
#define FIFO_PATH  "/tmp/fifoControl"   /* CAMBIO V4 */

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

static int   fd_fifo = -1;          /* CAMBIO V4: un solo descriptor */
static int   msqid = -1;
static pid_t pid_b = -1;
static int   contador_cola = 0;
static int   contador_pipe = 0;

static void die(const char *s)
{
    perror(s);
    exit(1);
}

static void manejador_int(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_COLA);
}

static void manejador_trap(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_PIPE);
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
    printf("[A %d] enviado por COLA: \"%s\"\n", (int) getpid(), m.mtext);
    fflush(stdout);
}

static void enviar_por_pipe(void)
{
    char buf[MAXTEXT];

    contador_pipe++;
    snprintf(buf, MAXTEXT, "mensaje %d de A por senal %d",
             contador_pipe, (int) ultima_senal);

    if (write(fd_fifo, buf, strlen(buf)) < 0)
    {
        perror("write");
        return;
    }
    printf("[A %d] enviado por FIFO: \"%s\"\n", (int) getpid(), buf);
    fflush(stdout);
}

static void terminar(void)
{
    close(fd_fifo);
    unlink(FIFO_PATH);      /* CAMBIO V4: el FIFO no desaparece solo */
    msgctl(msqid, IPC_RMID, NULL);
    waitpid(pid_b, NULL, 0);
    printf("[A %d] fin\n", (int) getpid());
    exit(0);
}

static void clon_de_b(void)
{
    /* CAMBIO V4: nada que heredar ni cerrar. Se pasa la RUTA del FIFO:
     * C lo abrira por su nombre, como un archivo. */
    char *args[] = { "./proceso_c", FIFO_PATH, NULL };

    fflush(stdout);
    execv("./proceso_c", args);
    die("execv");
}

static void proceso_b(void)
{
    pid_t pid_clon;
    struct mensaje m;
    ssize_t n;

    signal(SIGINT,  SIG_IGN);
    signal(SIGTRAP, SIG_IGN);

    pid_clon = fork();
    if (pid_clon < 0)
        die("fork clon");
    if (pid_clon == 0)
        clon_de_b();

    /* CAMBIO V4: B no hereda descriptores del FIFO: nada que cerrar */

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

    /* CAMBIO V4: FIFO con nombre en vez de pipe anonimo.
     * EEXIST no es error: el FIFO ya existia de otra ejecucion. */
    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST)
        die("mkfifo");
    if ((msqid = msgget(CLAVE_COLA, IPC_CREAT | 0666)) < 0)
        die("msgget");
    fflush(stdout);

    pid_b = fork();
    if (pid_b < 0)
        die("fork B");
    if (pid_b == 0)
        proceso_b();

    signal(SIGPIPE, SIG_IGN);

    /* CAMBIO V4: abrir para escribir. open BLOQUEA hasta que C abra
     * para leer: es la "cita" del FIFO. Por eso va DESPUES del fork. */
    printf("[A %d] esperando a que C abra el FIFO...\n", (int) getpid());
    fflush(stdout);
    if ((fd_fifo = open(FIFO_PATH, O_WRONLY)) < 0)
        die("open fifo");

    origen = sigsetjmp(punto_salto, 1);

    if (origen == SALTO_COLA)
        enviar_por_cola();
    else if (origen == SALTO_PIPE)
        enviar_por_pipe();
    else if (origen == SALTO_FIN)
        terminar();

    signal(SIGINT,  manejador_int);
    signal(SIGTRAP, manejador_trap);
    signal(SIGTERM, manejador_term);

    if (origen == 0)
    {
        printf("[A %d] listo. kill -2 / -5 / -15 %d\n",
               (int) getpid(), (int) getpid());
        fflush(stdout);
    }

    while (1)
        pause();

    return 0;
}
