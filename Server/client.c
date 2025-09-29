#include "queues/core.h"

int server_qid;
int client_qid;
int channel_qid = -1;
char client_name[MAX_NAME_LENGTH];
char current_channel[MAX_NAME_LENGTH];
pid_t listener_pid = -1;

void cleanup_client() {
    if (listener_pid > 0) {
        kill(listener_pid, SIGTERM);
    }
    if (client_qid != -1) {
        msgctl(client_qid, IPC_RMID, NULL);
    }
}

void signal_handler(int signum) {
    printf("\n\n👋 Cerrando cliente...\n");
    cleanup_client();
    exit(0);
}

void listener_process(int ch_qid, const char *name) {
    struct message msg;
    
    while (1) {
        if (msgrcv(ch_qid, &msg, sizeof(struct message_text), MSG_BROADCAST, 0) == -1) {
            break;
        }
        
        printf("\r💬 [%s] %s: %s\n", 
               current_channel, msg.data.sender_name, msg.data.content);
        printf("%s> ", name);
        fflush(stdout);
    }
}

int connect_to_server() {
    key_t key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        return -1;
    }
    
    server_qid = msgget(key, 0);
    if (server_qid == -1) {
        perror("msgget - ¿El servidor está corriendo?");
        return -1;
    }
    
    client_qid = msgget(IPC_PRIVATE, IPC_CREAT | QUEUE_PERMISSIONS);
    if (client_qid == -1) {
        perror("msgget client");
        return -1;
    }
    
    return 0;
}

int join_channel(const char *channel_name) {
    struct message msg;
    
    msg.message_type = MSG_JOIN_REQUEST;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = getpid();
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.channel_name, channel_name, MAX_NAME_LENGTH - 1);
    
    if (msgsnd(server_qid, &msg, sizeof(struct message_text), 0) == -1) {
        perror("msgsnd join");
        return -1;
    }
    
    if (msgrcv(client_qid, &msg, sizeof(struct message_text), MSG_SERVER_RESPONSE, 0) == -1) {
        perror("msgrcv response");
        return -1;
    }
    
    if (msg.data.response_code != RESPONSE_SUCCESS) {
        printf("✗ Error al unirse al canal\n");
        return -1;
    }
    
    channel_qid = msg.data.client_qid;
    strncpy(current_channel, channel_name, MAX_NAME_LENGTH - 1);
    
    // Crear proceso hijo para escuchar mensajes
    listener_pid = fork();
    if (listener_pid == 0) {
        listener_process(channel_qid, client_name);
        exit(0);
    }
    
    printf("✓ Te has unido a '%s'\n", channel_name);
    
    return 0;
}

int send_message_to_channel(const char *content) {
    if (channel_qid == -1) {
        printf("✗ No estás en ningún canal. Usa: join <canal>\n");
        return -1;
    }
    
    struct message msg;
    msg.message_type = MSG_SEND_MESSAGE;
    msg.data.client_qid = client_qid;
    msg.data.sender_pid = getpid();
    strncpy(msg.data.sender_name, client_name, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.channel_name, current_channel, MAX_NAME_LENGTH - 1);
    strncpy(msg.data.content, content, MAX_MESSAGE_SIZE - 1);
    
    if (msgsnd(server_qid, &msg, sizeof(struct message_text), 0) == -1) {
        perror("msgsnd message");
        return -1;
    }
    
    return 0;
}

void print_help() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║          COMANDOS DISPONIBLES         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("  join <canal>  - Unirse a un canal\n");
    printf("  leave         - Salir del canal actual\n");
    printf("  help          - Mostrar esta ayuda\n");
    printf("  quit          - Salir del cliente\n");
    printf("  <mensaje>     - Enviar mensaje al canal\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <nombre>\n", argv[0]);
        printf("Ejemplo: %s María\n", argv[0]);
        return 1;
    }
    
    strncpy(client_name, argv[1], MAX_NAME_LENGTH - 1);
    client_name[MAX_NAME_LENGTH - 1] = '\0';
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (connect_to_server() == -1) {
        return 1;
    }
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║       BIENVENIDO AL CHAT, %-12s ║\n", client_name);
    printf("╚════════════════════════════════════════╝\n");
    print_help();
    
    char input[MAX_MESSAGE_SIZE];
    
    while (1) {
        printf("%s> ", client_name);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remover salto de línea
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }
        
        if (strcmp(input, "help") == 0) {
            print_help();
            continue;
        }
        
        if (strncmp(input, "join ", 5) == 0) {
            char *channel = input + 5;
            if (strlen(channel) > 0) {
                join_channel(channel);
            } else {
                printf("✗ Uso: join <nombre_canal>\n");
            }
            continue;
        }
        
        if (strcmp(input, "leave") == 0) {
            if (channel_qid != -1) {
                if (listener_pid > 0) {
                    kill(listener_pid, SIGTERM);
                    listener_pid = -1;
                }
                channel_qid = -1;
                printf("✓ Has salido del canal '%s'\n", current_channel);
                current_channel[0] = '\0';
            } else {
                printf("✗ No estás en ningún canal\n");
            }
            continue;
        }
        
        // Si no es un comando, es un mensaje
        send_message_to_channel(input);
    }
    
    cleanup_client();
    printf("👋 ¡Hasta luego!\n");
    
    return 0;
}