#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{

    if (argc != 2) {
        fprintf(stderr, "Uso: %s descriptor\n", argv[0]);
        return 1;
    }

    int fd = atoi(argv[1]);
    printf("descriptor en el hijo (ARG): %d\n",fd);

    //////////////////////
    int flags = fcntl(fd, F_GETFD);

    if (flags == -1) {
        perror("fcntl");
    }
    else if (flags & FD_CLOEXEC) {
        printf("FD_CLOEXEC está ACTIVADO\n");
    }
    else {
        printf("FD_CLOEXEC está DESACTIVADO\n");
    }
    //////////////////////////////////


    char buffer[100];
    ssize_t n;

    n = read(fd, buffer, sizeof(buffer) - 1);
    printf("n:%d\n",(int)n);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Recibido: %s", buffer);
    }

    close(fd);

    return 0;
}