#include "queues/core.h"
#include "queues/client.h"
#include <pthread.h>

struct client_state client_global;

int main() {
    char input[512];
    char command[50];
    char argument[MAX_MESSAGE_SIZE];
    
    printf("=== Cliente de Chat ===\n");
    printf("Ingresa tu nombre: ");
    fflush(stdout);
    
    if (fgets(client_global.name, MAX_NAME_LENGTH, stdin) == NULL) {
        printf("Error leyendo el nombre\n");
        return 1;
    }
    
    // Remover salto de línea
    client_global.name[strcspn(client_global.name, "\n")] = '\0';
    
    if (strlen(client_global.name) == 0) {
        printf("El nombre no puede estar vacío\n");
        return 1;
    }
    
    // Conectar al servidor
    if (connect_to_server(&client_global) != 0) {
        printf("Error conectando al servidor\n");
        return 1;
    }
    
    printf("\n¡Conectado al servidor exitosamente!\n");
    printf("Escribe 'help' para ver los comandos disponibles\n\n");
    
    // Configurar manejo de señales
    signal(SIGINT, signal_handler_client);
    signal(SIGTERM, signal_handler_client);
    
    // Crear hilo para escuchar mensajes del servidor
    pthread_t listener_thread;
    if (pthread_create(&listener_thread, NULL, message_listener, &client_global) != 0) {
        printf("Error creando hilo de escucha\n");
        cleanup_client(&client_global);
        return 1;
    }
    
    // Bucle principal de comandos
    while (client_global.running) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remover salto de línea
        input[strcspn(input, "\n")] = '\0';
        
        // Parsear comando y argumento
        int parsed = sscanf(input, "%49s %255[^\n]", command, argument);
        if (parsed < 1) {
            continue;
        }
        
        // Procesar comandos
        if (strcmp(command, "help") == 0) {
            show_help();
        }
        else if (strcmp(command, "create") == 0) {
            if (parsed < 2) {
                printf("Uso: create <nombre_canal>\n");
                continue;
            }
            send_create_channel_request(&client_global, argument);
        }
        else if (strcmp(command, "join") == 0) {
            if (parsed < 2) {
                printf("Uso: join <nombre_canal>\n");
                continue;
            }
            send_join_request(&client_global, argument);
        }
        else if (strcmp(command, "leave") == 0) {
            send_leave_request(&client_global);
        }
        else if (strcmp(command, "msg") == 0) {
            if (parsed < 2) {
                printf("Uso: msg <mensaje>\n");
                continue;
            }
            if (!client_global.in_channel) {
                printf("Debes estar en un canal para enviar mensajes\n");
                continue;
            }
            send_message(&client_global, argument);
        }
        else if (strcmp(command, "list") == 0) {
            send_list_channels_request(&client_global);
        }
        else if (strcmp(command, "clear") == 0) {
            clear_screen();
        }
        else if (strcmp(command, "status") == 0) {
            if (client_global.in_channel) {
                printf("Conectado al canal: %s\n", client_global.current_channel);
            } else {
                printf("No estás en ningún canal\n");
            }
        }
        else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        }
        else {
            printf("Comando desconocido: %s. Escribe 'help' para ver los comandos disponibles\n", command);
        }
    }
    
    // Limpiar y salir
    client_global.running = 0;
    pthread_cancel(listener_thread);
    pthread_join(listener_thread, NULL);
    cleanup_client(&client_global);
    
    printf("¡Hasta luego!\n");
    return 0;
}