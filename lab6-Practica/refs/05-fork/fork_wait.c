/*
 * FORK - CASO 2: recoger el codigo de salida del hijo
 *
 * waitpid(pid, &estado, 0): espera a UN hijo concreto.
 * El entero estado esta codificado; se decodifica con macros:
 *   WIFEXITED(estado)    -> 1 si termino con exit/return
 *   WEXITSTATUS(estado)  -> el valor de exit (0..255)
 *   WIFSIGNALED(estado)  -> 1 si lo mato una senal
 *   WTERMSIG(estado)     -> cual senal
 *
 * Probar: ./fork_wait
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int estado;

    fflush(stdout);
    pid = fork();

    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        printf("HIJO %d: trabajo 1 segundo y salgo con 42\n", (int) getpid());
        fflush(stdout);
        sleep(1);
        exit(42);
    }

    printf("PADRE: esperando al hijo %d\n", (int) pid);
    fflush(stdout);

    if (waitpid(pid, &estado, 0) < 0) { perror("waitpid"); exit(1); }

    if (WIFEXITED(estado))
        printf("PADRE: el hijo salio con codigo %d\n", WEXITSTATUS(estado));
    else if (WIFSIGNALED(estado))
        printf("PADRE: al hijo lo mato la senal %d\n", WTERMSIG(estado));

    return 0;
}
