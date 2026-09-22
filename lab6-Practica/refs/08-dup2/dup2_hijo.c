/*
 * DUP2 - CASO 1 (segunda parte): el programa que lee de stdin
 *
 * No recibe argumentos. No sabe que existe un pipe.
 * Lee del descriptor 0 y ya. Lo lanza dup2_padre.
 *
 * Tambien funciona solo, leyendo del teclado:  ./dup2_hijo
 * O con la shell:  echo "hola" | ./dup2_hijo
 */

#include <stdio.h>
#include <unistd.h>

#define MAXBUF 256

int main(void)
{
    char buf[MAXBUF];
    ssize_t n;

    printf("HIJO %d leyendo de stdin (fd 0)...\n", (int) getpid());
    fflush(stdout);

    while ((n = read(STDIN_FILENO, buf, MAXBUF - 1)) > 0)
    {
        buf[n] = '\0';
        printf("HIJO recibio %zd bytes: %s", n, buf);
    }

    printf("HIJO EOF\n");
    return 0;
}
