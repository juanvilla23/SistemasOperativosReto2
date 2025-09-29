#include "queues/core.h"
//#include "queues/server.h"
 
struct server srv;

// Prototipos de funciones auxiliares
struct channel* find_channel_by_name(struct server *srv, const char *name);
struct channel* find_client_channel(struct server *srv, pid_t client_pid);
int is_client_in_channel(struct channel *ch, pid_t client_pid);
void remove_client_from_channel(struct channel *ch, pid_t client_pid);
void send_response(int client_qid, int response_code, const char *message);
void broadcast_message(struct server *srv, struct channel *ch, const struct message *msg);
struct channel* create_channel(struct server *srv, const struct message *msg);

// Handlers
void handle_join_request(struct server *srv, struct message *msg);
void handle_leave_request(struct server *srv, struct message *msg);
void handle_send_message(struct server *srv, struct message *msg);
void handle_list_channels(struct server *srv, struct message *msg);
void handle_create_channel(struct server *srv, struct message *msg);
void handle_delete_channel(struct server *srv, struct message *msg);

// Gestión del servidor
void signal_handler(int signum);
void cleanup_server(struct server *srv);
void init_server(struct server *srv);

void handle_join_request(struct server *srv, struct message *msg) {
    // RESTRICCIÓN: Verificar si el cliente ya está en un canal
    struct channel *existing_channel = find_client_channel(srv, msg->data.sender_pid);
    if (existing_channel != NULL) {
        char error_msg[MAX_MESSAGE_SIZE];
        snprintf(error_msg, MAX_MESSAGE_SIZE, 
                "Ya estás en el canal '%s'. Debes salir primero.", 
                existing_channel->name);
        send_response(msg->data.client_qid, RESPONSE_ERROR, error_msg);
        printf("Cliente %d intentó unirse a '%s' pero ya está en '%s'\n", 
               msg->data.sender_pid, msg->data.channel_name, existing_channel->name);
        return;
    }

    struct channel *ch = find_channel_by_name(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_NOT_FOUND, 
                     "Canal no encontrado");
        return;
    }
    
    if (ch->client_count >= MAX_CLIENTS) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_FULL, 
                     "Canal lleno");
        return;
    }
    
    // Agregar cliente al canal
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == -1) {
            ch->clients[i] = msg->data.sender_pid;
            ch->client_count++;
            break;
        }
    }
    
    // Registrar cliente si no existe
    int client_exists = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid == msg->data.sender_pid) {
            client_exists = 1;
            break;
        }
    }
    
    if (!client_exists) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (srv->clients[i].pid == -1) {
                srv->clients[i].pid = msg->data.sender_pid;
                srv->clients[i].qid = msg->data.client_qid;
                strncpy(srv->clients[i].name, msg->data.sender_name, MAX_NAME_LENGTH - 1);
                srv->client_count++;
                break;
            }
        }
    }
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, 
                 "Te has unido al canal exitosamente");
    
    // Notificar a otros usuarios
    struct message broadcast;
    broadcast.message_type = MSG_BROADCAST;
    broadcast.data.client_qid = ch->qid;
    snprintf(broadcast.data.content, MAX_MESSAGE_SIZE, 
            "*** %s se ha unido al canal ***", msg->data.sender_name);
    broadcast_message(srv, ch, &broadcast);
    
    printf("Cliente '%s' (%d) se unió al canal '%s'\n", 
           msg->data.sender_name, msg->data.sender_pid, ch->name);
}

void handle_leave_request(struct server *srv, struct message *msg) {
    struct channel *ch = find_channel_by_name(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_NOT_FOUND, 
                     "Canal no encontrado");
        return;
    }
    
    if (!is_client_in_channel(ch, msg->data.sender_pid)) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, 
                     "No estás en este canal");
        return;
    }
    
    remove_client_from_channel(ch, msg->data.sender_pid);
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, 
                 "Has salido del canal");
    
    // Notificar a otros usuarios
    struct message broadcast;
    broadcast.message_type = MSG_BROADCAST;
    broadcast.data.client_qid = ch->qid;
    snprintf(broadcast.data.content, MAX_MESSAGE_SIZE, 
            "*** %s ha salido del canal ***", msg->data.sender_name);
    broadcast_message(srv, ch, &broadcast);
    
    printf("Cliente '%s' (%d) salió del canal '%s'\n", 
           msg->data.sender_name, msg->data.sender_pid, ch->name);
}

