
#ifndef QUEUE_SERVER_H
#define QUEUE_SERVER_H

#include "core.h"

void init_server(struct server *srv);
void cleanup_server(struct server *srv);
void signal_handler(int signum);

struct channel* create_channel(struct server *srv, const struct message *msg);
struct channel* find_channel(struct server *srv, const char *channel_name);
int delete_channel(struct server *srv, const char *channel_name);

int add_client_to_channel(struct channel *ch, pid_t client_pid);
int remove_client_from_channel(struct channel *ch, pid_t client_pid);
int is_client_in_channel(struct channel *ch, pid_t client_pid);

void handle_join_request(struct server *srv, struct message *msg);
void handle_leave_request(struct server *srv, struct message *msg);
void handle_send_message(struct server *srv, struct message *msg);
void handle_list_channels(struct server *srv, struct message *msg);
void handle_create_channel(struct server *srv, struct message *msg);
void handle_delete_channel(struct server *srv, struct message *msg);

void send_response(int client_qid, int response_code, const char *message);
void broadcast_message(struct channel *ch, struct message *msg);

#endif