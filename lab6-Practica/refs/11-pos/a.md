# Variantes del Control 01

Cada variante está construida **sobre la misma base** (`base/`), así que el cambio exacto está en `cambios.diff` de cada carpeta. Todas compiladas con `-Wall -Wextra` sin avisos y probadas de principio a fin, incluida la terminación limpia.

```bash
cd v5_confirmacion_por_cola
gcc -Wall -Wextra proceso_c.c -o proceso_c
gcc -Wall -Wextra proceso_a.c -o proceso_a      # V9: añadir -lrt al final
./proceso_a
```

Para ver solo lo que cambió: `cat cambios.diff` (líneas con `+` añadidas, con `-` quitadas).

---

## Matriz: qué zona del código toca cada variante

| Variante | constantes | manejadores | enviar | terminar | clon / exec | proceso_b | main (orden) | proceso_c |
|---|---|---|---|---|---|---|---|---|
| V1 sigaction, señales 10 y 12 | | **X** | x | | | x | x | |
| V2 canales intercambiados | | x | | | **X** | **X** | | **X** |
| V3 dup2 a stdin | | | | | **X** | | | **X** |
| V4 FIFO en vez de pipe | x | | x | x | **X** | x | **X** | **X** |
| V5 confirmación por la cola | x | | **X** | | | **X** | | |
| V6 B y C hermanos | | | | x | x | **X** | **X** | |
| V7 C avisa a A con señal | x | x | **X** | | **X** | | x | **X** |
| V8 B reenvía a C | | | | | | **X** | | |
| V9 cola POSIX | **X** | | **X** | **X** | | **X** | **X** | |

**X** = cambio central, x = ajuste menor.

---

## V1 — Otras señales y `sigaction` obligatorio

**Enunciado posible:** "A debe capturar SIGUSR1 (10) y SIGUSR2 (12) usando `sigaction()`, e indicar el PID del proceso que envió cada señal."

**Qué cambia:** los manejadores pasan a la firma de 3 argumentos para leer `info->si_pid`, y el registro se hace con `sigaction` y la bandera `SA_SIGINFO`. B ignora las señales nuevas en vez de 2 y 5.

**Trampa:** con `SA_SIGINFO` hay que asignar `sa.sa_sigaction`, no `sa.sa_handler`. Y `memset(&sa, 0, sizeof(sa))` antes de usarla: la estructura tiene campos que, con basura, cambian el comportamiento.

**Probar:** `kill -10 <A>` y `kill -12 <A>`. El PID del emisor que imprime A es el de tu terminal.

---

## V2 — Canales intercambiados

**Enunciado posible:** "Señal 2 → A envía a B por un pipe. Señal 5 → A envía a C por la cola de mensajes."

**Qué cambia:** A no cambia su forma de enviar; solo se intercambian los códigos de salto. Lo que cambia son los **receptores**: B lee del pipe (conserva `fd[0]`), el clon cierra **ambos** extremos, y C usa la cola.

**Trampa central:** tras `execv`, C **perdió la variable `msqid`**. No puede usar la cola "heredada" como hacía B. Solución: pasarle la **clave** por `argv` y que haga su propio `msgget` (sin `IPC_CREAT`, la cola ya existe). También hay que duplicar en C la `struct mensaje` y el `TIPO_MSG`.

**Probar:** `ps` muestra `./proceso_c 2026` (la clave, no un descriptor).

---

## V3 — `dup2`: C lee de su entrada estándar

**Enunciado posible:** "El clon de B debe redirigir el extremo de lectura del pipe a su entrada estándar antes de ejecutar `proceso_c`. `proceso_c` no recibe argumentos."

**Qué cambia:** en el clon, `dup2(fd[0], STDIN_FILENO)` y `close(fd[0])` antes del `execv`; `args` queda solo con el nombre. C lee de `STDIN_FILENO` sin saber que existe un pipe.

**Trampa:** el `dup2` va **antes** del `execv`. Después ya no hay código tuyo que ejecutar.

**Probar:** `ps` muestra `./proceso_c` sin argumentos, y C informa que lee del fd 0.

