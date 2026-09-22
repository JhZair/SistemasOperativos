/*
 * ============================================================================
 *  CONTROL 01 - SISTEMAS OPERATIVOS 2026-II
 *  proceso_a.c  ->  contiene el codigo de A, de B y del clon de B
 * ============================================================================
 *
 *  Por que B y el clon estan en este mismo archivo:
 *    B no es un programa aparte. Nace con fork() desde A, asi que es una
 *    COPIA de este mismo programa que toma otra rama del if. Lo mismo el
 *    clon de B. Solo C es un ejecutable distinto (proceso_c.c), porque el
 *    enunciado exige que el clon se transforme con execv().
 *
 *  Jerarquia de procesos:
 *
 *      A  (proceso_a)          captura senales 2 y 5
 *      |
 *      +-- fork() --> B        recibe por la cola de mensajes
 *                     |
 *                     +-- fork() --> clon de B
 *                                    |
 *                                    +-- execv() --> C (proceso_c)
 *                                                    recibe por el pipe
 *
 *  Mapa de requisitos del enunciado (buscar las etiquetas en el codigo):
 *    [A1] A inicializa los recursos IPC: la tuberia y la cola
 *    [A2] A registra manejadores para las senales 2 (SIGINT) y 5 (SIGTRAP)
 *    [A3] A se mantiene en espera pasiva de senales
 *    [A4] Senal 2: A construye un mensaje y lo envia a B por la cola
 *    [A5] Senal 5: A construye un mensaje y lo envia a C por el pipe
 *    [B1] B es creado por A mediante fork()
 *    [B2] B se clona inmediatamente despues de nacer
 *    [B3] B espera los mensajes de A en la cola
 *    [B4] B imprime su PID y el contenido
 *    [C1] El clon de B reemplaza su imagen con execv() -> proceso_c
 *    [C2] C recibe por argv el descriptor de lectura del pipe
 *    (C3 y C4 estan en proceso_c.c)
 *
 *  Compilar (los dos, proceso_c ANTES de ejecutar):
 *    gcc -Wall -Wextra proceso_c.c -o proceso_c
 *    gcc -Wall -Wextra proceso_a.c -o proceso_a
 *
 *  Probar (ver instrucciones completas en la respuesta):
 *    ./proceso_a
 *    kill -2  <PID de A>   -> B imprime
 *    kill -5  <PID de A>   -> C imprime
 *    kill -15 <PID de A>   -> todo termina limpiamente
 * ============================================================================
 */

#include <stdio.h>      /* printf, perror, fflush, snprintf       */
#include <stdlib.h>     /* exit                                    */
#include <string.h>     /* strlen                                  */
#include <unistd.h>     /* fork, pipe, write, close, execv, pause  */
                        /* getpid, getppid                         */
#include <errno.h>      /* errno, EINTR, EIDRM, EINVAL             */
#include <signal.h>     /* signal, SIGINT, SIGTRAP, SIG_IGN        */
#include <setjmp.h>     /* sigjmp_buf, sigsetjmp, siglongjmp       */
#include <sys/types.h>  /* pid_t, ssize_t, key_t                   */
#include <sys/wait.h>   /* waitpid                                 */
#include <sys/ipc.h>    /* IPC_CREAT, IPC_NOWAIT, IPC_RMID         */
#include <sys/msg.h>    /* msgget, msgsnd, msgrcv, msgctl          */

/* ---------------------------------------------------------------------------
 *  Constantes
 * ------------------------------------------------------------------------- */

#define CLAVE_COLA  2026        /* clave de la cola A -> B               */
#define TIPO_MSG    1           /* tipo de mensaje que A envia y B lee   */
#define MAXTEXT     256

/* Codigos de salto: cada manejador salta con uno distinto, y el valor que
 * devuelve sigsetjmp nos dice que senal llego.
 * OJO: NO usar #define SIGTRAP 5. SIGTRAP ya existe en signal.h; redefinirlo
 * es un error. Estos son codigos NUESTROS, con nombres propios. */
#define SALTO_COLA  1           /* llego SIGINT  (2)                      */
#define SALTO_PIPE  2           /* llego SIGTRAP (5)                      */
#define SALTO_FIN   3           /* llego SIGTERM (15), extra para salir   */

/* Estructura del mensaje. REGLA: mtype primero y de tipo long.
 * El kernel lee ese campo para clasificar el mensaje. */
