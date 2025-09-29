#ifndef QUEUE_CLIENT_H
#define QUEUE_CLIENT_H

#include "core.h"

struct client_state {
    int server_qid;
    int client_qid;
    pid_t pid;
    char name[MAX_NAME_LENGTH];
    char current_channel[MAX_NAME_LENGTH];
    int running;
    int in_channel;
};

int connect_to_server(struct client_state *client);
void disconnect_from_server(struct client_state *client);
int send_join_request(struct client_state *client, const char *channel_name);
int send_leave_request(struct client_state *client);
int send_message(struct client_state *client, const char *message);
int send_list_channels_request(struct client_state *client);
int send_create_channel_request(struct client_state *client, const char *channel_name);

void show_menu();
void show_help();
void clear_screen();
int get_user_input(char *buffer, int max_size);

void* message_listener(void *arg);
void handle_server_response(struct message *msg);
void handle_broadcast_message(struct message *msg);

void cleanup_client(struct client_state *client);
void signal_handler_client(int signum);

#endif
