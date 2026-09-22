#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    pipe(fd);

    pid_t pid = fork();

    if (pid == 0) {
        // HIJO: ejecutará programa2

        close(fd[1]);

        char descriptor[20];
        sprintf(descriptor, "%d", fd[0]);

        char *args[] = {
            "./programa3.exe",
            descriptor,
            NULL
        };
        printf("Descriptor del papa: %s\n",descriptor);
        execv("./programa3.exe", args);

        perror("execv");
        exit(1);
    }

    // PADRE
    close(fd[0]);

    char mensaje[] = "John zair oros perez\n";
    write(fd[1], mensaje, sizeof(mensaje) - 1);

    close(fd[1]);

    wait(NULL);

    return 0;
}