
#ifndef QUEUE_SERVER_H
#define QUEUE_SERVER_H

#include "core.h"

// Funciones auxiliares para búsqueda y manipulación de canales y clientes
struct channel* find_channel_by_name(struct server *srv, const char *name);        // Busca un canal por nombre
struct channel* find_client_channel(struct server *srv, pid_t client_pid);        // Encuentra el canal donde está un cliente
int is_client_in_channel(struct channel *ch, pid_t client_pid);                   // Verifica si un cliente está en un canal
void remove_client_from_channel(struct channel *ch, pid_t client_pid);           // Remueve un cliente de un canal
void send_response(int client_qid, int response_code, const char *message);       // Envía respuesta a un cliente
void broadcast_message(struct server *srv, struct channel *ch, const struct message *msg);  // Envía mensaje a todos los clientes del canal
struct channel* create_channel(struct server *srv, const struct message *msg);   // Crea un nuevo canal

// Manejadores de solicitudes del cliente
void handle_join_request(struct server *srv, struct message *msg);                // Maneja solicitud de unirse a un canal
void handle_leave_request(struct server *srv, struct message *msg);              // Maneja solicitud de salir de un canal
void handle_send_message(struct server *srv, struct message *msg);                // Maneja envío de mensajes
void handle_list_channels(struct server *srv, struct message *msg);               // Maneja solicitud de lista de canales
void handle_create_channel(struct server *srv, struct message *msg);               // Maneja solicitud de crear canal
void handle_delete_channel(struct server *srv, struct message *msg);             // Maneja solicitud de eliminar canal (no implementado)

// Funciones de inicialización, limpieza y manejo de señales
void signal_handler(int signum);                                                  // Maneja señales del sistema (SIGINT, SIGTERM)
void cleanup_server(struct server *srv);                                          // Limpia recursos del servidor al cerrar
void init_server(struct server *srv);                                             // Inicializa el servidor y sus estructuras

#endif