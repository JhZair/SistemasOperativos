# Catálogo de errores — qué significa y cómo se arregla

Buscar aquí el texto del error antes de perder tiempo.

---

## A. Errores de compilación

### `implicit declaration of function 'fork'` (o `pipe`, `read`, `execv`, `getpid`…)
Falta `#include <unistd.h>`.

### `implicit declaration of function 'wait'`
Falta `#include <sys/wait.h>`.

### `implicit declaration of function 'mkfifo'`
Falta `#include <sys/stat.h>`.

### `implicit declaration of function 'open'` / `'O_RDONLY' undeclared`
Falta `#include <fcntl.h>`.

### `implicit declaration of function 'msgget'` / `'IPC_CREAT' undeclared`
Faltan `#include <sys/ipc.h>` y `#include <sys/msg.h>` (y `<sys/types.h>` antes).

### `unknown type name 'sigjmp_buf'`
Falta `#include <setjmp.h>`.

### `'errno' undeclared` / `'EINTR' undeclared`
Falta `#include <errno.h>`.

### `too few arguments to function 'execv'` / `too many arguments`
`execv` recibe **2** argumentos: ruta y arreglo. Si quieres pasar la lista suelta usa `execl(ruta, arg0, arg1, ..., NULL)`.

### `'null' undeclared`
Es `NULL` en mayúsculas.

### `expected ';' before ...`
Falta un punto y coma en la línea anterior.

### `format '%d' expects argument of type 'int', but argument has type 'long'`
Especificador equivocado. `%ld` para `long` (el `mtype`), `%zd` para `ssize_t` (lo que devuelve `read`), `%zu` para `size_t`.

### `format '%[^\n]' expects argument of type 'char *', but argument has type 'long'`
`scanf` necesita una **dirección**. Para un `long`: `scanf("%ld", &variable)`.

### `return type of 'main' is not 'int'`
Cambiar `void main()` por `int main(void)`.

### `warning: unused parameter 'signo'`
Añadir `(void) signo;` dentro del manejador.

### `warning: unused variable`
Borrar la variable o usarla.

### `warning: ignoring return value of 'write'`
Comprobar: `if (write(...) < 0) perror("write");`

---

## B. Errores en tiempo de ejecución (mensajes de `perror`)

### `Bad file descriptor` (EBADF)
Estás usando un descriptor que no está abierto en este proceso.
- Cerraste el extremo y luego intentaste usarlo.
- Pasaste el descriptor equivocado por `argv` (¿`fd[0]` o `fd[1]`?).
- El descriptor tenía `FD_CLOEXEC` y se cerró en el `exec`.
- Hiciste `pipe()` **después** del `fork()`: el hijo no lo tiene.

### `Interrupted system call` (EINTR)
Llegó una señal mientras esperabas en `read`/`msgrcv`/`pause`. No es un error real: reintentar con `if (errno == EINTR) continue;`.

### `No such file or directory` (ENOENT)
- En `execv`: la ruta del ejecutable está mal. Recuerda el `./` y compilar primero el programa hijo.
- En `open` de FIFO: no se hizo `mkfifo` o la ruta es distinta.
- En `msgget` sin `IPC_CREAT`: la cola no existe aún.

### `File exists` (EEXIST)
`mkfifo` sobre un FIFO que ya existe. **No es error**: ignorar con `if (mkfifo(...) < 0 && errno != EEXIST)`.

### `Resource temporarily unavailable` (EAGAIN)
- `msgsnd` con `IPC_NOWAIT`: la cola está llena. Probablemente nadie la vacía; revisar el receptor o `ipcrm`.
- `read` con `O_NONBLOCK`: no hay datos todavía.

### `No message of desired type` (ENOMSG)
`msgrcv` con `IPC_NOWAIT` y no hay mensajes de ese tipo. Sin la bandera, bloquea en vez de fallar.

### `Invalid argument` (EINVAL)
- `msgsnd`: `mtype` ≤ 0, o la estructura no tiene `long mtype` primero.
- `msgrcv`: el `msqid` no es válido (¿borraron la cola con `ipcrm`?).
- `kill`: número de señal inexistente.

### `Identifier removed` (EIDRM)
La cola se borró mientras estabas bloqueado en `msgrcv`. Alguien hizo `ipcrm`.

### `Operation not permitted` (EPERM)
`kill` a un proceso de otro usuario, o a uno que no existe.

### `Permission denied` (EACCES)
Permisos del FIFO o de la cola. Comprobar el `0666` y la `umask`.

### `No such device or address` (ENXIO)
`open(fifo, O_WRONLY | O_NONBLOCK)` sin ningún lector abierto.

---

## C. Comportamientos extraños (sin mensaje de error)

### El texto se imprime dos veces
`printf` sin `\n` (o con salida redirigida) dejó texto en el búfer, y `fork()` **copió el búfer al hijo**. Poner `fflush(stdout)` antes de cada `fork()`.

### El texto del hijo desaparece tras `execv`
`exec` descarta el búfer de `printf`. `fflush(stdout)` antes del `execv`.

### El lector nunca termina / `read` nunca devuelve 0
Algún proceso tiene abierto un `fd[1]` que no usa. Revisar que **todos** cierren los extremos que no usan, incluidos los intermediarios que no tocan el pipe.

### El programa se cuelga en `open()` del FIFO
Es normal: espera al otro extremo. Abre el otro lado (o `echo > fifo` / `cat fifo` desde la terminal). Si no quieres bloquear, `O_NONBLOCK`.

### `echo "x" > /tmp/fifo` se queda colgado en la terminal
Idem: espera a un lector. Usar `echo "x" > /tmp/fifo &` para recuperar el prompt.

### Ctrl+C no funciona / la señal no hace nada la segunda vez
`sigsetjmp(buf, 0)` o `setjmp` sin guardar máscara: la señal quedó bloqueada tras el primer salto. Usar `sigsetjmp(buf, 1)`.

### El hijo también reacciona a `kill -2` (o muere con la señal del padre)
Los manejadores se heredan con `fork`. Registrarlos **después** del `fork`, o en el hijo hacer `signal(SIGINT, SIG_DFL)`.

### `Segmentation fault` al llegar la señal
`siglongjmp` a un `sigjmp_buf` que nunca pasó por `sigsetjmp`. Armar los puntos **antes** de `signal()`.

### `Segmentation fault` en `scanf`
`scanf("%[^\n]", variable)` con una variable que no es `char *`. Escribió en la dirección 1.

### Basura al imprimir lo leído
`read` no pone `'\0'`. Añadir `buf[n] = '\0'` y leer con `MAXBUF - 1`.

### Los mensajes de otro programa aparecen en mi cola
Misma clave (1234) que otro proceso. Cambiar la clave o limpiar con `ipcrm -Q 1234`.

### `msgsnd: No space left on device` después de varias pruebas
Colas huérfanas llenas de ejecuciones anteriores. `ipcs -q` y `ipcrm -Q clave`.

### Aparece `<defunct>` en `ps`
Un hijo terminó y el padre no hizo `wait()`. Es un **zombi**: inofensivo pero conviene recogerlo con `wait()`.

### El PID que imprime `system("echo $$")` no es el de mi proceso
`$$` es el PID de la **shell** que lanza `system()`. Usar `getpid()`.

### El proceso B recibe los mensajes que eran para C (o viceversa)
Misma cola y tipo compartidos. Usar claves distintas por canal, y tipos distintos como segunda capa.