void handle_send_message(struct server *srv, struct message *msg) {
    struct channel *ch = find_channel_by_name(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_NOT_FOUND, 
                     "Canal no encontrado");
        return;
    }
    
    if (!is_client_in_channel(ch, msg->data.sender_pid)) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, 
                     "No estás en este canal");
        return;
    }
    
    broadcast_message(srv, ch, msg);
}

void handle_list_channels(struct server *srv, struct message *msg) {
    char list[MAX_MESSAGE_SIZE * 5] = "Canales disponibles:\n";
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (srv->channels[i].qid != -1) {
            char temp[100];
            snprintf(temp, sizeof(temp), "- %s (%d/%d usuarios)\n", 
                    srv->channels[i].name, 
                    srv->channels[i].client_count, 
                    MAX_CLIENTS);
            strncat(list, temp, sizeof(list) - strlen(list) - 1);
        }
    }
    
    if (srv->channel_count == 0) {
        strcpy(list, "No hay canales disponibles");
    }
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, list);
}

void handle_create_channel(struct server *srv, struct message *msg) {
    if (find_channel_by_name(srv, msg->data.channel_name) != NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, 
                     "El canal ya existe");
        return;
    }
    
    struct channel *new_ch = create_channel(srv, msg);
    
    if (new_ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, 
                     "No se pudo crear el canal");
        return;
    }
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, 
                 "Canal creado exitosamente");
    
    printf("Canal '%s' creado\n", new_ch->name);
}

void handle_delete_channel(struct server *srv, struct message *msg) {
    struct channel *ch = find_channel_by_name(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_NOT_FOUND, 
                     "Canal no encontrado");
        return;
    }
    
    if (ch->client_count > 0) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, 
                     "No se puede eliminar un canal con usuarios");
        return;
    }
    
    msgctl(ch->qid, IPC_RMID, NULL);
    ch->qid = -1;
    ch->name[0] = '\0';
    srv->channel_count--;
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, 
                 "Canal eliminado exitosamente");
    
    printf("Canal '%s' eliminado\n", msg->data.channel_name);
}

// Funciones auxiliares
struct channel* find_channel_by_name(struct server *srv, const char *name) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (srv->channels[i].qid != -1 && 
            strcmp(srv->channels[i].name, name) == 0) {
            return &srv->channels[i];
        }
    }
    return NULL;
}

struct channel* find_client_channel(struct server *srv, pid_t client_pid) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (srv->channels[i].qid != -1) {
            if (is_client_in_channel(&srv->channels[i], client_pid)) {
                return &srv->channels[i];
            }
        }
    }
    return NULL;
}

int is_client_in_channel(struct channel *ch, pid_t client_pid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == client_pid) {
            return 1;
        }
    }
    return 0;
}

void remove_client_from_channel(struct channel *ch, pid_t client_pid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == client_pid) {
            ch->clients[i] = -1;
            ch->client_count--;
            break;
        }
    }
}

void send_response(int client_qid, int response_code, const char *message) {
    struct message response;
    response.message_type = MSG_SERVER_RESPONSE;
    response.data.response_code = response_code;
    strncpy(response.data.content, message, MAX_MESSAGE_SIZE - 1);
    
    if (msgsnd(client_qid, &response, sizeof(response.data), 0) == -1) {
        perror("msgsnd response");
    }
}