---

## V4 — FIFO en lugar de pipe

**Enunciado posible:** "Señal 5 → A envía a C a través del FIFO `/tmp/fifoControl`. C recibe como argumento la ruta del FIFO."

**Qué cambia:** `mkfifo` en vez de `pipe`; desaparece toda la herencia y el cierre de extremos (B no toca nada); se pasa la **ruta** por `argv`; A abre con `open(O_WRONLY)` y C con `open(O_RDONLY)`; al terminar, `unlink`.

**Trampa central:** `open` de un FIFO **bloquea hasta que el otro extremo abre**. Si A abriera el FIFO *antes* del `fork`, se quedaría bloqueado para siempre: C todavía no existe. Por eso el `open` de A va **después** del `fork`.

**Diferencia conceptual clave:** el pipe exige parentesco (se hereda); el FIFO no (se encuentra por nombre). Por eso aquí no hay tabla de cierres.

**Probar:** además de `kill -5`, desde otra terminal `echo hola > /tmp/fifoControl` también llega a C. `echo` añade un `\n` que C imprime tal cual.

---

## V5 — B confirma a A por la misma cola

**Enunciado posible:** "Al recibir el mensaje, B debe enviar una confirmación a A por la misma cola usando el tipo 2."

**Qué cambia:** B, tras imprimir, envía un mensaje con `mtype = TIPO_ACK`. A, tras enviar, hace `msgrcv(..., TIPO_ACK, 0)`.

**Trampa central:** **una cola, dos canales lógicos separados por el tipo.** Si B recibiera con `msgtyp = 0` en vez de `TIPO_MSG`, se leería **su propia confirmación**. Si A recibiera con 0, se podría llevar un mensaje dirigido a B.

**Probar:** `kill -2 <A>` → A imprime el envío y luego la confirmación de B.

---

## V6 — B y C hermanos (hijos de A)

**Enunciado posible:** "A crea dos hijos: B, que recibe por la cola, y un segundo hijo que se transforma con `execv` en `proceso_c`."

**Qué cambia:** B ya no hace `fork`; el segundo `fork` lo hace A. `terminar` espera a los dos hijos.

**Trampa central (verificada):** A debe cerrar `fd[0]` **después** del segundo `fork`. Si lo cierra antes, C nace sin el extremo de lectura:
```
read: Bad file descriptor      <- C
write: Broken pipe             <- A, porque ya no queda ningún lector
```
**Trampa secundaria:** C ya no hereda el `SIG_IGN` de B; hay que ponerlo en el hijo antes del `execv`.

**Probar:** en `ps`, el PPID de C es el PID de A, no el de B.

---

## V7 — C avisa a A con una señal

**Enunciado posible:** "Cuando C recibe un mensaje, debe notificar a A enviándole SIGUSR1. A debe imprimir la confirmación."

**Qué cambia:** A guarda su PID en `pid_a` antes de los `fork`; el clon pasa **dos** argumentos (`fd` y `pid_a`); C hace `kill(pid_a, SIGUSR1)`; A registra un tercer manejador.

**Trampa central:** en C, `getppid()` devuelve el PID de **B**, no el de A. El PID de A hay que pasarlo explícitamente por `argv`.

**Trampa secundaria:** A imprime **antes** de escribir en el pipe. C responde en microsegundos; si A estuviera a mitad del `printf`, el salto lo interrumpiría.

**Probar:** `ps` muestra `./proceso_c 3 <PID_A>`. Tras `kill -5`, A imprime el envío y después la confirmación.

---

## V8 — B reenvía a C (relé)

**Enunciado posible:** "Señal 2: A envía a B por la cola, y B reenvía el mensaje a C por el pipe. Señal 5: A envía directamente a C por el pipe."

**Qué cambia:** B conserva `fd[1]` y escribe en el pipe cada mensaje que recibe. El pipe pasa a tener **dos escritores**.

