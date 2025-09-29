#ifndef QUEUE_CORE_H
#define QUEUE_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SERVER_KEY_PATHNAME "/tmp"
#define PROJECT_ID 33
#define QUEUE_PERMISSIONS 0660

#define MAX_MESSAGE_SIZE 256
#define MAX_CHANNELS 10
#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 50

#define MSG_JOIN_REQUEST    1
#define MSG_LEAVE_REQUEST   2
#define MSG_SEND_MESSAGE    3
#define MSG_SERVER_RESPONSE 4
#define MSG_BROADCAST       5
#define MSG_LIST_CHANNELS   6
#define MSG_CREATE_CHANNEL  7
#define MSG_DELETE_CHANNEL  8

#define RESPONSE_SUCCESS    0
#define RESPONSE_ERROR      -1
#define RESPONSE_CHANNEL_FULL -2
#define RESPONSE_CHANNEL_NOT_FOUND -3
#define RESPONSE_ALREADY_IN_CHANNEL -4

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
    int current_channel_index;
};

struct channel {
    int qid;
    pid_t clients[MAX_CLIENTS];
    char name[MAX_NAME_LENGTH];
    int client_count;
};

struct server {
    int qid;
    struct channel channels[MAX_CHANNELS];
    struct client clients[MAX_CLIENTS];
    int channel_count;
    int client_count;
    int running;
};

#endif