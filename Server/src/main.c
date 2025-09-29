#include "queues/core.h"
#include "queues/server.h"

struct server srv;

int main() {
    struct message msg;
    init_server(&srv);
 
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
 
    while (srv.running) {
        if (msgrcv(srv.qid, &msg, sizeof(msg.data), 0, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("msgrcv");
            continue;
        }
 
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
                handle_list_channels(&srv, &msg);
                break;
            case MSG_CREATE_CHANNEL:
                handle_create_channel(&srv, &msg);
                break;
            // case MSG_DELETE_CHANNEL:
            //     handle_delete_channel(&srv, &msg);
            //     break;
            default:
                fprintf(stderr, "Tipo de mensaje desconocido: %ld\n", msg.message_type);
                break;
        }
    }
   
    cleanup_server(&srv);
    return 0;
}