#include "queues/core.h"
#include "queues/client.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>

extern struct client_state client_global;

int connect_to_server(struct client_state *client) {
    // Generar clave para acceder a la cola de mensajes del servidor
    key_t server_key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (server_key == -1) {
        perror("ftok (clave del servidor)");
        return -1;
    }
    
    // Conectar a la cola de mensajes del servidor (solo lectura)
    // msgget es la llamada del sistema de colas de mensajes SysV que crea o abre una cola
    // con qid puedes enviar y recibir mensajes
    client->server_qid = msgget(server_key, 0);
    if (client->server_qid == -1) {
        perror("msgget (servidor no encontrado)");
        return -1;
    }
    
    // Obtener el PID del proceso cliente
    client->pid = getpid();
    
    // Generar clave única para la cola de mensajes del cliente
    key_t client_key = ftok(SERVER_KEY_PATHNAME, client->pid);
    if (client_key == -1) {
        perror("ftok (clave del cliente)");
        return -1;
    }
    
    // Crear la cola de mensajes del cliente
    client->client_qid = msgget(client_key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (client->client_qid == -1) {
        perror("msgget (cola del cliente)");
        return -1;
    }
    
    // Inicializar estado del cliente
    client->running = 1;                    // Cliente activo
    client->in_channel = 0;                 // No está en ningún canal
    client->current_channel[0] = '\0';      // Canal actual vacío
    
    return 0;
}

void disconnect_from_server(struct client_state *client) {
    // Si el cliente está en un canal, salir antes de desconectar
    if (client->in_channel) {
        send_leave_request(client);
    }
}

int send_join_request(struct client_state *client, const char *channel_name) {
    struct message msg;
    
    // Configurar el mensaje de solicitud de unión
    msg.message_type = MSG_JOIN_REQUEST;                    // Tipo de mensaje
    msg.data.client_qid = client->client_qid;                // ID de la cola del cliente
    msg.data.sender_pid = client->pid;                       // PID del cliente
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);  // Nombre del cliente
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, channel_name, MAX_NAME_LENGTH - 1); // Nombre del canal
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';                             // Sin contenido adicional
    msg.data.response_code = 0;                             // Sin código de respuesta
    
    // Enviar mensaje al servidor
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando solicitud de unión");
        return -1;
    }
    
    return 0;
}

int send_leave_request(struct client_state *client) {
    // Verificar que el cliente esté en un canal
    if (!client->in_channel) {
        printf("No estás en ningún canal\n");
        return -1;
    }
    
    struct message msg;
    
    // Configurar el mensaje de solicitud de salida
    msg.message_type = MSG_LEAVE_REQUEST;                   // Tipo de mensaje
    msg.data.client_qid = client->client_qid;               // Cola del cliente
    msg.data.sender_pid = client->pid;                       // PID del cliente
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);  // Nombre del cliente
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, client->current_channel, MAX_NAME_LENGTH - 1);  // Canal actual
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';                             // Sin contenido adicional
    msg.data.response_code = 0;                              // Sin código de respuesta
    
    // Enviar mensaje al servidor
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando solicitud de salida");
        return -1;
    }
    
    return 0;
}

int send_message(struct client_state *client, const char *message) {
    struct message msg;
    
    // Configurar el mensaje para enviar al canal
    msg.message_type = MSG_SEND_MESSAGE;                     // Tipo de mensaje
    msg.data.client_qid = client->client_qid;               // ID de la cola del cliente
    msg.data.sender_pid = client->pid;                       // PID del cliente
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);  // Nombre del cliente
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, client->current_channel, MAX_NAME_LENGTH - 1);  // Canal actual
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.content, message, MAX_MESSAGE_SIZE - 1);  // Contenido del mensaje
    msg.data.content[MAX_MESSAGE_SIZE - 1] = '\0';
    msg.data.response_code = 0;                              // Sin código de respuesta
    
    // Enviar mensaje al servidor
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando mensaje");
        return -1;
    }
    
    return 0;
}

int send_list_channels_request(struct client_state *client) {
    struct message msg;
    
    // Configurar el mensaje de solicitud de lista de canales
    msg.message_type = MSG_LIST_CHANNELS;                     // Tipo de mensaje
    msg.data.client_qid = client->client_qid;                 // ID de la cola del cliente
    msg.data.sender_pid = client->pid;                        // PID del cliente
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);  // Nombre del cliente
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.channel_name[0] = '\0';                         // Sin canal específico
    msg.data.content[0] = '\0';                               // Sin contenido adicional
    msg.data.response_code = 0;                               // Sin código de respuesta
    
    // Enviar solicitud al servidor
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error solicitando lista de canales");
        return -1;
    }
    
    return 0;
}

