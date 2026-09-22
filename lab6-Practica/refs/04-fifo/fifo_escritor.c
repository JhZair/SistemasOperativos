/*
 * FIFO (named pipe) - ESCRITOR
 *
 * Uso:  ./fifo_escritor "<texto>"
 *
 * mkfifo(ruta, 0666): crea el nodo en el sistema de archivos.
 *   "ls -l" lo muestra con una 'p' al inicio y tamano 0 SIEMPRE:
 *   los datos viven en el kernel, no en el archivo.
 *   Si ya existe falla con EEXIST -> no es error, se reutiliza.
 *
 * open(ruta, O_WRONLY): BLOQUEA hasta que alguien abra para lectura.
 *   Es la "cita" del FIFO: cada extremo espera al otro.
 *   Con O_WRONLY | O_NONBLOCK no bloquea, pero falla con ENXIO
 *   si no hay lector.
 *
 * Equivalente desde la terminal:  echo "texto" > /tmp/fifoKit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO "/tmp/fifoKit"

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char *argv[])
{
    int fd;
    size_t len;

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s \"<texto>\"\n", argv[0]);
        return 1;
    }

    if (mkfifo(FIFO, 0666) < 0 && errno != EEXIST)
        die("mkfifo");

    printf("abriendo %s para escribir (espera a un lector)...\n", FIFO);
    fflush(stdout);

    if ((fd = open(FIFO, O_WRONLY)) < 0)
        die("open");

    len = strlen(argv[1]);
    if (write(fd, argv[1], len) < 0)
        die("write");

    close(fd);
    printf("escritos %zu bytes: \"%s\"\n", len, argv[1]);
    return 0;
}