struct mensaje
{
    long mtype;
    char mtext[MAXTEXT];
};

/* ---------------------------------------------------------------------------
 *  Variables globales
 *
 *  Por que son globales y no locales de main:
 *   - Los manejadores de senal no reciben parametros: lo que necesiten
 *     tiene que ser global (punto_salto, ultima_senal).
 *   - Los contadores se MODIFICAN despues de sigsetjmp. El estandar dice
 *     que las variables locales no-volatile modificadas entre sigsetjmp y
 *     siglongjmp quedan con valor INDETERMINADO tras el salto. Globales
 *     no tienen ese problema.
 *   - fd, msqid y pid_b los usan varias funciones.
 * ------------------------------------------------------------------------- */

static sigjmp_buf punto_salto;
static volatile sig_atomic_t ultima_senal = 0;

static int   fd[2];             /* fd[0] lectura, fd[1] escritura */
static int   msqid = -1;
static pid_t pid_b = -1;
static int   contador_cola = 0;
static int   contador_pipe = 0;

/* ---------------------------------------------------------------------------
 *  Utilidad
 * ------------------------------------------------------------------------- */

static void die(const char *s)
{
    perror(s);
    exit(1);
}

/* ---------------------------------------------------------------------------
 *  [A2] Manejadores de senal de A
 *
 *  Solo anotan la senal y saltan. NO hacen printf, msgsnd ni write:
 *  esas funciones no son seguras dentro de un manejador. El trabajo real
 *  se hace en main, despues del salto.
 * ------------------------------------------------------------------------- */

static void manejador_int(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_COLA);
}

static void manejador_trap(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_PIPE);
}

static void manejador_term(int signo)
{
    ultima_senal = signo;
    siglongjmp(punto_salto, SALTO_FIN);
}

/* ---------------------------------------------------------------------------
 *  [A4] Senal 2 -> mensaje a B por la cola
 * ------------------------------------------------------------------------- */

static void enviar_por_cola(void)
{
    struct mensaje m;
    size_t len;

    contador_cola++;

    m.mtype = TIPO_MSG;
    snprintf(m.mtext, MAXTEXT, "mensaje %d de A (PID %d) por senal %d",
             contador_cola, (int) getpid(), (int) ultima_senal);

    len = strlen(m.mtext) + 1;          /* +1: enviar tambien el '\0' */

    if (msgsnd(msqid, &m, len, IPC_NOWAIT) < 0)
    {
        perror("msgsnd");
        return;                         /* A no muere, sigue esperando */
    }

    printf("[A %d] senal %d -> enviado por COLA (tipo %d): \"%s\"\n",
           (int) getpid(), (int) ultima_senal, TIPO_MSG, m.mtext);
    fflush(stdout);
}

/* ---------------------------------------------------------------------------
 *  [A5] Senal 5 -> mensaje a C por el pipe
 * ------------------------------------------------------------------------- */

static void enviar_por_pipe(void)
{
    char buf[MAXTEXT];

    contador_pipe++;

    snprintf(buf, MAXTEXT, "mensaje %d de A (PID %d) por senal %d",
             contador_pipe, (int) getpid(), (int) ultima_senal);

    /* El pipe es un flujo de bytes sin fronteras: si se enviaran dos
     * mensajes antes de que C lea, C podria recibirlos pegados en un solo
     * read. Con kill manuales no pasa, pero conviene saberlo. */
    if (write(fd[1], buf, strlen(buf)) < 0)
    {
        perror("write");
        return;
    }

    printf("[A %d] senal %d -> enviado por PIPE (fd %d): \"%s\"\n",
           (int) getpid(), (int) ultima_senal, fd[1], buf);
    fflush(stdout);
}

/* ---------------------------------------------------------------------------
 *  EXTRA (no lo pide el enunciado): terminar ordenadamente con kill -15
 *
 *  Sin esto, al matar A quedarian B y C vivos y la cola en el kernel.
 *  El orden importa:
 *    1. cerrar fd[1] -> C recibe EOF en su read y termina
 *    2. borrar cola  -> B despierta de msgrcv con error EIDRM y termina
 *    3. esperar a B  -> evita que B quede zombi
 * ------------------------------------------------------------------------- */

