#include "queues/core.h"

int server_qid;   // cola del servidor
int client_qid;   // cola privada del cliente
pid_t client_pid;
char client_name[MAX_NAME_LENGTH];

// Hilo o proceso para escuchar mensajes
void listen_messages() {
    struct message msg;
    while (1) {
        if (msgrcv(client_qid, &msg, sizeof(msg.data), 0, 0) == -1) {
            if (errno == EINTR) continue;
            perror("msgrcv client");
            exit(EXIT_FAILURE);
        }

        switch (msg.message_type) {
            case MSG_SERVER_RESPONSE:
                printf("[SERVER] %s (code=%d)\n", msg.data.content, msg.data.response_code);
                break;
            case MSG_BROADCAST:
                printf("[%s] %s\n", msg.data.sender_name, msg.data.content);
                break;
            default:
                printf("[UNKNOWN MESSAGE TYPE %ld] %s\n", msg.message_type, msg.data.content);
                break;
        }
    }
}

void send_join(const char *channel) {
    struct message msg;
    msg.message_type = MSG_JOIN_REQUEST;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = client_pid;
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.channel_name, channel, MAX_NAME_LENGTH - 1);

    if (msgsnd(server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("msgsnd join");
    }
}

void send_leave(const char *channel) {
    struct message msg;
    msg.message_type = MSG_LEAVE_REQUEST;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = client_pid;
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.channel_name, channel, MAX_NAME_LENGTH - 1);

    if (msgsnd(server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("msgsnd leave");
    }
}

void send_message(const char *channel, const char *text) {
    struct message msg;
    msg.message_type = MSG_SEND_MESSAGE;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = client_pid;
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.channel_name, channel, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.content, text, MAX_MESSAGE_SIZE - 1);

    if (msgsnd(server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("msgsnd message");
    }
}

void send_list_channels() {
    struct message msg;
    msg.message_type = MSG_LIST_CHANNELS;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = client_pid;
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);

    if (msgsnd(server_qid, &msg, sizeof(msg.data), 0) == -1) {
        perror("msgsnd list");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nombre_usuario>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(client_name, argv[1], MAX_NAME_LENGTH - 1);
    client_pid = getpid();

    // Obtener cola del servidor
    key_t key;
    if ((key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    if ((server_qid = msgget(key, QUEUE_PERMISSIONS)) == -1) {
        perror("msgget server");
        exit(EXIT_FAILURE);
    }

    // Crear cola del cliente
    if ((client_qid = msgget(IPC_PRIVATE, IPC_CREAT | QUEUE_PERMISSIONS)) == -1) {
        perror("msgget client");
        exit(EXIT_FAILURE);
    }

    printf("Cliente '%s' iniciado (PID=%d, QID=%d)\n", client_name, client_pid, client_qid);

    // Crear proceso para escuchar mensajes
    pid_t listener = fork();
    if (listener == 0) {
        listen_messages();
        exit(0);
    }

    // Loop de comandos
    char line[512];
    char command[50], arg1[100], arg2[200];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        int args = sscanf(line, "%s %s %[^\n]", command, arg1, arg2);

        if (args == 1 && strcmp(command, "list") == 0) {
            send_list_channels();
        } else if (args == 2 && strcmp(command, "join") == 0) {
            send_join(arg1);
        } else if (args == 2 && strcmp(command, "leave") == 0) {
            send_leave(arg1);
        } else if (args == 3 && strcmp(command, "send") == 0) {
            send_message(arg1, arg2);
        } else if (args == 1 && strcmp(command, "quit") == 0) {
            msgctl(client_qid, IPC_RMID, NULL);
            kill(listener, SIGTERM);
            printf("Cliente cerrado.\n");
            break;
        } else {
            printf("Comandos disponibles:\n");
            printf("  list\n");
            printf("  join <canal>\n");
            printf("  leave <canal>\n");
            printf("  send <canal> <mensaje>\n");
            printf("  quit\n");
        }
    }

    return 0;
}
