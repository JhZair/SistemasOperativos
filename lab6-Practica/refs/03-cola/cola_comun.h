/*
 * COLAS DE MENSAJES - cabecera compartida
 * Emisor y receptor DEBEN usar la misma clave y la misma estructura.
 * Incluir con:  #include "cola_comun.h"
 */
#ifndef COLA_COMUN_H
#define COLA_COMUN_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define CLAVE   1234        /* numero acordado entre procesos      */
#define MAXTEXT 256

/* REGLA: mtype PRIMERO y de tipo long. El kernel lo lee.
 * Lo que viene despues es carga util opaca: puede ser una cadena,
 * varios campos, una estructura entera... */
struct mensaje
{
    long mtype;             /* > 0 obligatorio                     */
    char mtext[MAXTEXT];
};

#endif
