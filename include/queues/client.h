#ifndef QUEUE_CLIENT_H
#define QUEUE_CLIENT_H

#include "core.h"

// Estructura que mantiene el estado del cliente durante la sesión
struct client_state {
    int server_qid;                     // ID de la cola de mensajes del servidor
    int client_qid;                     // ID de la cola de mensajes del cliente
    pid_t pid;                          // PID del proceso cliente
    char name[MAX_NAME_LENGTH];         // Nombre del cliente
    char current_channel[MAX_NAME_LENGTH]; // Nombre del canal actual
    int running;                        // Flag que indica si el cliente está activo
    int in_channel;                     // Flag que indica si el cliente está en algún canal
};

// Funciones de conexión y comunicación con el servidor
int connect_to_server(struct client_state *client);                    // Establece conexión con el servidor
void disconnect_from_server(struct client_state *client);              // Cierra la conexión con el servidor
int send_join_request(struct client_state *client, const char *channel_name);  // Solicita unirse a un canal
int send_leave_request(struct client_state *client);                   // Solicita salir del canal actual
int send_message(struct client_state *client, const char *message);     // Envía un mensaje al canal actual
int send_list_channels_request(struct client_state *client);            // Solicita lista de canales disponibles
int send_create_channel_request(struct client_state *client, const char *channel_name);  // Solicita crear un canal

// Funciones de interfaz de usuario
void show_menu();                                                       // Muestra el menú principal (no implementado)
void show_help();                                                        // Muestra la ayuda con comandos disponibles
void clear_screen();                                                     // Limpia la pantalla del terminal
int get_user_input(char *buffer, int max_size);                         // Obtiene entrada del usuario (no implementado)

// Funciones de manejo de mensajes y hilos
void* message_listener(void *arg);                                      // Hilo que escucha mensajes del servidor
void handle_server_response(struct message *msg);                       // Procesa respuestas del servidor
void handle_broadcast_message(struct message *msg);                     // Procesa mensajes broadcast de otros clientes

// Funciones de limpieza y manejo de señales
void cleanup_client(struct client_state *client);                       // Limpia recursos del cliente al salir
void signal_handler_client(int signum);                                 // Maneja señales del sistema (SIGINT, SIGTERM)

#endif