**Trampa central (verificada):** B debe cerrar su `fd[1]` al terminar. Si no, se produce un **bloqueo circular**: C espera EOF, que nunca llega porque B tiene un escritor abierto; B espera a que C termine; A espera a B. Los tres quedan vivos indefinidamente.

**Nota:** escrituras de hasta `PIPE_BUF` (4096 bytes en Linux) son atómicas, así que los mensajes de A y de B no se mezclan a mitad.

---

## V9 — Cola de mensajes POSIX

**Enunciado posible:** "Implementar la comunicación A → B con una cola de mensajes POSIX." (El control original dice "POSIX/System V".)

**Qué cambia:**

| System V | POSIX |
|---|---|
| `msgget(clave, IPC_CREAT \| 0666)` | `mq_open("/nombre", O_CREAT \| O_RDWR, 0666, &attr)` |
| `struct { long mtype; ... }` | bytes sueltos + prioridad |
| `msgsnd(id, &m, len, 0)` | `mq_send(cola, texto, len, prioridad)` |
| `msgrcv(id, &m, max, tipo, 0)` | `mq_receive(cola, buf, max, &prio)` |
| filtrar por `tipo` | no existe: sale el de **mayor prioridad** |
| `msgctl(id, IPC_RMID, NULL)` | `mq_close` + `mq_unlink` |
| `ipcs -q` | `ls /dev/mqueue` |
| `<sys/msg.h>` | `<mqueue.h>`, enlazar con `-lrt` |

**Trampas:**
- El búfer de `mq_receive` debe ser **al menos** `mq_msgsize`; si no, falla con `EMSGSIZE`.
- `mq_unlink` **no despierta** a quien está bloqueado en `mq_receive` (a diferencia de `IPC_RMID`). Por eso A envía un mensaje `"FIN"` para que B salga.
- El nombre debe empezar con `/`.

---

## Variantes pequeñas (sin carpeta propia)

| Si piden... | Cambio |
|---|---|
| Señal 9 o 19 | **No se puede**: SIGKILL y SIGSTOP no son capturables. La respuesta correcta es explicarlo |
| Terminar tras N mensajes | En `enviar_*`, `if (contador == N) terminar();` |
| Que A termine matando a los hijos | En `terminar`, `kill(pid_b, SIGTERM)` antes del `waitpid` |
| Usar `execl` en vez de `execv` | `execl("./proceso_c", "./proceso_c", texto_fd, (char *) NULL);` |
| Que B tarde 5 s en procesar | `sleep(5)` tras imprimir en B; los mensajes se acumulan en la cola (`ipcs -q`) |
| Que B lea todos los tipos | `msgrcv(..., 0, 0)` en vez de `TIPO_MSG` |
| Que A mande distintos tipos según la señal | `m.mtype = (senal == SIGINT) ? 1 : 2;` y B distingue con `switch (m.mtype)` |
| Que C responda a A por un pipe | Segundo `pipe()` **antes** de los `fork`; C escribe, A lee |
| Mensaje escrito por el usuario | `./proceso_a "texto"` y usar `argv[1]` al construir el mensaje |
| Salida de C a un archivo | En el clon, `open` + `dup2(fd_archivo, STDOUT_FILENO)` antes del `execv` |
| Que B también atienda una señal | Registrar su manejador **dentro** de `proceso_b`, después de su `fork` |

---

## Las preguntas que resuelven cualquier variante

Ante un enunciado nuevo, antes de escribir código:

1. **¿Quién crea a quién?** Dibuja el árbol. Define dónde van los `fork`.
2. **¿Quién escribe y quién lee cada canal?** Define qué extremos cierra cada proceso.
3. **¿El receptor viene de `execv`?** Si sí, perdió todas sus variables: lo que necesite (descriptor, clave, PID) debe llegarle por `argv`, o por un nombre conocido (FIFO, clave, cola POSIX).
4. **¿Quién debe reaccionar a cada señal?** Los manejadores se heredan con `fork` y se pierden con `execv`; `SIG_IGN` sobrevive a ambos.
5. **¿Cómo termina todo?** Quién cierra qué para que cada lector reciba su EOF o su error, y quién espera a quién.
