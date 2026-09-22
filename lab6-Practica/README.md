# Kit de Sistemas Operativos — señales, IPC y procesos

Colección de programas mínimos, probados y comentados, para consultar y
ensamblar durante el examen. Cada archivo resuelve **un** caso.

```bash
make            # compila todo (26 binarios)
make clean      # borra binarios, el FIFO y la cola de prueba
bash probar_todo.sh   # ejecuta todos los casos y muestra su salida
```

---

## Índice

### `00-referencia/` — leer primero
| Archivo | Contenido |
|---|---|
| `referencia.md` | cabeceras, prototipos, valores de retorno, **reglas de integración**, comandos |
| `errores.md` | catálogo de errores de compilación y ejecución con su solución |
| `plantilla.c` | esqueleto con todas las cabeceras y `die()` |

### `01-senales/`
| Archivo | Caso |
|---|---|
| `senal_basica.c` | `signal()` con un manejador para varias señales, `pause()` |
| `senal_flag.c` | patrón seguro: el manejador solo levanta una bandera |
| `senal_sigaction.c` | `sigaction()` con `SA_SIGINFO`: saber quién envió la señal |
| `senal_enviar.c` | enviar señales desde C con `kill()` |

### `02-saltos/`
| Archivo | Caso |
|---|---|
| `salto_un_punto.c` | un `sigjmp_buf`, N señales distinguidas por el valor de retorno |
| `salto_varios_puntos.c` | un `sigjmp_buf` por señal ("cada handler con su punto") |

### `03-cola/`
| Archivo | Caso |
|---|---|
| `cola_comun.h` | clave y estructura compartidas |
| `cola_emisor.c` | `./cola_emisor <tipo> "<texto>"` |
| `cola_receptor.c` | `./cola_receptor [tipo]` — 0 = cualquiera; Ctrl+C borra la cola |

### `04-fifo/`
| Archivo | Caso |
|---|---|
| `fifo_escritor.c` | `mkfifo` + `open(O_WRONLY)` + `write` |
| `fifo_lector.c` | `open(O_RDONLY)` + bucle `read` hasta EOF |

### `05-fork/`
| Archivo | Caso |
|---|---|
| `fork_basico.c` | padre e hijo, valores de retorno, `wait` |
| `fork_wait.c` | `waitpid` y código de salida del hijo |
| `fork_arbol.c` | padre → hijo → nieto (la jerarquía del control) |
| `fork_senales.c` | herencia de manejadores, zombis |

### `06-pipe/`
| Archivo | Caso |
|---|---|
| `pipe_padre_hijo.c` | el padre escribe, el hijo lee; cierre de extremos |
| `pipe_bidireccional.c` | dos pipes, ida y vuelta |
| `pipe_senal.c` | el padre escribe en el pipe cuando llega una señal |

### `07-execv/`
| Archivo | Caso |
|---|---|
| `exec_simple.c` | `execv`/`execl`/`execlp`, qué se conserva y qué se pierde |
| `exec_padre.c` + `exec_hijo.c` | pasar el descriptor del pipe por `argv` |

### `08-dup2/`
| Archivo | Caso |
|---|---|
| `dup2_padre.c` + `dup2_hijo.c` | pipe redirigido a `stdin` del hijo |
| `dup2_archivo.c` | `stdout` redirigido a un archivo |

### `09-integracion/`
| Archivo | Caso |
|---|---|
| `integ_a.c` + `integ_c.c` | A → B → clon → `execv` C; A escribe en el pipe al recibir señal |

---

## Cómo usar el kit en el examen

1. **Leer el enunciado y dibujar el árbol de procesos** antes de tocar código. Quién crea a quién, quién habla con quién, por qué canal.
2. **Elegir el punto de partida más cercano.** Si hay `fork` + `execv` + pipe, partir de `09-integracion`. Si hay señales + cola, partir de `03-cola` y `02-saltos`.
3. **Aplicar las reglas de integración** de `referencia.md` §4: orden de `pipe`/`fork`/`signal`, qué extremos cierra cada proceso, qué se hereda.
4. **Compilar con `-Wall -Wextra` a cada paso.** Un aviso es casi siempre un bug.
5. **Ante cualquier error, buscar el texto en `errores.md`.**
6. **Comprobar con la terminal:** `ps -o pid,ppid,cmd`, `ipcs -q`, `ls -l /tmp`.

## Lo que no se puede copiar de aquí

- El **orden** de las líneas en tu programa concreto.
- Qué **extremo del pipe** cierra cada proceso de tu árbol.
- Si los manejadores van **antes o después** del `fork`.
- Los **diagramas** que te pidan.

Eso es lo que hay que practicar antes.
