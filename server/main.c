#include "queues/core.h"
#include "queues/server.h"

// Variable global que mantiene el estado del servidor
struct server srv;

int main() {
    struct message msg;
    
    // Inicializar el servidor y sus estructuras
    init_server(&srv);
 
    // Configurar manejadores de señales para limpieza adecuada al salir
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // Terminación del proceso
 
    // Bucle principal del servidor - procesa mensajes de los clientes
    while (srv.running) {
        // Recibir mensaje de la cola del servidor
        if (msgrcv(srv.qid, &msg, sizeof(msg.data), 0, 0) == -1) {
            // Si la interrupción es por señal, continuar
            if (errno == EINTR) {
                continue;
            }
            perror("msgrcv");
            continue;
        }
 
        // Procesar el mensaje según su tipo
        switch (msg.message_type) {
            case MSG_JOIN_REQUEST:
                handle_join_request(&srv, &msg);      // Cliente solicita unirse a un canal
                break;
            case MSG_LEAVE_REQUEST:
                handle_leave_request(&srv, &msg);      // Cliente solicita salir de un canal
                break;
            case MSG_SEND_MESSAGE:
                handle_send_message(&srv, &msg);       // Cliente envía mensaje a un canal
                break;
            case MSG_LIST_CHANNELS:
                handle_list_channels(&srv, &msg);     // Cliente solicita lista de canales
                break;
            case MSG_CREATE_CHANNEL:
                handle_create_channel(&srv, &msg);     // Cliente solicita crear un canal
                break;
            // case MSG_DELETE_CHANNEL:
            //     handle_delete_channel(&srv, &msg);   // Funcionalidad no implementada
            //     break;
            default:
                // Tipo de mensaje no reconocido
                fprintf(stderr, "Tipo de mensaje desconocido: %ld\n", msg.message_type);
                break;
        }
    }
   
    // Limpiar recursos del servidor al salir
    cleanup_server(&srv);
    return 0;
}