static void terminar(void)
{
    printf("[A %d] senal %d -> cerrando pipe y cola\n",
           (int) getpid(), (int) ultima_senal);
    fflush(stdout);

    close(fd[1]);
    msgctl(msqid, IPC_RMID, NULL);
    waitpid(pid_b, NULL, 0);

    printf("[A %d] fin\n", (int) getpid());
    exit(0);
}

/* ---------------------------------------------------------------------------
 *  [C1] [C2] Clon de B: se transforma en proceso_c
 *
 *  Esta funcion NUNCA retorna: o execv tiene exito (y este codigo deja de
 *  existir, reemplazado por proceso_c) o falla y hacemos exit.
 * ------------------------------------------------------------------------- */

static void clon_de_b(void)
{
    char texto_fd[16];

    /* C solo va a leer: cierra el extremo de escritura.
     * Si no lo cerrara, C nunca recibiria EOF, porque el mismo seria
     * "un escritor que sigue abierto". */
    close(fd[1]);

    /* [C2] El programa nuevo no sabe que descriptor tiene el pipe.
     * Se lo pasamos como TEXTO por argv: numero -> cadena con snprintf.
     * C hara el camino inverso con atoi. */
    snprintf(texto_fd, sizeof(texto_fd), "%d", fd[0]);

    char *args[] = { "./proceso_c", texto_fd, NULL };
    /*                  argv[0]      argv[1]   fin de la lista (obligatorio) */

    printf("[clon de B %d] execv(./proceso_c) pasando el fd %s\n",
           (int) getpid(), texto_fd);
    fflush(stdout);     /* obligatorio: execv descarta el buffer de printf */

    /* [C1] Reemplaza la imagen de memoria. El PID se conserva: C tendra
     * el mismo PID que este clon. El descriptor fd[0] sobrevive al exec
     * porque los pipes no tienen FD_CLOEXEC activado. */
    execv("./proceso_c", args);

    /* Solo se llega aqui si execv FALLO (casi siempre: no se compilo
     * proceso_c o no esta en el directorio actual) */
    die("execv ./proceso_c");
}

/* ---------------------------------------------------------------------------
 *  [B1] a [B4] Proceso B
 * ------------------------------------------------------------------------- */

static void proceso_b(void)
{
    pid_t pid_clon;
    struct mensaje m;
    ssize_t n;

    /* EXTRA: B ignora las senales 2 y 5. Asi, si alguien pulsa Ctrl+C en
     * la terminal (que envia SIGINT a TODO el grupo de procesos), B no
     * muere. Y como SIG_IGN SOBREVIVE a execv, el clon lo hereda y C
     * tambien queda protegido. (Los manejadores propios, en cambio, se
     * pierden con execv.) */
    signal(SIGINT,  SIG_IGN);
    signal(SIGTRAP, SIG_IGN);

    printf("[B %d] creado por A (PID %d)\n", (int) getpid(), (int) getppid());
    fflush(stdout);     /* antes del fork: evita duplicar el buffer */

    /* [B2] Inmediatamente despues de nacer, B se clona */
    pid_clon = fork();
    if (pid_clon < 0)
        die("fork clon de B");

    if (pid_clon == 0)
        clon_de_b();                /* no retorna */

    /* B no usa el pipe: cierra AMBOS extremos.
     * Tiene que hacerlo DESPUES de crear al clon: si lo hiciera antes, el
     * clon naceria sin descriptores del pipe y C no podria leer. */
    close(fd[0]);
    close(fd[1]);

    printf("[B %d] mi clon es %d, espero mensajes en la cola %d (tipo %d)\n",
           (int) getpid(), (int) pid_clon, msqid, TIPO_MSG);
    fflush(stdout);

    /* [B3] Espera los mensajes de A. msgrcv sin IPC_NOWAIT es BLOQUEANTE:
     * B duerme en el kernel hasta que llegue algo. */
    while (1)
    {
        n = msgrcv(msqid, &m, MAXTEXT, TIPO_MSG, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;           /* interrumpido por una senal: reintentar */
            if (errno == EIDRM || errno == EINVAL)
                break;              /* A borro la cola: es hora de salir */
            die("msgrcv");
        }

        /* [B4] Imprime su PID y el contenido */
        printf("[B %d] recibido por COLA (tipo %ld, %zd bytes): \"%s\"\n",
               (int) getpid(), m.mtype, n, m.mtext);
        fflush(stdout);
    }

    waitpid(pid_clon, NULL, 0);     /* recoger a C para que no quede zombi */
    printf("[B %d] la cola fue eliminada y C termino, salgo\n", (int) getpid());
    exit(0);
}

