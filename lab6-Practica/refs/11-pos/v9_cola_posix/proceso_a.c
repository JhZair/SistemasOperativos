#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>        /* CAMBIO V9: O_CREAT, O_RDWR */
#include <mqueue.h>       /* CAMBIO V9: cola POSIX */

#define NOMBRE_COLA "/colaControl"   /* CAMBIO V9: nombre, no clave numerica */
#define PRIORIDAD   0
#define MAXTEXT     256

#define SALTO_COLA 1
#define SALTO_PIPE 2
#define SALTO_FIN  3

/* CAMBIO V9: no hay struct con mtype. Se envian bytes y una prioridad. */

static sigjmp_buf punto_salto;
static volatile sig_atomic_t ultima_senal = 0;

static int   fd[2];
static mqd_t cola = (mqd_t) -1;    /* CAMBIO V9 */
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
    char texto[MAXTEXT];

    contador_cola++;
    snprintf(texto, MAXTEXT, "mensaje %d de A por senal %d",
             contador_cola, (int) ultima_senal);

    /* CAMBIO V9: mq_send(cola, bytes, longitud, prioridad) */
    if (mq_send(cola, texto, strlen(texto) + 1, PRIORIDAD) < 0)
    {
        perror("mq_send");
        return;
    }
    printf("[A %d] enviado por COLA POSIX: \"%s\"\n", (int) getpid(), texto);
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
    printf("[A %d] enviado por PIPE: \"%s\"\n", (int) getpid(), buf);
    fflush(stdout);
}

static void terminar(void)
{
    close(fd[1]);

    /* CAMBIO V9: mq_unlink NO despierta a B bloqueado en mq_receive.
     * Se le manda un mensaje especial de fin. */
    mq_send(cola, "FIN", 4, PRIORIDAD);
    waitpid(pid_b, NULL, 0);
    mq_close(cola);
    mq_unlink(NOMBRE_COLA);
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
    char texto[MAXTEXT];    /* CAMBIO V9: >= mq_msgsize o mq_receive falla */
    unsigned int prio;
    ssize_t n;

    signal(SIGINT,  SIG_IGN);
    signal(SIGTRAP, SIG_IGN);

    pid_clon = fork();
    if (pid_clon < 0)
        die("fork clon");
    if (pid_clon == 0)
        clon_de_b();

    close(fd[0]);
    close(fd[1]);

    printf("[B %d] esperando en la cola POSIX %s\n", (int) getpid(), NOMBRE_COLA);
    fflush(stdout);

    while (1)
    {
        /* CAMBIO V9: mq_receive entrega el de MAYOR prioridad, y el mas
         * antiguo entre iguales. No hay filtrado por tipo. */
        n = mq_receive(cola, texto, MAXTEXT, &prio);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("mq_receive");
        }
        if (strcmp(texto, "FIN") == 0)
            break;
        printf("[B %d] recibido por COLA POSIX (prioridad %u): \"%s\"\n",
               (int) getpid(), prio, texto);
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
    /* CAMBIO V9: mq_open en vez de msgget. El atributo mq_msgsize fija el
     * tamano maximo de cada mensaje; mq_maxmsg, cuantos caben. */
    {
        struct mq_attr attr;

        memset(&attr, 0, sizeof(attr));
        attr.mq_maxmsg  = 10;
        attr.mq_msgsize = MAXTEXT;
        mq_unlink(NOMBRE_COLA);     /* por si quedo de otra ejecucion */
        cola = mq_open(NOMBRE_COLA, O_CREAT | O_RDWR, 0666, &attr);
        if (cola == (mqd_t) -1)
            die("mq_open");
    }
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
