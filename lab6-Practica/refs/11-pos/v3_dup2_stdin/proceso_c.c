#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#define MAXBUF 256

int main(int argc, char *argv[])
{
    int fd;
    char buf[MAXBUF];
    ssize_t n;

    (void) argc;
    (void) argv;

    /* CAMBIO V3: C no sabe que existe un pipe: lee de stdin */
    fd = STDIN_FILENO;
    printf("[C %d] escuchando stdin (fd %d), redirigido al PIPE\n",
           (int) getpid(), fd);
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
    }

    printf("[C %d] EOF, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
