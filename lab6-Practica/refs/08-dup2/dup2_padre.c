/*
 * DUP2 - CASO 1: redirigir el pipe a la entrada estandar del hijo
 *
 * dup2(viejo, nuevo): hace que el descriptor 'nuevo' apunte al mismo
 * canal que 'viejo'. Si 'nuevo' estaba abierto, lo cierra primero.
 *
 *   dup2(fd[0], STDIN_FILENO)   -> ahora el 0 (stdin) ES el pipe
 *
 * Ventaja frente a pasar el descriptor por argv: el programa lanzado
 * NO necesita saber nada. Lee de stdin como siempre (read(0,...),
 * scanf, getchar) y recibe lo que el padre escribio en el pipe.
 * Es exactamente lo que hace la shell con "cmd1 | cmd2".
 *
 * Descriptores estandar:  0 STDIN_FILENO, 1 STDOUT_FILENO, 2 STDERR_FILENO
 *
 * Compilar ambos y probar: ./dup2_padre
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    pid_t pid;
    const char *msg = "llegue por stdin gracias a dup2\n";

    if (pipe(fd) < 0) { perror("pipe"); exit(1); }

    fflush(stdout);
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        close(fd[1]);

        if (dup2(fd[0], STDIN_FILENO) < 0) { perror("dup2"); exit(1); }
        close(fd[0]);       /* ya no hace falta: el 0 apunta al pipe */

        char *args[] = { "./dup2_hijo", NULL };
        execv("./dup2_hijo", args);
        perror("execv");
        exit(1);
    }

    close(fd[0]);
    write(fd[1], msg, strlen(msg));
    close(fd[1]);           /* EOF para el hijo */
    wait(NULL);
    return 0;
}
