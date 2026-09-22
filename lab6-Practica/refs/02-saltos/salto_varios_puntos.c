/*
 * SALTOS NO LOCALES - CASO 2: un punto de recuperacion por senal
 *
 * Cuando el enunciado dice "cada handler con su punto de recuperacion",
 * se usa un sigjmp_buf POR SENAL. Cada manejador salta al suyo.
 * Es el patron de los labs 02 y 03.
 *
 * Flujo tras un salto a punto_b:
 *   sigsetjmp(punto_b) devuelve != 0  -> se ejecuta accion_b()
 *   sigsetjmp(punto_c) se llama directo -> devuelve 0, se salta
 *   se re-registran los manejadores y se vuelve a pause()
 *
 * Probar: ./salto_varios_puntos ; kill -2/-10/-12 <PID>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf punto_a;
static sigjmp_buf punto_b;
static sigjmp_buf punto_c;

static volatile sig_atomic_t ultima = 0;

static void h_a(int s) { ultima = s; siglongjmp(punto_a, 1); }
static void h_b(int s) { ultima = s; siglongjmp(punto_b, 1); }
static void h_c(int s) { ultima = s; siglongjmp(punto_c, 1); }

/* Las acciones corren en el flujo normal de main: aqui SI se puede
 * hacer printf, msgsnd, write, open... con seguridad. */
static void accion_a(void) { printf("accion A por senal %d\n", (int) ultima); }
static void accion_b(void) { printf("accion B por senal %d\n", (int) ultima); }
static void accion_c(void) { printf("accion C por senal %d\n", (int) ultima); }

int main(void)
{
    printf("PID %d\n", (int) getpid());
    fflush(stdout);

    /* Los tres puntos, en secuencia. Primera pasada: todos devuelven 0. */
    if (sigsetjmp(punto_a, 1) != 0) accion_a();
    if (sigsetjmp(punto_b, 1) != 0) accion_b();
    if (sigsetjmp(punto_c, 1) != 0) accion_c();
    fflush(stdout);

    /* Registrar DESPUES de armar los tres puntos */
    signal(SIGINT,  h_a);
    signal(SIGUSR1, h_b);
    signal(SIGUSR2, h_c);

    while (1)
        pause();

    return 0;
}
