#include "queues/core.h"
#include "queues/server.h"

extern struct server srv;

struct channel* find_channel_by_name(struct server *srv, const char *name) {
    // Buscar un canal por su nombre en la lista de canales del servidor
    for (int i = 0; i < srv->channel_count; i++) {
        if (strcmp(srv->channels[i].name, name) == 0) {
            return &srv->channels[i];
        }
    }
    return NULL;  // Canal no encontrado
}

struct channel* find_client_channel(struct server *srv, pid_t client_pid) {
    // Buscar en qué canal está un cliente específico
    for (int i = 0; i < srv->channel_count; i++) {
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (srv->channels[i].clients[j] == client_pid) {
                return &srv->channels[i];
            }
        }
    }
    return NULL;  // Cliente no encontrado en ningún canal
}

int is_client_in_channel(struct channel *ch, pid_t client_pid) {
    // Verificar si un cliente específico está en un canal dado
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == client_pid) {
            return 1;  // Cliente encontrado
        }
    }
    return 0;  // Cliente no encontrado
}

void remove_client_from_channel(struct channel *ch, pid_t client_pid) {
    // Remover un cliente de un canal específico
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == client_pid) {
            ch->clients[i] = 0;        // Liberar el slot
            ch->client_count--;        // Decrementar contador de clientes
            break;
        }
    }
}

void send_response(int client_qid, int response_code, const char *message) {
    struct message response;
    
    // Configurar el mensaje de respuesta del servidor
    response.message_type = MSG_SERVER_RESPONSE;
    response.data.client_qid = 0;                        // Sin cola específica
    response.data.sender_pid = getpid();                  // PID del servidor
    strcpy(response.data.sender_name, "Servidor");
    response.data.channel_name[0] = '\0';                // Sin canal específico
    strncpy(response.data.content, message, MAX_MESSAGE_SIZE - 1);
    response.data.content[MAX_MESSAGE_SIZE - 1] = '\0';
    response.data.response_code = response_code;          // Código de respuesta
    
    // Enviar respuesta al cliente (no bloqueante)
    if (msgsnd(client_qid, &response, sizeof(response.data), IPC_NOWAIT) == -1) {
        perror("Error enviando respuesta al cliente");
    }
}

void broadcast_message(struct server *srv, struct channel *ch, const struct message *msg) {
    // Enviar un mensaje a todos los clientes de un canal específico
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] != 0) {  // Si hay un cliente en este slot
            // Buscar la información del cliente en el servidor
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (srv->clients[j].pid == ch->clients[i]) {
                    struct message broadcast = *msg;  // Copiar el mensaje
                    // Enviar mensaje al cliente (no bloqueante)
                    if (msgsnd(srv->clients[j].qid, &broadcast, sizeof(broadcast.data), IPC_NOWAIT) == -1) {
                        perror("Error enviando mensaje broadcast");
                    }
                    break;
                }
            }
        }
    }
}

struct channel* create_channel(struct server *srv, const struct message *msg) {
    // Verificar que no se haya alcanzado el límite máximo de canales
    if (srv->channel_count >= MAX_CHANNELS) {
        return NULL;
    }
    
    // Obtener referencia al nuevo canal
    struct channel *ch = &srv->channels[srv->channel_count];
    
    // Configurar el nombre del canal
    strncpy(ch->name, msg->data.channel_name, MAX_NAME_LENGTH - 1);
    ch->name[MAX_NAME_LENGTH - 1] = '\0';
    ch->client_count = 0;  // Inicializar sin clientes
    
    // Inicializar array de clientes como vacío
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ch->clients[i] = 0;
    }
    
    // Incrementar contador de canales del servidor
    srv->channel_count++;
    return ch;
}

void signal_handler(int signum) {
    // Manejar señales del sistema (SIGINT, SIGTERM) para cierre limpio
    printf("\nSeñal recibida: %d. Cerrando servidor...\n", signum);
    srv.running = 0;  // Marcar servidor como no activo para terminar bucles
}

void cleanup_server(struct server *srv) {
    printf("Limpiando recursos del servidor...\n");
    
    // Crear mensaje de cierre para notificar a todos los clientes
    struct message shutdown_msg;
    shutdown_msg.message_type = MSG_SERVER_RESPONSE;
    shutdown_msg.data.response_code = RESPONSE_ERROR;
    strcpy(shutdown_msg.data.content, "Servidor cerrándose");
    
    // Notificar a todos los clientes registrados sobre el cierre
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid != 0) {
            msgsnd(srv->clients[i].qid, &shutdown_msg, sizeof(shutdown_msg.data), IPC_NOWAIT);
        }
    }
    
    // Eliminar la cola de mensajes del servidor
    if (msgctl(srv->qid, IPC_RMID, NULL) == -1) {
        perror("Error eliminando cola de mensajes");
    } else {
        printf("Cola de mensajes eliminada correctamente\n");
    }
}

void init_server(struct server *srv) {
    // Generar clave para la cola de mensajes del servidor
    key_t key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    
    // Crear la cola de mensajes del servidor
    srv->qid = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (srv->qid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    // Inicializar contadores del servidor
    srv->channel_count = 0;
    srv->client_count = 0;
    srv->running = 1;  // Servidor activo
    
    // Inicializar array de canales
    for (int i = 0; i < MAX_CHANNELS; i++) {
        srv->channels[i].name[0] = '\0';           // Nombre vacío
        srv->channels[i].client_count = 0;          // Sin clientes
        for (int j = 0; j < MAX_CLIENTS; j++) {
            srv->channels[i].clients[j] = 0;       // Sin clientes
        }
    }
    
    // Inicializar array de clientes
    for (int i = 0; i < MAX_CLIENTS; i++) {
        srv->clients[i].pid = 0;                   // Sin PID
        srv->clients[i].qid = 0;                   // Sin cola
        srv->clients[i].name[0] = '\0';            // Sin nombre
        srv->clients[i].current_channel_index = -1; // Sin canal
    }
    
    printf("Servidor inicializado correctamente\n");
    printf("ID de cola de mensajes: %d\n", srv->qid);
}