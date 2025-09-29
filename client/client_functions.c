#include "queues/core.h"
#include "queues/client.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>

extern struct client_state client_global;

int connect_to_server(struct client_state *client) {
    key_t server_key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (server_key == -1) {
        perror("ftok");
        return -1;
    }
    
    client->server_qid = msgget(server_key, 0);
    if (client->server_qid == -1) {
        perror("msgget (servidor no encontrado)");
        return -1;
    }
    
    client->pid = getpid();
    key_t client_key = ftok(SERVER_KEY_PATHNAME, client->pid);
    if (client_key == -1) {
        perror("ftok client");
        return -1;
    }
    
    client->client_qid = msgget(client_key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (client->client_qid == -1) {
        perror("msgget client");
        return -1;
    }
    
    client->running = 1;
    client->in_channel = 0;
    client->current_channel[0] = '\0';
    
    return 0;
}

void disconnect_from_server(struct client_state *client) {
    if (client->in_channel) {
        send_leave_request(client);
    }
}

int send_join_request(struct client_state *client, const char *channel_name) {
    struct message msg;
    msg.message_type = MSG_JOIN_REQUEST;
    msg.data.client_qid = client->client_qid;
    msg.data.sender_pid = client->pid;
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, channel_name, MAX_NAME_LENGTH - 1);
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';
    msg.data.response_code = 0;
    
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando solicitud de unión");
        return -1;
    }
    
    return 0;
}

int send_leave_request(struct client_state *client) {
    if (!client->in_channel) {
        printf("No estás en ningún canal\n");
        return -1;
    }
    
    struct message msg;
    msg.message_type = MSG_LEAVE_REQUEST;
    msg.data.client_qid = client->client_qid;
    msg.data.sender_pid = client->pid;
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, client->current_channel, MAX_NAME_LENGTH - 1);
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';
    msg.data.response_code = 0;
    
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando solicitud de salida");
        return -1;
    }
    
    return 0;
}

int send_message(struct client_state *client, const char *message) {
    struct message msg;
    msg.message_type = MSG_SEND_MESSAGE;
    msg.data.client_qid = client->client_qid;
    msg.data.sender_pid = client->pid;
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, client->current_channel, MAX_NAME_LENGTH - 1);
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.content, message, MAX_MESSAGE_SIZE - 1);
    msg.data.content[MAX_MESSAGE_SIZE - 1] = '\0';
    msg.data.response_code = 0;
    
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error enviando mensaje");
        return -1;
    }
    
    return 0;
}

int send_list_channels_request(struct client_state *client) {
    struct message msg;
    msg.message_type = MSG_LIST_CHANNELS;
    msg.data.client_qid = client->client_qid;
    msg.data.sender_pid = client->pid;
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.channel_name[0] = '\0';
    msg.data.content[0] = '\0';
    msg.data.response_code = 0;
    
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error solicitando lista de canales");
        return -1;
    }
    
    return 0;
}

int send_create_channel_request(struct client_state *client, const char *channel_name) {
    struct message msg;
    msg.message_type = MSG_CREATE_CHANNEL;
    msg.data.client_qid = client->client_qid;
    msg.data.sender_pid = client->pid;
    strncpy(msg.data.sender_name, client->name, MAX_NAME_LENGTH - 1);
    msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(msg.data.channel_name, channel_name, MAX_NAME_LENGTH - 1);
    msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    msg.data.content[0] = '\0';
    msg.data.response_code = 0;
    
    if (msgsnd(client->server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("Error solicitando creación de canal");
        return -1;
    }
    
    return 0;
}

void show_help() {
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
    system("clear");
}

void* message_listener(void *arg) {
    struct client_state *client = (struct client_state*)arg;
    struct message msg;
    
    while (client->running) {
        ssize_t result = msgrcv(client->client_qid, &msg, sizeof(msg.data), 0, 0);
        
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error recibiendo mensaje");
            break;
        }
        
        switch (msg.message_type) {
            case MSG_SERVER_RESPONSE:
                handle_server_response(&msg);
                break;
            case MSG_BROADCAST:
                handle_broadcast_message(&msg);
                break;
            default:
                printf("Mensaje desconocido recibido\n");
                break;
        }
    }
    
    return NULL;
}

void handle_server_response(struct message *msg) {
    printf("\n[SERVIDOR]: %s\n", msg->data.content);
    
    if (msg->data.response_code == RESPONSE_SUCCESS) {
        if (strstr(msg->data.content, "unido al canal") != NULL) {
            client_global.in_channel = 1;
        } else if (strstr(msg->data.content, "salido del canal") != NULL) {
            client_global.in_channel = 0;
            client_global.current_channel[0] = '\0';
        }
    }
    
    printf("> ");
    fflush(stdout);
}

void handle_broadcast_message(struct message *msg) {
    if (strcmp(msg->data.sender_name, "Sistema") == 0) {
        printf("\n[%s]: %s\n", msg->data.sender_name, msg->data.content);
    } else {
        printf("\n[%s en %s]: %s\n", msg->data.sender_name, msg->data.channel_name, msg->data.content);
    }
    printf("> ");
    fflush(stdout);
}

void cleanup_client(struct client_state *client) {
    if (client->in_channel) {
        send_leave_request(client);
        sleep(1);
    }
    
    if (client->client_qid != -1) {
        if (msgctl(client->client_qid, IPC_RMID, NULL) == -1) {
            perror("Error eliminando cola del cliente");
        }
    }
}

void signal_handler_client(int signum) {
    printf("\nRecibida señal %d. Cerrando cliente...\n", signum);
    client_global.running = 0;
}