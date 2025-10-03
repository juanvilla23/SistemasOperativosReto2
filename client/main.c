#include "queues/core.h"
#include "queues/client.h"
#include <pthread.h>

// Variable global que mantiene el estado del cliente durante toda la sesión
struct client_state client_global;

int main() {
    char input[512];                    // Buffer para la entrada del usuario
    char command[50];                   // Buffer para el comando ingresado
    char argument[MAX_MESSAGE_SIZE];    // Buffer para los argumentos del comando
    
    // Mensaje de bienvenida y solicitud del nombre del usuario
    printf("=== Cliente de Chat ===\n");
    printf("Ingresa tu nombre: ");
    fflush(stdout);
    
    // Leer el nombre del usuario desde la entrada estándar
    if (fgets(client_global.name, MAX_NAME_LENGTH, stdin) == NULL) {
        printf("Error leyendo el nombre\n");
        return 1;
    }
    
    // Eliminar el carácter de nueva línea del final del nombre
    client_global.name[strcspn(client_global.name, "\n")] = '\0';
    
    // Validar que el nombre no esté vacío
    if (strlen(client_global.name) == 0) {
        printf("El nombre no puede estar vacío\n");
        return 1;
    }
    
    // Intentar conectar al servidor
    if (connect_to_server(&client_global) != 0) {
        printf("Error conectando al servidor\n");
        return 1;
    }
    
    // Mensaje de confirmación de conexión exitosa
    printf("\n¡Conectado al servidor exitosamente!\n");
    printf("Escribe 'help' para ver los comandos disponibles\n\n");
    
    // Configurar manejadores de señales para limpieza adecuada al salir
    signal(SIGINT, signal_handler_client);   // Ctrl+C
    signal(SIGTERM, signal_handler_client);  // Terminación del proceso
    
    // Crear hilo para escuchar mensajes del servidor de forma asíncrona
    pthread_t listener_thread;
    if (pthread_create(&listener_thread, NULL, message_listener, &client_global) != 0) {
        printf("Error creando hilo de escucha\n");
        cleanup_client(&client_global);
        return 1;
    }
    
    // Bucle principal del cliente - procesa comandos del usuario
    while (client_global.running) {
        printf("> ");                    // Prompt para el usuario
        fflush(stdout);
        
        // Leer entrada del usuario
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // Salir si hay error en la entrada (EOF)
        }
        
        // Eliminar el carácter de nueva línea del final de la entrada
        input[strcspn(input, "\n")] = '\0';
        
        // Parsear el comando y sus argumentos
        int parsed = sscanf(input, "%49s %255[^\n]", command, argument);
        if (parsed < 1) {
            continue;  // Ignorar líneas vacías
        }
        
        // Procesar comandos del usuario
        if (strcmp(command, "help") == 0) {
            show_help();  // Mostrar ayuda
        }
        else if (strcmp(command, "create") == 0) {
            // Crear un nuevo canal
            if (parsed < 2) {
                printf("Uso: create <nombre_canal>\n");
                continue;
            }
            send_create_channel_request(&client_global, argument);
        }
        else if (strcmp(command, "join") == 0) {
            // Unirse a un canal existente
            if (parsed < 2) {
                printf("Uso: join <nombre_canal>\n");
                continue;
            }
            send_join_request(&client_global, argument);
        }
        else if (strcmp(command, "leave") == 0) {
            // Salir del canal actual
            send_leave_request(&client_global);
        }
        else if (strcmp(command, "msg") == 0) {
            // Enviar mensaje al canal actual
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
            // Listar canales disponibles
            send_list_channels_request(&client_global);
        }
        else if (strcmp(command, "clear") == 0) {
            // Limpiar pantalla
            clear_screen();
        }
        else if (strcmp(command, "status") == 0) {
            // Mostrar estado actual del cliente
            if (client_global.in_channel) {
                printf("Conectado al canal: %s\n", client_global.current_channel);
            } else {
                printf("No estás en ningún canal\n");
            }
        }
        else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            // Salir del programa
            break;
        }
        else {
            // Comando no reconocido
            printf("Comando desconocido: %s. Escribe 'help' para ver los comandos disponibles\n", command);
        }
    }
    
    // Limpieza al salir del programa
    client_global.running = 0;              // Marcar cliente como no activo
    pthread_cancel(listener_thread);       // Cancelar hilo de escucha
    pthread_join(listener_thread, NULL);    // Esperar a que termine el hilo
    cleanup_client(&client_global);         // Limpiar recursos del cliente
    
    printf("¡Hasta luego!\n");
    return 0;
}