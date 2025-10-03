#ifndef QUEUE_CORE_H
#define QUEUE_CORE_H

// Librerías estándar de C para funcionalidades básicas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

// Librerías para System V IPC (Inter-Process Communication)
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Configuración de la cola de mensajes del servidor
#define SERVER_KEY_PATHNAME "/tmp"  // Ruta para generar la clave del servidor
#define PROJECT_ID 33               // ID del proyecto para ftok()
#define QUEUE_PERMISSIONS 0660      // Permisos de lectura/escritura para propietario y grupo

// Límites del sistema de chat
#define MAX_MESSAGE_SIZE 256         // Tamaño máximo de un mensaje
#define MAX_CHANNELS 10             // Número máximo de canales simultáneos
#define MAX_CLIENTS 10              // Número máximo de clientes por canal
#define MAX_NAME_LENGTH 50          // Longitud máxima de nombres de usuario y canales

// Tipos de mensajes que puede enviar el cliente al servidor
#define MSG_JOIN_REQUEST    1       // Solicitud para unirse a un canal
#define MSG_LEAVE_REQUEST   2       // Solicitud para salir de un canal
#define MSG_SEND_MESSAGE    3       // Envío de mensaje a un canal
#define MSG_SERVER_RESPONSE 4       // Respuesta del servidor al cliente
#define MSG_BROADCAST       5       // Mensaje broadcast a todos los clientes del canal
#define MSG_LIST_CHANNELS   6       // Solicitud de lista de canales disponibles
#define MSG_CREATE_CHANNEL  7       // Solicitud para crear un nuevo canal
#define MSG_DELETE_CHANNEL  8       // Solicitud para eliminar un canal

// Códigos de respuesta del servidor
#define RESPONSE_SUCCESS    0       // Operación exitosa
#define RESPONSE_ERROR      -1      // Error genérico
#define RESPONSE_CHANNEL_FULL -2    // Canal lleno (máximo de clientes alcanzado)
#define RESPONSE_CHANNEL_NOT_FOUND -3  // Canal no encontrado
#define RESPONSE_ALREADY_IN_CHANNEL -4 // Cliente ya está en el canal

// Estructura que contiene los datos del mensaje
struct message_text {
    int client_qid;                     // ID de la cola del cliente que envía el mensaje
    pid_t sender_pid;                   // PID del proceso cliente
    char sender_name[MAX_NAME_LENGTH];  // Nombre del usuario que envía el mensaje
    char channel_name[MAX_NAME_LENGTH]; // Nombre del canal de destino
    char content[MAX_MESSAGE_SIZE];     // Contenido del mensaje
    int response_code;                  // Código de respuesta del servidor
};

// Estructura completa del mensaje para System V message queues
struct message {
    long message_type;                  // Tipo de mensaje (requerido por msgrcv/msgsnd)
    struct message_text data;          // Datos del mensaje
};

// Estructura que representa un cliente conectado
struct client {
    int qid;                           // ID de la cola de mensajes del cliente
    pid_t pid;                         // PID del proceso cliente
    char name[MAX_NAME_LENGTH];        // Nombre del cliente
    int current_channel_index;         // Índice del canal actual (-1 si no está en ningún canal)
};

// Estructura que representa un canal de chat
struct channel {
    int qid;                           // ID de la cola del canal (no usado actualmente)
    pid_t clients[MAX_CLIENTS];        // Array de PIDs de clientes en el canal
    char name[MAX_NAME_LENGTH];        // Nombre del canal
    int client_count;                  // Número actual de clientes en el canal
};

// Estructura principal del servidor que mantiene el estado global
struct server {
    int qid;                           // ID de la cola de mensajes del servidor
    struct channel channels[MAX_CHANNELS];  // Array de canales disponibles
    struct client clients[MAX_CLIENTS];    // Array de clientes registrados
    int channel_count;                 // Número actual de canales creados
    int client_count;                  // Número actual de clientes registrados
    int running;                       // Flag para controlar el bucle principal del servidor
};

#endif