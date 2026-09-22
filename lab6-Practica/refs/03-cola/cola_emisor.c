/*
 * COLAS DE MENSAJES - EMISOR
 *
 * Uso:  ./cola_emisor <tipo> "<texto>"
 * Ej:   ./cola_emisor 1 "hola"
 *       ./cola_emisor 3 "control next week"
 *
 * msgget(clave, IPC_CREAT | 0666)
 *   traduce la clave (publica) a un msqid (privado del proceso).
 *   IPC_CREAT: crea si no existe, reutiliza si existe.
 *   Igual que open() convierte una ruta en un descriptor.
 *
 * msgsnd(msqid, &msg, tamano, banderas)
 *   tamano = SOLO el texto (sin mtype). strlen + 1 para el '\0'.
 *   IPC_NOWAIT: si la cola esta llena falla con EAGAIN en vez de bloquear.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cola_comun.h"

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char *argv[])
{
    int msqid;
    struct mensaje m;
    size_t len;

    if (argc != 3)
    {
        fprintf(stderr, "uso: %s <tipo> \"<texto>\"\n", argv[0]);
        return 1;
    }

    if ((msqid = msgget(CLAVE, IPC_CREAT | 0666)) < 0)
        die("msgget");

    m.mtype = atol(argv[1]);
    if (m.mtype <= 0)
    {
        fprintf(stderr, "el tipo debe ser > 0\n");
        return 1;
    }
    snprintf(m.mtext, MAXTEXT, "%s", argv[2]);
    len = strlen(m.mtext) + 1;

    if (msgsnd(msqid, &m, len, IPC_NOWAIT) < 0)
        die("msgsnd");

    printf("enviado -> cola %d (clave %d), tipo %ld, %zu bytes: \"%s\"\n",
           msqid, CLAVE, m.mtype, len, m.mtext);
    return 0;
}
