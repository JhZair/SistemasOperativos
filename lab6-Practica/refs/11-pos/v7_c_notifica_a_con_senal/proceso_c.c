#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>       /* CAMBIO V7: kill */
#include <sys/types.h>

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;
    pid_t pid_a;            /* CAMBIO V7 */

    if (argc != 3)          /* CAMBIO V7: ahora son 2 argumentos */
    {
        fprintf(stderr, "uso: %s <fd> <pid_A>\n", argv[0]);
        return 1;
    }

    fd = atoi(argv[1]);
    pid_a = (pid_t) atoi(argv[2]);     /* CAMBIO V7 */
    printf("[C %d] escuchando el PIPE por el fd %d\n", (int) getpid(), fd);
    fflush(stdout);

    while (1)
    {
        n = read(fd, buf, MAXBUF - 1);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }
        if (n == 0)
            break;
        buf[n] = '\0';
        printf("[C %d] recibido por PIPE (fd %d): \"%s\"\n",
               (int) getpid(), fd, buf);
        fflush(stdout);

        /* CAMBIO V7: notificar a A que el mensaje llego */
        if (kill(pid_a, SIGUSR1) < 0)
            perror("kill");
    }

    printf("[C %d] EOF, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