int send_create_channel_request(struct client_state *client, const char *channel_name) {
    struct message msg;
    
    // Configurar el mensaje de solicitud de creación de canal
    msg.message_type = MSG_CREATE_CHANNEL;                      // Tipo de mensaje
    msg.data.client_qid = client->client_qid;                 // ID de la cola del cliente
    msg.data.sender_pid = client->pid;                        // PID del cliente
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);  // Nombre del cliente
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, channel_name, MAX_NAME_LENGTH - 1);  // Nombre del nuevo canal
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';                               // Sin contenido adicional
    msg.data.response_code = 0;                            // Sin código de respuesta
    
    // Enviar solicitud al servidor
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error solicitando creación de canal");
        return -1;
    }
    
    return 0;
}

void show_help() {
    // Mostrar la lista de comandos disponibles para el usuario
    printf("\n=== Comandos disponibles ===\n");
    printf("create <canal>  - Crear un nuevo canal\n");
    printf("join <canal>    - Unirse a un canal\n");
    printf("leave           - Salir del canal actual\n");
    printf("msg <mensaje>   - Enviar mensaje al canal actual\n");
    printf("list            - Listar canales disponibles\n");
    printf("status          - Ver estado actual\n");
    printf("clear           - Limpiar pantalla\n");
    printf("help            - Mostrar esta ayuda\n");
    printf("quit/exit       - Salir del programa\n");
    printf("============================\n\n");
}

void clear_screen() {
    // Limpiar la pantalla del terminal usando el comando clear
    system("clear");
}

void* message_listener(void *arg) {
    struct client_state *client = (struct client_state*)arg;
    struct message msg;
    
    // Bucle principal del hilo que escucha mensajes del servidor
    while (client->running) {
        // Recibir mensaje de la cola del cliente
        ssize_t result = msgrcv(client->client_qid, &msg, sizeof(msg.data), 0, 0);
        
        if (result == -1) {
            // Si la interrupción es por señal, continuar
            if (errno == EINTR) {
                continue;
            }
            perror("Error recibiendo mensaje");
            break;
        }
        
        // Procesar el mensaje según su tipo
        switch (msg.message_type) {
            case MSG_SERVER_RESPONSE:
                handle_server_response(&msg);      // Respuesta del servidor
                break;
            case MSG_BROADCAST:
                handle_broadcast_message(&msg);    // Mensaje broadcast de otros clientes
                break;
            default:
                printf("Mensaje desconocido recibido\n");
                break;
        }
    }
    
    return NULL;
}

void handle_server_response(struct message *msg) {
    // Mostrar la respuesta del servidor al usuario
    printf("\n[SERVIDOR]: %s\n", msg->data.content);
    
    // Si la respuesta es exitosa, actualizar el estado del cliente
    if (msg->data.response_code == RESPONSE_SUCCESS) {
        if (strstr(msg->data.content, "unido al canal") != NULL) {
            // Cliente se unió exitosamente a un canal
            client_global.in_channel = 1;
        } else if (strstr(msg->data.content, "salido del canal") != NULL) {
            // Cliente salió exitosamente del canal
            client_global.in_channel = 0;
            client_global.current_channel[0] = '\0';
        }
    }
    
    // Mostrar el prompt para la siguiente entrada del usuario
    printf("> ");
    fflush(stdout);
}

void handle_broadcast_message(struct message *msg) {
    // Mostrar mensaje broadcast de otros clientes o del sistema
    if (strcmp(msg->data.sender_name, "Sistema") == 0) {
        // Mensaje del sistema (notificaciones de unión/salida)
        printf("\n[%s]: %s\n", msg->data.sender_name, msg->data.content);
    } else {
        // Mensaje de otro cliente en el canal
        printf("\n[%s en %s]: %s\n", msg->data.sender_name, msg->data.channel_name, msg->data.content);
    }
    
    // Mostrar el prompt para la siguiente entrada del usuario
    printf("> ");
    fflush(stdout);
}

void cleanup_client(struct client_state *client) {
    // Si el cliente está en un canal, salir antes de limpiar
    if (client->in_channel) {
        send_leave_request(client);
        sleep(1);  // Dar tiempo para que se procese la solicitud de salida
    }
    
    // Eliminar la cola de mensajes del cliente
    if (client->client_qid != -1) {
        if (msgctl(client->client_qid, IPC_RMID, NULL) == -1) {
            perror("Error eliminando cola del cliente");
        }
    }
}

void signal_handler_client(int signum) {
    // Manejar señales del sistema (SIGINT, SIGTERM) para limpieza adecuada
    printf("\nRecibida señal %d. Cerrando cliente...\n", signum);
    client_global.running = 0;  // Marcar cliente como no activo para terminar bucles
}