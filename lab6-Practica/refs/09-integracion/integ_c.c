/*
 * INTEGRACION: el proceso C, lanzado con execv por el clon de B
 *
 * Recibe el descriptor de lectura del pipe en argv[1].
 * Lee hasta EOF (cuando A cierre su extremo de escritura).
 * Imprime su PID y el canal, como pide el control.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <descriptor>\n", argv[0]);
        return 1;
    }

    fd = atoi(argv[1]);
    printf("C %d (padre %d): escuchando el pipe por el fd %d\n",
           (int) getpid(), (int) getppid(), fd);
    fflush(stdout);

    while ((n = read(fd, buf, MAXBUF - 1)) > 0)
    {
        buf[n] = '\0';
        printf("C %d: recibido por pipe (fd %d): \"%s\"\n",
               (int) getpid(), fd, buf);
        fflush(stdout);
    }

    if (n < 0) { perror("read"); return 1; }

    printf("C %d: EOF, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
