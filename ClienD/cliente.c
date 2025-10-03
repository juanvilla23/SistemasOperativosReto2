// Cliente adaptado para usar la API oficial del proyecto
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "queues/core.h"
#include "queues/client.h"

// Este símbolo es requerido por funciones como message_listener/handle_server_response
struct client_state client_global;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <nombre_usuario> <canal>\n", argv[0]);
        return 1;
    }

    const char *user_name = argv[1];
    const char *channel_name = argv[2];

    // 1) Inicializar estado con el nombre recibido
    strncpy(client_global.name, user_name, MAX_NAME_LENGTH - 1);
    client_global.name[MAX_NAME_LENGTH - 1] = '\0';

    // 2) Conectar al servidor existente (cola creada por server_app)
    if (connect_to_server(&client_global) != 0) {
        printf("Error conectando al servidor\n");
        return 1;
    }

    // 3) Registrar manejo de señales para cierre ordenado
    signal(SIGINT, signal_handler_client);
    signal(SIGTERM, signal_handler_client);

    // 4) Lanzar listener para recibir respuestas/broadcasts
    pthread_t listener_thread;
    if (pthread_create(&listener_thread, NULL, message_listener, &client_global) != 0) {
        printf("Error creando hilo de escucha\n");
        cleanup_client(&client_global);
        return 1;
    }

    // 5) Intentar crear el canal (si ya existe, el servidor responderá error)
    send_create_channel_request(&client_global, channel_name);

    // 6) Unirse al canal indicado
    if (send_join_request(&client_global, channel_name) != 0) {
        printf("No fue posible unirse al canal '%s'\n", channel_name);
        client_global.running = 0;
        pthread_cancel(listener_thread);
        pthread_join(listener_thread, NULL);
        cleanup_client(&client_global);
        return 1;
    }

    // 7) Bucle simple: leer mensajes de stdin y enviarlos. 'exit' para salir
    client_global.running = 1;
    printf("Conectado como '%s' en canal '%s'. Escribe mensajes o 'exit' para salir.\n",
           client_global.name, channel_name);

    char input[MAX_MESSAGE_SIZE];
    while (client_global.running) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        if (strlen(input) == 0) {
            continue;
        }
        send_message(&client_global, input);
    }

    // 8) Cierre ordenado
    client_global.running = 0;
    pthread_cancel(listener_thread);
    pthread_join(listener_thread, NULL);
    cleanup_client(&client_global);
    return 0;
}
