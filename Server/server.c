#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SERVER_KEY_PATHNAME "/tmp/chat_server"
#define PROJECT_ID 33
#define QUEUE_PERMISSIONS 0666

#define MAX_MESSAGE_SIZE 256
#define MAX_CHANNELS 10
#define MAX_CLIENTS_PER_CHANNEL 10
#define MAX_CLIENTS_GLOBAL 50
#define MAX_NAME_LENGTH 50

#define MSG_JOIN_REQUEST    1
#define MSG_LEAVE_REQUEST   2
#define MSG_SEND_MESSAGE    3
#define MSG_SERVER_RESPONSE 4
#define MSG_BROADCAST       5
#define MSG_LIST_CHANNELS   6

#define RESPONSE_SUCCESS    0
#define RESPONSE_ERROR      -1
#define RESPONSE_CHANNEL_FULL -2
#define RESPONSE_CHANNEL_NOT_FOUND -3

struct message_text {
    int client_qid;
    pid_t sender_pid;
    char sender_name[MAX_NAME_LENGTH];
    char channel_name[MAX_NAME_LENGTH];
    char content[MAX_MESSAGE_SIZE];
    int response_code;
};

struct message {
    long message_type;
    struct message_text data;
};

struct client {
    int qid;
    pid_t pid;
    char name[MAX_NAME_LENGTH];
    time_t last_activity;
    int active;
};

struct channel {
    int qid;
    pid_t clients[MAX_CLIENTS_PER_CHANNEL];
    char name[MAX_NAME_LENGTH];
    int client_count;
    int active;
    time_t created_at;
};

struct server {
    int qid;
    struct channel channels[MAX_CHANNELS];
    struct client clients[MAX_CLIENTS_GLOBAL];
    int channel_count;
    int client_count;
    int running;
};

struct server *global_server = NULL;

// Prototipos
void init_server(struct server *srv);
void cleanup_server(struct server *srv);
void signal_handler(int signum);

struct channel* find_channel(struct server *srv, const char *name);
struct channel* create_channel(struct server *srv, const char *name);
int add_client_to_channel(struct channel *ch, pid_t client_pid);
int remove_client_from_channel(struct channel *ch, pid_t client_pid);

void handle_join_request(struct server *srv, struct message *msg);
void handle_leave_request(struct server *srv, struct message *msg);
void handle_send_message(struct server *srv, struct message *msg);
void broadcast_to_channel(struct channel *ch, struct message *msg);

struct client* register_client(struct server *srv, pid_t pid, const char *name, int qid);
struct client* find_client(struct server *srv, pid_t pid);

// ==================== IMPLEMENTACIONES ====================

void init_server(struct server *srv) {
    key_t key;
    
    // Asegurar que el archivo existe
    FILE *fp = fopen(SERVER_KEY_PATHNAME, "a");
    if (fp) fclose(fp);
    
    key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    srv->qid = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (srv->qid == -1) {
        perror("msgget");
        exit(1);
    }
    
    srv->channel_count = 0;
    srv->client_count = 0;
    srv->running = 1;
    
    memset(srv->channels, 0, sizeof(srv->channels));
    memset(srv->clients, 0, sizeof(srv->clients));
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║   SERVIDOR DE CHAT INICIADO           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("✓ QID del servidor: %d\n", srv->qid);
    printf("✓ Esperando conexiones...\n\n");
}

struct channel* find_channel(struct server *srv, const char *name) {
    for (int i = 0; i < srv->channel_count; i++) {
        if (srv->channels[i].active && 
            strcmp(srv->channels[i].name, name) == 0) {
            return &srv->channels[i];
        }
    }
    return NULL;
}

struct channel* create_channel(struct server *srv, const char *name) {
    if (srv->channel_count >= MAX_CHANNELS) {
        fprintf(stderr, "✗ Error: Límite de canales alcanzado\n");
        return NULL;
    }
    
