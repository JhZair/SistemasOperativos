#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

static sigjmp_buf punto_salto;

void manejador(int signo)
{
    printf("\nSe recibió la señal %d\n", signo);

    // Salto de ejecución al punto establecido con sigsetjmp()
    siglongjmp(punto_salto, 1);
}

int main(void)
{
    // Registrar el manejador para SIGINT (Ctrl+C)
    signal(SIGINT, manejador);

    if (sigsetjmp(punto_salto, 1) == 0) {
        printf("Programa iniciado.\n");
        printf("Pulsa Ctrl+C para enviar SIGINT.\n");

        while (1) {
            printf("Ejecutando código normal...\n");
            sleep(2);
        }
    } else {
        // La ejecución llega aquí después de recibir SIGINT
        printf("¡Se realizó el salto de código!\n");
        printf("Continuando desde el punto de recuperación...\n");
    }

    printf("Fin del programa.\n");

    return 0;
}
