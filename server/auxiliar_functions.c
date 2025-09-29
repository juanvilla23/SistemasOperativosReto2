#include "queues/core.h"
#include "queues/server.h"

extern struct server srv;

struct channel* find_channel_by_name(struct server *srv, const char *name) {
    for (int i = 0; i < srv->channel_count; i++) {
        if (strcmp(srv->channels[i].name, name) == 0) {
            return &srv->channels[i];
        }
    }
    return NULL;
}

struct channel* find_client_channel(struct server *srv, pid_t client_pid) {
    for (int i = 0; i < srv->channel_count; i++) {
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (srv->channels[i].clients[j] == client_pid) {
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
            ch->clients[i] = 0;
            ch->client_count--;
            break;
        }
    }
}

void send_response(int client_qid, int response_code, const char *message) {
    struct message response;
    response.message_type = MSG_SERVER_RESPONSE;
    response.data.client_qid = 0;
    response.data.sender_pid = getpid();
    strcpy(response.data.sender_name, "Servidor");
    response.data.channel_name[0] = '\0';
    strncpy(response.data.content, message, MAX_MESSAGE_SIZE - 1);
    response.data.content[MAX_MESSAGE_SIZE - 1] = '\0';
    response.data.response_code = response_code;
    
    if (msgsnd(client_qid, &response, sizeof(response.data), IPC_NOWAIT) == -1) {
        perror("Error enviando respuesta al cliente");
    }
}

void broadcast_message(struct server *srv, struct channel *ch, const struct message *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] != 0) {
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (srv->clients[j].pid == ch->clients[i]) {
                    struct message broadcast = *msg;
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
    if (srv->channel_count >= MAX_CHANNELS) {
        return NULL;
    }
    
    struct channel *ch = &srv->channels[srv->channel_count];
    strncpy(ch->name, msg->data.channel_name, MAX_NAME_LENGTH - 1);
    ch->name[MAX_NAME_LENGTH - 1] = '\0';
    ch->client_count = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ch->clients[i] = 0;
    }
    
    srv->channel_count++;
    return ch;
}

void signal_handler(int signum) {
    printf("\nSeñal recibida: %d. Cerrando servidor...\n", signum);
    srv.running = 0;
}

void cleanup_server(struct server *srv) {
    printf("Limpiando recursos del servidor...\n");
    
    struct message shutdown_msg;
    shutdown_msg.message_type = MSG_SERVER_RESPONSE;
    shutdown_msg.data.response_code = RESPONSE_ERROR;
    strcpy(shutdown_msg.data.content, "Servidor cerrándose");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid != 0) {
            msgsnd(srv->clients[i].qid, &shutdown_msg, sizeof(shutdown_msg.data), IPC_NOWAIT);
        }
    }
    
    if (msgctl(srv->qid, IPC_RMID, NULL) == -1) {
        perror("Error eliminando cola de mensajes");
    } else {
        printf("Cola de mensajes eliminada correctamente\n");
    }
}

void init_server(struct server *srv) {
    key_t key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    
    srv->qid = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (srv->qid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    srv->channel_count = 0;
    srv->client_count = 0;
    srv->running = 1;
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        srv->channels[i].name[0] = '\0';
        srv->channels[i].client_count = 0;
        for (int j = 0; j < MAX_CLIENTS; j++) {
            srv->channels[i].clients[j] = 0;
        }
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        srv->clients[i].pid = 0;
        srv->clients[i].qid = 0;
        srv->clients[i].name[0] = '\0';
        srv->clients[i].current_channel_index = -1;
    }
    
    printf("Servidor inicializado correctamente\n");
    printf("ID de cola de mensajes: %d\n", srv->qid);
}