/*
 * PIPE - CASO 2: comunicacion en las dos direcciones
 *
 * Un pipe es de un solo sentido. Para ida y vuelta hacen falta DOS:
 *   p1: padre -> hijo   (padre escribe p1[1], hijo lee p1[0])
 *   p2: hijo  -> padre  (hijo escribe p2[1], padre lee p2[0])
 *
 * Cada proceso cierra los DOS extremos que no usa.
 * El hijo convierte a mayusculas y devuelve.
 *
 * Probar: ./pipe_bidireccional
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXBUF 256

int main(void)
{
    int p1[2], p2[2];
    pid_t pid;
    char buf[MAXBUF];
    ssize_t n, i;

    if (pipe(p1) < 0 || pipe(p2) < 0) { perror("pipe"); exit(1); }

    fflush(stdout);
    pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0)
    {
        /* ---- HIJO: lee de p1, escribe en p2 ---- */
        close(p1[1]);
        close(p2[0]);

        n = read(p1[0], buf, MAXBUF - 1);
        if (n < 0) { perror("read"); exit(1); }
        buf[n] = '\0';
        printf("HIJO  recibio: \"%s\"\n", buf);

        for (i = 0; i < n; i++)
            buf[i] = (char) toupper((unsigned char) buf[i]);

        write(p2[1], buf, (size_t) n);
        close(p1[0]);
        close(p2[1]);
        exit(0);
    }

    /* ---- PADRE: escribe en p1, lee de p2 ---- */
    close(p1[0]);
    close(p2[1]);

    write(p1[1], "mensaje de ida", 14);
    close(p1[1]);

    n = read(p2[0], buf, MAXBUF - 1);
    if (n < 0) { perror("read"); exit(1); }
    buf[n] = '\0';
    printf("PADRE recibio: \"%s\"\n", buf);

    close(p2[0]);
    wait(NULL);
    return 0;
}
