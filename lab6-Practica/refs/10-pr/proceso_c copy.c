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
        fprintf(stderr, "uso: %s <descriptor de lectura>\n", argv[0]);
        fprintf(stderr, "este programa lo lanza proceso_a, no se ejecuta solo\n");
        return 1;
    }

    fd = atoi(argv[1]);

    printf("[C %d] soy proceso_c (padre B = %d), escucho el PIPE por el fd %d\n",
           (int) getpid(), (int) getppid(), fd);
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

        printf("[C %d] recibido por PIPE (fd %d, %zd bytes): \"%s\"\n",
               (int) getpid(), fd, n, buf);
        fflush(stdout);
    }

    printf("[C %d] EOF: A cerro el pipe, salgo\n", (int) getpid());
    close(fd);
    return 0;
}
