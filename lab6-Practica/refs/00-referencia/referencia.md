# Referencia rápida — Sistemas Operativos (señales, IPC, procesos)

Compilar siempre: `gcc -Wall -Wextra archivo.c -o archivo`

---

## 1. Cabeceras: qué incluir para qué

| Cabecera | Funciones / tipos que trae |
|---|---|
| `stdio.h` | `printf` `fprintf` `snprintf` `perror` `fflush` |
| `stdlib.h` | `exit` `atoi` `atol` |
| `string.h` | `strlen` `strcmp` `memset` `strsignal` |
| `unistd.h` | `fork` `pipe` `read` `write` `close` `dup2` `execv` `execl` `getpid` `getppid` `sleep` `pause` `unlink` `STDIN_FILENO` |
| `errno.h` | `errno` `EINTR` `EEXIST` `EAGAIN` `ENOMSG` `EBADF` |
| `signal.h` | `signal` `sigaction` `kill` `raise` `SIGINT`… `sig_atomic_t` `SIG_DFL` `SIG_IGN` |
| `setjmp.h` | `sigjmp_buf` `sigsetjmp` `siglongjmp` |
| `sys/types.h` | `pid_t` `ssize_t` `key_t` |
| `sys/wait.h` | `wait` `waitpid` `WEXITSTATUS` `WIFEXITED` |
| `sys/ipc.h` + `sys/msg.h` | `msgget` `msgsnd` `msgrcv` `msgctl` `IPC_CREAT` `IPC_NOWAIT` `IPC_RMID` |
| `sys/stat.h` | `mkfifo` |
| `fcntl.h` | `open` `O_RDONLY` `O_WRONLY` `O_NONBLOCK` `fcntl` `FD_CLOEXEC` |

---

## 2. Prototipos y valores de retorno

Convención universal: **−1 = error, motivo en `errno`, imprimir con `perror()`**.

### Procesos
```c
pid_t fork(void);
```
| Devuelve | En quién |
|---|---|
| PID del hijo (> 0) | padre |
| 0 | hijo |
| −1 | padre, no hubo hijo |

```c
pid_t wait(int *estado);                  // cualquier hijo
pid_t waitpid(pid_t pid, int *estado, 0); // un hijo concreto
WIFEXITED(estado)  WEXITSTATUS(estado)    // salió con exit? con qué valor?
pid_t getpid(void);   pid_t getppid(void);
```

### exec (nunca retorna si tiene éxito)
```c
int execv (const char *ruta, char *const argv[]);   // vector, termina en NULL
int execl (const char *ruta, const char *arg0, ..., NULL);
int execvp(const char *archivo, char *const argv[]); // busca en PATH
```
`argv[0]` = nombre del programa (convención). El `NULL` final es obligatorio.

### Pipe
```c
int pipe(int fd[2]);        // fd[0] lectura, fd[1] escritura
int dup2(int viejo, int nuevo);   // nuevo pasa a ser copia de viejo
```

### Lectura / escritura (pipes, FIFOs, archivos)
```c
ssize_t read (int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);
int close(int fd);
```
| `read` devuelve | Significado |
|---|---|
| > 0 | bytes leídos (puede ser menos de `n`) |
| 0 | EOF: todos los escritores cerraron |
| −1 | error (`EINTR` si llegó una señal) |

`read` **no** pone el `'\0'`: hacer `buf[n] = '\0'` antes de imprimir.

### FIFO
```c
int mkfifo(const char *ruta, mode_t modo);   // EEXIST no es error
int open(const char *ruta, int flags);        // O_RDONLY / O_WRONLY
int unlink(const char *ruta);                 // borrar
```
`open` **bloquea** hasta que el otro extremo también abra. Con `O_NONBLOCK` no.

### Señales
```c
void (*signal(int senal, void (*manejador)(int)))(int);
int sigaction(int senal, const struct sigaction *nueva, struct sigaction *vieja);
int kill(pid_t pid, int senal);      // kill(pid, 0) = ¿existe?
int pause(void);                     // duerme hasta una señal atrapada
```
Firma obligatoria del manejador: `void f(int signo)`.
Con `SA_SIGINFO`: `void f(int signo, siginfo_t *info, void *ctx)` → `info->si_pid` = quién envió.

### Saltos no locales
```c
int  sigsetjmp (sigjmp_buf env, int savemask);   // 0 directo, n al volver
void siglongjmp(sigjmp_buf env, int val);        // nunca retorna
```
`savemask = 1` **siempre**, si no la señal queda bloqueada tras el primer salto.

