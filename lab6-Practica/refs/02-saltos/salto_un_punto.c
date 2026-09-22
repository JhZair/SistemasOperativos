/*
 * SALTOS NO LOCALES - CASO 1: un solo punto, varias senales
 *
 * Un unico sigjmp_buf. Cada manejador salta con un valor distinto,
 * y ese valor es lo que devuelve sigsetjmp para saber quien salto.
 * Es el patron mas compacto: 1 buffer, N manejadores, N condiciones.
 *
 * sigsetjmp(buf, 1):
 *   - primera vez (llamada directa)    -> devuelve 0
 *   - al volver por siglongjmp(buf, n) -> devuelve n
 *   - el 1 guarda/restaura la MASCARA de senales. Sin el, tras el
 *     primer salto la senal queda bloqueada para siempre.
 *
 * siglongjmp(buf, n): nunca retorna. Descarta la pila acumulada
 * desde sigsetjmp y reaparece dentro de el devolviendo n.
 *
 * Probar:  ./salto_un_punto ; kill -2/-10/-12 <PID> varias veces
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#define SALTO_INT  1
#define SALTO_USR1 2
#define SALTO_USR2 3

static sigjmp_buf punto;                      /* global: el manejador */
static volatile sig_atomic_t ultima = 0;      /* lo necesita ver     */

static void h_int (int s) { ultima = s; siglongjmp(punto, SALTO_INT);  }
static void h_usr1(int s) { ultima = s; siglongjmp(punto, SALTO_USR1); }
static void h_usr2(int s) { ultima = s; siglongjmp(punto, SALTO_USR2); }

int main(void)
{
    int origen;

    printf("PID %d\n", (int) getpid());
    fflush(stdout);

    /* ---- ETIQUETA DE RETORNO ---- */
    origen = sigsetjmp(punto, 1);

    if (origen == 0)
    {
        printf("primera pasada, etiqueta armada\n");
    }
    else if (origen == SALTO_INT)
    {
        printf("salto %d: llego la senal %d (SIGINT)\n", origen, (int) ultima);
    }
    else if (origen == SALTO_USR1)
    {
        printf("salto %d: llego la senal %d (SIGUSR1)\n", origen, (int) ultima);
    }
    else if (origen == SALTO_USR2)
    {
        printf("salto %d: llego la senal %d (SIGUSR2) -> fin\n", origen, (int) ultima);
        exit(0);
    }
    fflush(stdout);

    /* Registrar DESPUES de armar la etiqueta. Si una senal llegara
     * antes, siglongjmp saltaria a un buffer con basura. */
    signal(SIGINT,  h_int);
    signal(SIGUSR1, h_usr1);
    signal(SIGUSR2, h_usr2);

    /* El bucle va FUERA del if: asi se reanuda tras cada salto */
    while (1)
    {
        printf("trabajando...\n");
        fflush(stdout);
        sleep(2);               /* sleep se interrumpe con la senal */
    }

    return 0;
}
