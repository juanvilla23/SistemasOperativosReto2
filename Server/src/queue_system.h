#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SERVER_KEY_PATHNAME "/tmp"
#define PROJECT_ID 33
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGE_SIZE 200

struct message_text {
    int qid;
    char buffer [MAX_MESSAGE_SIZE];
};

struct message {
    long message_type;
    struct message_text message_text;
};