### Colas de mensajes
```c
int     msgget(key_t clave, int flags);                            // IPC_CREAT | 0666
int     msgsnd(int id, const void *msg, size_t tam, int flags);    // tam = solo texto
ssize_t msgrcv(int id, void *msg, size_t max, long tipo, int flags);
int     msgctl(int id, IPC_RMID, NULL);                            // borrar
```
| `tipo` en `msgrcv` | Recibe |
|---|---|
| > 0 | solo ese tipo |
| 0 | el primero de la cola |
| < 0 | el de menor tipo ≤ \|tipo\| |

Estructura: `struct { long mtype; char mtext[N]; }` — `mtype` **primero** y `long`, valor > 0.

---

## 3. Números de señal

| Nº | Nombre | Origen típico | Atrapable |
|---|---|---|---|
| 2 | SIGINT | Ctrl+C | sí |
| 3 | SIGQUIT | Ctrl+\ | sí |
| 5 | SIGTRAP | depuración (usada en el control) | sí |
| 9 | SIGKILL | `kill -9` | **no** |
| 10 | SIGUSR1 | libre | sí |
| 12 | SIGUSR2 | libre | sí |
| 15 | SIGTERM | `kill` sin número | sí |
| 17 | SIGCHLD | murió un hijo | sí |
| 19 | SIGSTOP | — | **no** |
| 20 | SIGTSTP | Ctrl+Z | sí |

---

## 4. Reglas de integración (lo que no está en ningún snippet)

### Qué se hereda y qué se pierde

| | `fork()` | `execv()` |
|---|---|---|
| Descriptores abiertos | se heredan | **sobreviven** (salvo FD_CLOEXEC) |
| Manejadores de señal | se heredan | **se pierden** (vuelven a SIG_DFL) |
| Señales ignoradas (SIG_IGN) | se heredan | se mantienen |
| Memoria / variables | copia | se pierde todo |
| PID | nuevo | **el mismo** |
| Búfer de `printf` | **se duplica** | se descarta |

### Orden de las operaciones

1. `pipe()` **antes** de cualquier `fork()` — si no, el hijo no lo ve.
2. `fflush(stdout)` **antes** de `fork()` y de `execv()`.
3. `sigsetjmp()` **antes** de `signal()` — si no, un salto temprano va a basura.
4. `signal()` **después** de `fork()` si solo el padre debe reaccionar.
5. Cerrar extremos del pipe **inmediatamente** después de `fork()` en cada proceso.

### Cierre de extremos del pipe

| Proceso | Cierra |
|---|---|
| Solo escribe | `fd[0]` |
| Solo lee | `fd[1]` |
| No usa el pipe (intermediario) | `fd[0]` **y** `fd[1]` |

Si alguien deja abierto un `fd[1]` que no usa, el lector **nunca recibe EOF**.

### Dentro de un manejador de señal

Seguro: asignar `volatile sig_atomic_t`, `write()`, `_exit()`, `kill()`, `siglongjmp()`.
No seguro: `printf`, `malloc`, `msgsnd`, `msgrcv`, `exit`, `open`.
Patrón: el manejador anota o salta; `main` hace el trabajo.

### `EINTR`

Si una llamada bloqueante (`read`, `msgrcv`, `pause`, `open` de FIFO) recibe una señal, devuelve −1 con `errno == EINTR`. No es un error: reintentar.
`msgrcv`/`msgsnd` **nunca** se reinician solos, ni con `SA_RESTART`.

### Persistencia de recursos

| Recurso | Muere con el proceso? | Ver | Borrar |
|---|---|---|---|
| Pipe anónimo | sí | — | `close` |
| FIFO | **no** (el nodo queda) | `ls -l /tmp` (`p` al inicio) | `rm` / `unlink` |
| Cola System V | **no** | `ipcs -q` | `ipcrm -Q clave` / `msgctl IPC_RMID` |

---

## 5. Comandos de terminal

```bash
ps -ef | grep nombre              # PID de un proceso
ps -o pid,ppid,stat,cmd           # jerarquía y estado (Z = zombi)
pstree -p <PID>                   # árbol de procesos
kill -<num> <PID>                 # enviar señal
kill -l                           # lista de señales
ipcs -q                           # colas de mensajes
ipcrm -Q <clave>                  # borrar cola por clave
ls -l /tmp/fifo                   # 'p' al inicio = FIFO
echo "texto" > /tmp/fifo &        # escribir en FIFO (el & evita bloquear)
cat /tmp/fifo                     # leer FIFO
```

---

## 6. Esqueleto del control (proceso A)

```
crear pipe
crear cola (msgget IPC_CREAT)
fflush
fork -> B
    B: fork -> clon
        clon: close fd[1]; snprintf(fd[0]); execv("./proceso_c", args)
    B: close fd[0], fd[1]; bucle msgrcv(tipo); imprimir PID+contenido
A: close fd[0]
A: sigsetjmp(punto_cola); sigsetjmp(punto_pipe)
A: signal(SIGINT -> h_cola); signal(SIGTRAP -> h_pipe)
A: while(1) pause()
```
