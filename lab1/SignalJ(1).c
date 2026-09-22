#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

#define SALTO_SIGINT   1
#define SALTO_SIGUSR1  2
#define SALTO_SIGUSR2  3

static sigjmp_buf punto_salto;

static volatile sig_atomic_t ultima_senal = 0;


static void manejador_int(int signo)
{
	ultima_senal = signo;
	siglongjmp(punto_salto, SALTO_SIGINT);
}

static void manejador_usr1(int signo)
{
	ultima_senal = signo;
	siglongjmp(punto_salto, SALTO_SIGUSR1);
}


static void manejador_usr2(int signo)
{
	ultima_senal = signo;
	siglongjmp(punto_salto, SALTO_SIGUSR2);
}

int main(void)
{
	int origen;
	int pid = (int) getpid();

	signal(SIGINT,  manejador_int);
	signal(SIGUSR1, manejador_usr1);
	signal(SIGUSR2, manejador_usr2);
	
	origen = sigsetjmp(punto_salto, 1);
	
	if (origen == 0) {
		printf("Programa iniciado. PID = %d\n", pid);
		printf("  Ctrl+C               -> SIGINT  (%d)\n", SIGINT);
		printf("  kill -SIGUSR1 %-7d-> SIGUSR1 (%d)\n", pid, SIGUSR1);
		printf("  kill -SIGUSR2 %-7d-> SIGUSR2 (%d)  [cierre ordenado]\n",
			   pid, SIGUSR2);
		printf("----------------------------------------------------\n");
	}
	else if (origen == SALTO_SIGINT) {
		printf("\n[SALTO %d] Se recibió la señal %d (SIGINT).\n",
			   origen, (int) ultima_senal);
		printf("Interrupción de teclado ignorada: se reanuda el bucle.\n");
	}
	else if (origen == SALTO_SIGUSR1) {
		printf("\n[SALTO %d] Se recibió la señal %d (SIGUSR1).\n",
			   origen, (int) ultima_senal);
		printf("Simulando recarga de configuración hecho.\n");
	}
	else if (origen == SALTO_SIGUSR2) {
		printf("\n[SALTO %d] Se recibió la señal %d (SIGUSR2).\n",
			   origen, (int) ultima_senal);
		printf("Cierre ordenado solicitado. Liberando recursos\n");
		printf("Fin del programa.\n");
		return 0;
	}
	
	while (1) {
		printf("Ejecutando código normal\n");
		sleep(2);
	}
	
	return 0; 
}
