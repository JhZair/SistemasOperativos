/*
 * INTEGRACION: fork + fork + pipe + execv + senal + salto no local
 *
 * Es la forma del laboratorio 5 y la mitad del control:
 *
 *   A (este programa)  --fork-->  B (hijo)  --fork-->  clon de B
 *   |                                                     |
 *   | pipe                                                | execv
 *   v                                                     v
 *   escribe al recibir SIGUSR1  ----------------------->  C (integ_c)
 *                                                         lee del pipe
 *
 * ORDEN de las operaciones en A y por que:
 *   1. pipe()            antes de cualquier fork, para que se herede
 *   2. fork() -> B       B hereda el pipe
 *   3.   B: fork() -> clon      el clon hereda el pipe
 *   4.   clon: execv(integ_c)   el pipe sobrevive al exec
 *   5. A: sigsetjmp + signal    DESPUES del fork: B no hereda manejadores
 *   6. A: pause()
 *
 * Cierre de extremos (critico para que C reciba EOF al final):
 *   A cierra fd[0]           (solo escribe)
 *   B cierra fd[0] y fd[1]   (no usa el pipe)
 *   clon cierra fd[1]        (C solo lee)
 *
 * Compilar:
 *   gcc -Wall -Wextra integ_a.c -o integ_a
 *   gcc -Wall -Wextra integ_c.c -o integ_c
 * Probar:
 *   ./integ_a
 *   kill -10 <PID A>   (varias veces: C imprime cada mensaje)
 *   kill -15 <PID A>   (A cierra el pipe, C ve EOF, todo termina)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXBUF 256

static sigjmp_buf punto;
static volatile sig_atomic_t ultima = 0;
static int fd[2];
static int contador = 0;

static void h_enviar(int s) { ultima = s; siglongjmp(punto, 1); }
static void h_fin(int s)    { ultima = s; siglongjmp(punto, 2); }

int main(void)
{
    pid_t b;
    int origen;
    char buf[MAXBUF];

    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    printf("A %d: pipe creado (lectura fd %d, escritura fd %d)\n",
           (int) getpid(), fd[0], fd[1]);
    fflush(stdout);

    b = fork();
    if (b < 0) { perror("fork"); exit(1); }

    if (b == 0)
    {
        /* ================= B ================= */
        pid_t clon;

        printf("B %d: nacido, creo mi clon\n", (int) getpid());
        fflush(stdout);

        clon = fork();
        if (clon < 0) { perror("fork"); exit(1); }

        if (clon == 0)
        {
            /* ============ CLON DE B -> C ============ */
            char texto_fd[16];

            close(fd[1]);
            snprintf(texto_fd, sizeof(texto_fd), "%d", fd[0]);

            char *args[] = { "./integ_c", texto_fd, NULL };
            execv("./integ_c", args);
            perror("execv");
            exit(1);
        }

        /* B no usa el pipe: cierra AMBOS extremos */
        close(fd[0]);
        close(fd[1]);

        wait(NULL);                     /* B espera a C */
        printf("B %d: C termino, salgo\n", (int) getpid());
        exit(0);
    }

    /* ================= A ================= */
    close(fd[0]);

    origen = sigsetjmp(punto, 1);

    if (origen == 1)
    {
        contador++;
        snprintf(buf, MAXBUF, "mensaje %d de A por senal %d",
                 contador, (int) ultima);
        write(fd[1], buf, strlen(buf));
        printf("A: enviado por el pipe \"%s\"\n", buf);
        fflush(stdout);
    }
    else if (origen == 2)
    {
        printf("A: cerrando el pipe y esperando a B\n");
        fflush(stdout);
        close(fd[1]);                   /* C recibe EOF */
        wait(NULL);
        printf("A: fin\n");
        exit(0);
    }

    /* Registrar DESPUES del fork y DESPUES de sigsetjmp */
    signal(SIGUSR1, h_enviar);
    signal(SIGTERM, h_fin);

    if (origen == 0)
    {
        printf("A: kill -10 %d envia, kill -15 %d termina\n",
               (int) getpid(), (int) getpid());
        fflush(stdout);
    }

    while (1)
        pause();

    return 0;
}
