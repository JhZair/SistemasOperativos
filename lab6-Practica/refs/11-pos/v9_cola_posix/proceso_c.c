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

    if (argc != 2)
    {
        fprintf(stderr, "uso: %s <fd>\n", argv[0]);
        return 1;
    }

    fd = atoi(argv[1]);
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
    }

    printf("[C %d] EOF, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
