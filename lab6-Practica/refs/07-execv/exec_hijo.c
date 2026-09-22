/*
 * EXECV - CASO 2 (segunda parte): el programa lanzado
 *
 * Recibe en argv[1] el numero del descriptor de lectura del pipe.
 * argv[0] = "./exec_hijo" (lo puso el padre), argv[1] = "3" o similar.
 *
 * Comprueba FD_CLOEXEC solo por didactica: si estuviera activado,
 * el descriptor se habria cerrado al hacer exec y read fallaria
 * con EBADF (Bad file descriptor).
 *
 * No se ejecuta solo: lo lanza exec_padre.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd, flags;
    char buf[MAXBUF];
    ssize_t n;

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <descriptor>\n", argv[0]);
        return 1;
    }

    fd = atoi(argv[1]);

    printf("HIJO  %d (soy %s): recibi el descriptor %d por argv\n",
           (int) getpid(), argv[0], fd);

    flags = fcntl(fd, F_GETFD);
    if (flags < 0)
        perror("fcntl: el descriptor no es valido");
    else
        printf("HIJO  FD_CLOEXEC esta %s\n",
               (flags & FD_CLOEXEC) ? "ACTIVADO" : "desactivado");

    while ((n = read(fd, buf, MAXBUF - 1)) > 0)
    {
        buf[n] = '\0';
        printf("HIJO  leyo %zd bytes del fd %d: \"%s\"\n", n, fd, buf);
    }
    if (n < 0) { perror("read"); return 1; }

    printf("HIJO  EOF\n");
    close(fd);
    return 0;
}
