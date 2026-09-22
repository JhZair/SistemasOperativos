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

        char desciptor[20];
        sprintf(desciptor, "%d", fd[0])
        char *args[]={"./programa3.exe",desciptor,null}// què se busca hacer con esto?
        execv("./programa3.exe", desciptor, null)

        sprintf()
     
        perror("execv");
        exit(1);
    }

    // PADRE
    close(fd[0]);

    char mensaje[] = "Hola desde el proceso padre\n";
    write(fd[1], mensaje, sizeof(mensaje) - 1);

    close(fd[1]);

    wait(NULL);

    return 0;
}

//versiòn original del archivo en wsp"