    struct channel *ch = &srv->channels[srv->channel_count];
    
    ch->qid = msgget(IPC_PRIVATE, IPC_CREAT | QUEUE_PERMISSIONS);
    if (ch->qid == -1) {
        perror("msgget channel");
        return NULL;
    }
    
    strncpy(ch->name, name, MAX_NAME_LENGTH - 1);
    ch->name[MAX_NAME_LENGTH - 1] = '\0';
    ch->client_count = 0;
    ch->active = 1;
    ch->created_at = time(NULL);
    
    srv->channel_count++;
    
    printf("📢 Canal '%s' creado (QID: %d)\n", ch->name, ch->qid);
    
    return ch;
}

int add_client_to_channel(struct channel *ch, pid_t client_pid) {
    if (ch->client_count >= MAX_CLIENTS_PER_CHANNEL) {
        return -1;
    }
    
    for (int i = 0; i < ch->client_count; i++) {
        if (ch->clients[i] == client_pid) {
            return 0;
        }
    }
    
    ch->clients[ch->client_count++] = client_pid;
    return 0;
}

int remove_client_from_channel(struct channel *ch, pid_t client_pid) {
    for (int i = 0; i < ch->client_count; i++) {
        if (ch->clients[i] == client_pid) {
            // Mover el último elemento a esta posición
            ch->clients[i] = ch->clients[ch->client_count - 1];
            ch->client_count--;
            return 0;
        }
    }
    return -1;
}

struct client* find_client(struct server *srv, pid_t pid) {
    for (int i = 0; i < srv->client_count; i++) {
        if (srv->clients[i].active && srv->clients[i].pid == pid) {
            return &srv->clients[i];
        }
    }
    return NULL;
}

struct client* register_client(struct server *srv, pid_t pid, const char *name, int qid) {
    if (srv->client_count >= MAX_CLIENTS_GLOBAL) {
        return NULL;
    }
    
    struct client *cli = find_client(srv, pid);
    if (cli != NULL) {
        return cli;
    }
    
    cli = &srv->clients[srv->client_count];
    cli->pid = pid;
    cli->qid = qid;
    strncpy(cli->name, name, MAX_NAME_LENGTH - 1);
    cli->name[MAX_NAME_LENGTH - 1] = '\0';
    cli->last_activity = time(NULL);
    cli->active = 1;
    
    srv->client_count++;
    
    printf("👤 Cliente '%s' (PID: %d) registrado\n", cli->name, cli->pid);
    
    return cli;
}

void handle_join_request(struct server *srv, struct message *msg) {
    printf("\n→ JOIN: %s quiere unirse a '%s'\n", 
           msg->data.sender_name, msg->data.channel_name);
    
    // Registrar cliente si no existe
    register_client(srv, msg->data.sender_pid, 
                   msg->data.sender_name, msg->data.client_qid);
    
    struct channel *ch = find_channel(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        ch = create_channel(srv, msg->data.channel_name);
        if (ch == NULL) {
            msg->message_type = MSG_SERVER_RESPONSE;
            msg->data.response_code = RESPONSE_ERROR;
            msgsnd(msg->data.client_qid, msg, sizeof(struct message_text), 0);
            printf("✗ Error al crear canal\n");
            return;
        }
    }
    
    if (add_client_to_channel(ch, msg->data.sender_pid) == -1) {
        msg->message_type = MSG_SERVER_RESPONSE;
        msg->data.response_code = RESPONSE_CHANNEL_FULL;
        msgsnd(msg->data.client_qid, msg, sizeof(struct message_text), 0);
        printf("✗ Canal lleno\n");
        return;
    }
    
    struct message response;
    response.message_type = MSG_SERVER_RESPONSE;
    response.data.response_code = RESPONSE_SUCCESS;
    response.data.client_qid = ch->qid;
    strcpy(response.data.channel_name, ch->name);
    
    msgsnd(msg->data.client_qid, &response, sizeof(struct message_text), 0);
    
    printf("✓ %s se unió a '%s' (miembros: %d)\n", 
           msg->data.sender_name, ch->name, ch->client_count);
}