void broadcast_message(struct server *srv, struct channel *ch, const struct message *msg) {
    struct message broadcast = *msg;
    broadcast.message_type = MSG_BROADCAST;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] != -1) {
            // Buscar el qid del cliente
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (srv->clients[j].pid == ch->clients[i]) {
                    if (msgsnd(srv->clients[j].qid, &broadcast, 
                              sizeof(broadcast.data), IPC_NOWAIT) == -1) {
                        if (errno != EAGAIN) {
                            perror("msgsnd broadcast");
                        }
                    }
                    break;
                }
            }
        }
    }
}

struct channel* create_channel(struct server *srv, const struct message *msg) {
    if (srv->channel_count + 1 >= MAX_CHANNELS) {
        fprintf(stderr, "Error: Límite de canales alcanzado\n");
        return NULL;
    }
 
    struct channel *new_channel = NULL;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (srv->channels[i].qid == -1) {
            new_channel = &srv->channels[i];
            break;
        }
    }
 
    if (!new_channel) {
        fprintf(stderr, "Error: No se pudo encontrar un canal libre\n");
        return NULL;
    }
 
    new_channel->qid = msgget(IPC_PRIVATE, IPC_CREAT | QUEUE_PERMISSIONS);
    if (new_channel->qid == -1) {
        perror("msgget");
        return NULL;
    }
 
    strncpy(new_channel->name, msg->data.channel_name, MAX_NAME_LENGTH - 1);
    new_channel->client_count = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        new_channel->clients[i] = -1;
    }
    
    srv->channel_count++;
 
    return new_channel;
}
 
void signal_handler(int signum) {
    printf("Shutting down server for action %d...\n", signum);
    srv.running = 0;
}
 
void cleanup_server(struct server *srv) {
    printf("\n");
    printf("╔════════════════════════════════════════╗");
    printf("\n");
    printf("║   CERRANDO SERVIDOR                    ║");
    printf("\n");
    printf("╚════════════════════════════════════════╝");
    printf("\n");
   
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (srv->channels[i].qid != -1) {
            msgctl(srv->channels[i].qid, IPC_RMID, NULL);
            printf("Canal '%s' eliminado\n", srv->channels[i].name);
        }
    }
   
    msgctl(srv->qid, IPC_RMID, NULL);
    printf("✓ Servidor cerrado correctamente\n");
}
 
void init_server(struct server *srv) {
    key_t key;
    if ((key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
 
    if ((srv->qid = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS)) == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
 
    srv->channel_count = 0;
    srv->client_count = 0;
    srv->running = 1;
 
    for (int i = 0; i < MAX_CHANNELS; i++) {
        srv->channels[i].qid = -1;
        srv->channels[i].client_count = 0;
        srv->channels[i].name[0] = '\0';
        for (int j = 0; j < MAX_CLIENTS; j++) {
            srv->channels[i].clients[j] = -1;
        }
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        srv->clients[i].pid = -1;
        srv->clients[i].qid = -1;
        srv->clients[i].name[0] = '\0';
    }
 
    printf("╔════════════════════════════════════════╗");
    printf("\n");
    printf("║   INICIANDO SERVIDOR                   ║");
    printf("\n");
    printf("╚════════════════════════════════════════╝");
    printf("\n");
}

int main() {
    struct message msg;
    init_server(&srv);
 
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
 
    while (srv.running) {
        if (msgrcv(srv.qid, &msg, sizeof(msg.data), 0, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("msgrcv");
            continue;
        }
 
        switch (msg.message_type) {
            case MSG_JOIN_REQUEST:
                handle_join_request(&srv, &msg);
                break;
            case MSG_LEAVE_REQUEST:
                handle_leave_request(&srv, &msg);
                break;
            case MSG_SEND_MESSAGE:
                handle_send_message(&srv, &msg);
                break;
            case MSG_LIST_CHANNELS:
                handle_list_channels(&srv, &msg);
                break;
            case MSG_CREATE_CHANNEL:
                handle_create_channel(&srv, &msg);
                break;
            case MSG_DELETE_CHANNEL:
                handle_delete_channel(&srv, &msg);
                break;
            default:
                fprintf(stderr, "Unknown message type: %ld\n", msg.message_type);
                break;
        }
    }
   
    cleanup_server(&srv);
    return 0;
}