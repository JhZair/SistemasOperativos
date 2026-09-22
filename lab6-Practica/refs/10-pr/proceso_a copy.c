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

#define CLAVE_COLA  2026        
#define TIPO_MSG    1           
#define MAXTEXT     256

#define SALTO_COLA  1           
#define SALTO_PIPE  2           
#define SALTO_FIN   3           

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
    snprintf(m.mtext, MAXTEXT, "mensaje %d de A (PID %d) por senal %d",
             contador_cola, (int) getpid(), (int) ultima_senal);

    len = strlen(m.mtext) + 1;          

    if (msgsnd(msqid, &m, len, IPC_NOWAIT) < 0)
    {
        perror("msgsnd");
        return;                         
    }

    printf("[A %d] senal %d -> enviado por COLA (tipo %d): \"%s\"\n",
           (int) getpid(), (int) ultima_senal, TIPO_MSG, m.mtext);
    fflush(stdout);
}


static void enviar_por_pipe(void)
{
    char buf[MAXTEXT];

    contador_pipe++;

    snprintf(buf, MAXTEXT, "mensaje %d de A (PID %d) por senal %d",
             contador_pipe, (int) getpid(), (int) ultima_senal);

    if (write(fd[1], buf, strlen(buf)) < 0)
    {
        perror("write");
        return;
    }

    printf("[A %d] senal %d -> enviado por PIPE (fd %d): \"%s\"\n",
           (int) getpid(), (int) ultima_senal, fd[1], buf);
    fflush(stdout);
}


static void terminar(void)
{
    printf("[A %d] senal %d -> cerrando pipe y cola\n",
           (int) getpid(), (int) ultima_senal);
    fflush(stdout);

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

    printf("[clon de B %d] execv(./proceso_c) pasando el fd %s\n",
           (int) getpid(), texto_fd);
    fflush(stdout);    

    execv("./proceso_c", args);

    die("execv ./proceso_c");
}


static void proceso_b(void)
{
    pid_t pid_clon;
    struct mensaje m;
    ssize_t n;

    signal(SIGINT,  SIG_IGN);
    signal(SIGTRAP, SIG_IGN);

    printf("[B %d] creado por A (PID %d)\n", (int) getpid(), (int) getppid());
    fflush(stdout);    

    pid_clon = fork();
    if (pid_clon < 0)
        die("fork clon de B");

    if (pid_clon == 0)
        clon_de_b();               

    close(fd[0]);
    close(fd[1]);

    printf("[B %d] mi clon es %d, espero mensajes en la cola %d (tipo %d)\n",
           (int) getpid(), (int) pid_clon, msqid, TIPO_MSG);
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

    
        printf("[B %d] recibido por COLA (tipo %ld, %zd bytes): \"%s\"\n",
               (int) getpid(), m.mtype, n, m.mtext);
        fflush(stdout);
    }

    waitpid(pid_clon, NULL, 0);    
    printf("[B %d] la cola fue eliminada y C termino, salgo\n", (int) getpid());
    exit(0);
}


int main(void)
{
    int origen;


    if (pipe(fd) < 0)
        die("pipe");

    if ((msqid = msgget(CLAVE_COLA, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("[A %d] recursos IPC listos: pipe (lectura fd %d, escritura fd %d), "
           "cola %d (clave %d)\n",
           (int) getpid(), fd[0], fd[1], msqid, CLAVE_COLA);
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
        printf("[A %d] B es el PID %d. Esperando senales:\n",
               (int) getpid(), (int) pid_b);
        printf("        kill -2  %d  -> mensaje a B por la cola\n", (int) getpid());
        printf("        kill -5  %d  -> mensaje a C por el pipe\n", (int) getpid());
        printf("        kill -15 %d  -> terminar todo\n", (int) getpid());
        fflush(stdout);
    }


    while (1)
        pause();

    return 0;  
}
