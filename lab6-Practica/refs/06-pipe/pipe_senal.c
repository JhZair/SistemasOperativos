/*
 * PIPE - CASO 3: el padre escribe en el pipe CUANDO llega una senal
 *
 * Combina fork + pipe + senal + salto no local. Es el nucleo del
 * "Proceso A" del control: espera pasivamente y, al recibir una
 * senal, envia un mensaje por el pipe a su hijo.
 *
 * Nota el orden en el padre:
 *   1. pipe()  2. fork()  3. sigsetjmp  4. signal()  5. pause()
 * Los manejadores se registran DESPUES del fork para que el hijo
 * no los herede.
 *
 * Probar:
 *   ./pipe_senal
 *   kill -10 <PID padre>   (varias veces)
 *   kill -15 <PID padre>   (termina)
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
static int fd[2];
static int contador = 0;

static void h_enviar(int s) { (void) s; siglongjmp(punto, 1); }
static void h_fin(int s)    { (void) s; close(fd[1]); wait(NULL); exit(0); }

int main(void)
{
    pid_t pid;
    char buf[MAXBUF];
    ssize_t n;

    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    fflush(stdout);
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        /* ---- HIJO: lee hasta EOF ---- */
        close(fd[1]);
        while ((n = read(fd[0], buf, MAXBUF - 1)) > 0)
        {
            buf[n] = '\0';
            printf("HIJO %d recibio: \"%s\"\n", (int) getpid(), buf);
            fflush(stdout);
        }
        printf("HIJO: EOF, saliendo\n");
        exit(0);
    }

    /* ---- PADRE ---- */
    close(fd[0]);

    printf("PADRE %d: kill -10 %d para enviar, kill -15 %d para terminar\n",
           (int) getpid(), (int) getpid(), (int) getpid());
    fflush(stdout);

    if (sigsetjmp(punto, 1) != 0)
    {
        /* Aqui llegamos tras cada SIGUSR1: flujo normal, es seguro escribir */
        contador++;
        snprintf(buf, MAXBUF, "mensaje numero %d", contador);
        write(fd[1], buf, strlen(buf));
        printf("PADRE: enviado \"%s\"\n", buf);
        fflush(stdout);
    }

    signal(SIGUSR1, h_enviar);
    signal(SIGTERM, h_fin);

    while (1)
        pause();

    return 0;
}
