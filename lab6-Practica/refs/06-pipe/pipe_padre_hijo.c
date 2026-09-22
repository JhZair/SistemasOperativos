/*
 * PIPE - CASO 1: el padre escribe, el hijo lee
 *
 * pipe(fd): crea un canal UNIDIRECCIONAL en el kernel y devuelve
 * dos descriptores:
 *   fd[0] = extremo de LECTURA   (0 como en stdin)
 *   fd[1] = extremo de ESCRITURA (1 como en stdout)
 *
 * ORDEN OBLIGATORIO: pipe() ANTES de fork(). Asi el hijo hereda los
 * dos descriptores y ambos procesos ven el mismo canal.
 *
 * REGLA DE ORO: cada proceso CIERRA el extremo que no usa.
 *   - el lector cierra fd[1]. Si no, read() nunca devuelve 0 (EOF)
 *     porque "todavia hay un escritor abierto": el mismo.
 *   - el escritor cierra fd[0].
 *
 * Probar: ./pipe_padre_hijo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXBUF 256

int main(void)
{
    int fd[2];
    pid_t pid;
    char buf[MAXBUF];
    ssize_t n;
    const char *msg = "hola desde el padre";

    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    fflush(stdout);
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        /* ---- HIJO: lee ---- */
        close(fd[1]);                       /* no escribe */

        n = read(fd[0], buf, MAXBUF - 1);   /* bloquea hasta que haya datos */
        if (n < 0) { perror("read"); exit(1); }
        buf[n] = '\0';

        printf("HIJO %d leyo %zd bytes del fd %d: \"%s\"\n",
               (int) getpid(), n, fd[0], buf);
        close(fd[0]);
        exit(0);
    }

    /* ---- PADRE: escribe ---- */
    close(fd[0]);                           /* no lee */

    if (write(fd[1], msg, strlen(msg)) < 0) { perror("write"); exit(1); }
    printf("PADRE %d escribio en el fd %d\n", (int) getpid(), fd[1]);

    close(fd[1]);           /* cerrar = el lector recibira EOF tras los datos */
    wait(NULL);
    return 0;
}
