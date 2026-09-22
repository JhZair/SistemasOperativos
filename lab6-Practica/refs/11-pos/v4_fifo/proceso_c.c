#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>        /* CAMBIO V4: open */
#include <sys/types.h>

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <ruta_fifo>\n", argv[0]);
        return 1;
    }

    /* CAMBIO V4: argv[1] es la RUTA del FIFO, no un descriptor */
    if ((fd = open(argv[1], O_RDONLY)) < 0)
    {
        perror("open fifo");
        return 1;
    }
    printf("[C %d] escuchando el FIFO %s (fd %d)\n",
           (int) getpid(), argv[1], fd);
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
        printf("[C %d] recibido por FIFO (fd %d): \"%s\"\n",
               (int) getpid(), fd, buf);
        fflush(stdout);
    }

    printf("[C %d] EOF, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
