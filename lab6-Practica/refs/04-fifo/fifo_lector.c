/*
 * FIFO (named pipe) - LECTOR
 *
 * Uso:  ./fifo_lector
 *
 * open(ruta, O_RDONLY): BLOQUEA hasta que alguien abra para escribir.
 * read(fd, buf, n):
 *   > 0  bytes leidos (puede ser MENOS de lo pedido: es un flujo)
 *   = 0  fin de archivo: TODOS los escritores cerraron
 *   < 0  error (o EINTR si llego una senal)
 *
 * El bucle "while read > 0" lee hasta que el escritor cierre.
 * Un read solo NO garantiza el mensaje completo.
 *
 * Equivalente desde la terminal:  cat /tmp/fifoKit
 * Borrar el FIFO al final:        rm /tmp/fifoKit   (o unlink() en C)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO "/tmp/fifoKit"
#define MAXBUF 256

static void die(const char *s) { perror(s); exit(1); }

int main(void)
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;

    if (mkfifo(FIFO, 0666) < 0 && errno != EEXIST)
        die("mkfifo");

    printf("abriendo %s para leer (espera a un escritor)...\n", FIFO);
    fflush(stdout);

    if ((fd = open(FIFO, O_RDONLY)) < 0)
        die("open");

    while ((n = read(fd, buf, MAXBUF - 1)) > 0)
    {
        buf[n] = '\0';                      /* read NO pone el '\0' */
        printf("leidos %zd bytes: \"%s\"\n", n, buf);
    }

    if (n < 0)
        die("read");

    printf("fin de archivo: el escritor cerro\n");
    close(fd);
    return 0;
}
