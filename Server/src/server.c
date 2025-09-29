#include "queue_system.h"

int main() {
    key_t key;
    int qid;
    struct message msg;

    if ((key = ftok(SERVER_KEY_PATHNAME, PROJECT_ID)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    if ((qid = msgget(key, QUEUE_PERMISSIONS | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Server: Waiting for messages...\n");
    while (1) {
        if (msgrcv(qid, &msg, sizeof(struct message_text), 0, 0) == -1) {
            perror("msgrcv");
            printf("Server: Error receiving message, continuing...\n");
            exit(EXIT_FAILURE);
            //continue;
        }

        int length = strlen(msg.message_text.buffer);
        char buf[20];
        sprintf(buf, " (len=%d)", length);
        strcat(msg.message_text.buffer, buf);

        int client_qid = msg.message_text.qid;
        msg.message_text.qid = qid;

        if (msgsnd(client_qid, &msg, sizeof(struct message_text), 0) == -1) {
            perror("msgsnd");
            printf("Server: Error sending message to client queue %d, continuing...\n", client_qid);
            exit(EXIT_FAILURE);
            //continue;
        }
    }
}