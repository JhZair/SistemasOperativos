/*
 * DUP2 - CASO 2: redirigir la salida estandar a un archivo
 *
 * Es el otro uso clasico: "cmd > archivo" de la shell.
 *   fd = open(archivo, O_WRONLY|O_CREAT|O_TRUNC, 0644)
 *   dup2(fd, STDOUT_FILENO)
 * Desde ese momento todo printf va al archivo.
 *
 * Probar: ./dup2_archivo ; cat /tmp/salida_dup2.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int fd;

    printf("esto sale por la terminal\n");
    fflush(stdout);         /* vaciar ANTES de redirigir */

    fd = open("/tmp/salida_dup2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); exit(1); }

    if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); exit(1); }
    close(fd);

    printf("esto va al archivo\n");
    printf("PID %d escribiendo en /tmp/salida_dup2.txt\n", (int) getpid());
    fflush(stdout);

    /* stderr sigue en la terminal */
    fprintf(stderr, "stderr sigue en la terminal\n");
    return 0;
}