void handle_leave_request(struct server *srv, struct message *msg) {
    printf("\n→ LEAVE: %s quiere salir de '%s'\n", 
           msg->data.sender_name, msg->data.channel_name);
    
    struct channel *ch = find_channel(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        msg->message_type = MSG_SERVER_RESPONSE;
        msg->data.response_code = RESPONSE_CHANNEL_NOT_FOUND;
        msgsnd(msg->data.client_qid, msg, sizeof(struct message_text), 0);
        return;
    }
    
    remove_client_from_channel(ch, msg->data.sender_pid);
    
    msg->message_type = MSG_SERVER_RESPONSE;
    msg->data.response_code = RESPONSE_SUCCESS;
    msgsnd(msg->data.client_qid, msg, sizeof(struct message_text), 0);
    
    printf("✓ %s salió de '%s' (miembros: %d)\n", 
           msg->data.sender_name, ch->name, ch->client_count);
}

void broadcast_to_channel(struct channel *ch, struct message *msg) {
    struct client *sender_client = NULL;
    
    // Buscar el cliente del remitente
    for (int i = 0; i < ch->client_count; i++) {
        if (ch->clients[i] == msg->data.sender_pid) {
            continue; // No enviar al remitente
        }
        
        // Enviar a todos los demás miembros del canal
        struct message broadcast_msg;
        broadcast_msg.message_type = MSG_BROADCAST;
        broadcast_msg.data = msg->data;
        
        // Enviar directamente a la cola del canal
        // Los clientes escuchan en la cola del canal
        if (msgsnd(ch->qid, &broadcast_msg, sizeof(struct message_text), IPC_NOWAIT) == -1) {
            perror("msgsnd broadcast");
        }
    }
}

void handle_send_message(struct server *srv, struct message *msg) {
    struct channel *ch = find_channel(srv, msg->data.channel_name);
    
    if (ch == NULL) {
        printf("✗ Canal '%s' no encontrado\n", msg->data.channel_name);
        return;
    }
    
    printf("💬 [%s] %s: %s\n", 
           ch->name, msg->data.sender_name, msg->data.content);
    
    broadcast_to_channel(ch, msg);
}

void cleanup_server(struct server *srv) {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║   CERRANDO SERVIDOR                   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    for (int i = 0; i < srv->channel_count; i++) {
        if (srv->channels[i].active) {
            msgctl(srv->channels[i].qid, IPC_RMID, NULL);
            printf("✓ Canal '%s' eliminado\n", srv->channels[i].name);
        }
    }
    
    msgctl(srv->qid, IPC_RMID, NULL);
    printf("✓ Cola global eliminada\n");
    printf("✓ Servidor cerrado correctamente\n");
}

void signal_handler(int signum) {
    printf("\n⚠ Señal %d recibida\n", signum);
    if (global_server != NULL) {
        global_server->running = 0;
    }
}

int main() {
    struct server srv;
    struct message msg;
    
    global_server = &srv;
    
    // Configurar manejadores de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Inicializar servidor
    init_server(&srv);
    
    // Bucle principal
    while (srv.running) {
        // Recibir cualquier tipo de mensaje
        if (msgrcv(srv.qid, &msg, sizeof(struct message_text), 0, 0) == -1) {
            if (srv.running) {
                perror("msgrcv");
            }
            break;
        }
        
        // Procesar según el tipo de mensaje
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
                // TODO: Implementar listado de canales
                printf("→ LIST: Solicitud de listado de canales\n");
                break;
                
            default:
                printf("⚠ Tipo de mensaje desconocido: %ld\n", msg.message_type);
                break;
        }
    }
    
    cleanup_server(&srv);
    
    return 0;
}