/* ===========================================================================
 *  main: PROCESO A
 * =========================================================================== */

int main(void)
{
    int origen;

    /* -----------------------------------------------------------------------
     *  [A1] Inicializar los recursos IPC
     *
     *  AMBOS van ANTES de cualquier fork(). fork copia los descriptores
     *  abiertos y las variables: asi B, el clon y C heredan el pipe, y B
     *  hereda el msqid de la cola.
     * --------------------------------------------------------------------- */

    if (pipe(fd) < 0)
        die("pipe");

    if ((msqid = msgget(CLAVE_COLA, IPC_CREAT | 0666)) < 0)
        die("msgget");

    printf("[A %d] recursos IPC listos: pipe (lectura fd %d, escritura fd %d), "
           "cola %d (clave %d)\n",
           (int) getpid(), fd[0], fd[1], msqid, CLAVE_COLA);
    fflush(stdout);     /* SIEMPRE antes de fork */

    /* -----------------------------------------------------------------------
     *  [B1] Crear B con fork()
     * --------------------------------------------------------------------- */

    pid_b = fork();
    if (pid_b < 0)
        die("fork B");

    if (pid_b == 0)
        proceso_b();                /* B no retorna de aqui */

    /* ======================= Desde aqui solo corre A ===================== */

    /* A solo escribe en el pipe: cierra el extremo de lectura */
    close(fd[0]);

    /* EXTRA: si C muriera, escribir en el pipe provocaria SIGPIPE y
     * mataria a A. Ignorandola, write solo devuelve error EPIPE. */
    signal(SIGPIPE, SIG_IGN);

    /* -----------------------------------------------------------------------
     *  Etiqueta de retorno
     *
     *  Primera pasada: sigsetjmp devuelve 0 -> no se ejecuta ninguna rama.
     *  Tras una senal: el manejador salta aqui y sigsetjmp devuelve el
     *  codigo del salto -> se ejecuta la rama correspondiente.
     *  El 1 del segundo argumento guarda y restaura la mascara de senales:
     *  sin el, tras el primer salto la senal quedaria bloqueada.
     * --------------------------------------------------------------------- */

    origen = sigsetjmp(punto_salto, 1);

    if (origen == SALTO_COLA)
        enviar_por_cola();          /* [A4] */
    else if (origen == SALTO_PIPE)
        enviar_por_pipe();          /* [A5] */
    else if (origen == SALTO_FIN)
        terminar();                 /* extra, no retorna */

    /* -----------------------------------------------------------------------
     *  [A2] Registrar los manejadores
     *
     *  DESPUES del fork: si se registraran antes, B los heredaria.
     *  DESPUES de sigsetjmp: si una senal llegara antes de armar la
     *  etiqueta, siglongjmp saltaria a un buffer con basura.
     *  Registrar de nuevo en cada vuelta es inofensivo.
     * --------------------------------------------------------------------- */

    signal(SIGINT,  manejador_int);     /* senal 2 */
    signal(SIGTRAP, manejador_trap);    /* senal 5 */
    signal(SIGTERM, manejador_term);    /* senal 15, extra */

    if (origen == 0)
    {
        printf("[A %d] B es el PID %d. Esperando senales:\n",
               (int) getpid(), (int) pid_b);
        printf("        kill -2  %d  -> mensaje a B por la cola\n", (int) getpid());
        printf("        kill -5  %d  -> mensaje a C por el pipe\n", (int) getpid());
        printf("        kill -15 %d  -> terminar todo\n", (int) getpid());
        fflush(stdout);
    }

    /* -----------------------------------------------------------------------
     *  [A3] Espera pasiva de senales
     *
     *  pause() suspende el proceso en el kernel SIN consumir CPU hasta que
     *  llegue una senal atrapada. Es "pasiva" frente a la espera activa
     *  while(1); que gastaria el 100% de un nucleo.
     *  pause() nunca retorna aqui: el manejador salta antes a la etiqueta.
     * --------------------------------------------------------------------- */

    while (1)
        pause();

    return 0;   /* inalcanzable */
}
