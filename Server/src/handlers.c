#include "queues/core.h"
#include "queues/server.h"

void handle_join_request(struct server *srv, struct message *msg) {
    struct channel *current_channel = find_client_channel(srv, msg->data.sender_pid);
    if (current_channel != NULL) {
        if (strcmp(current_channel->name, msg->data.channel_name) == 0) {
            send_response(msg->data.client_qid, RESPONSE_ALREADY_IN_CHANNEL, "Ya estás en este canal");
            return;
        }
        send_response(msg->data.client_qid, RESPONSE_ERROR, "Ya estás en otro canal. Debes salir primero");
        return;
    } 
    
    struct channel *ch = find_channel_by_name(srv, msg->data.channel_name);
    if (ch == NULL) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_NOT_FOUND, "Canal no encontrado");
        return;
    }

    if (ch->client_count >= MAX_CLIENTS) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_FULL, "El canal está lleno");
        return;
    }


    int free_slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ch->clients[i] == 0) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == -1) {
        send_response(msg->data.client_qid, RESPONSE_CHANNEL_FULL, "El canal está lleno");
        return;
    }
    
    ch->clients[free_slot] = msg->data.sender_pid;
    ch->client_count++;
    
    int client_registered = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid == msg->data.sender_pid) {
            srv->clients[i].current_channel_index = ch - srv->channels;
            client_registered = 1;
            break;
        } else if (srv->clients[i].pid == 0) {
            srv->clients[i].pid = msg->data.sender_pid;
            srv->clients[i].qid = msg->data.client_qid;
            strncpy(srv->clients[i].name, msg->data.sender_name, MAX_NAME_LENGTH - 1);
            srv->clients[i].name[MAX_NAME_LENGTH - 1] = '\0';
            srv->clients[i].current_channel_index = ch - srv->channels;
            srv->client_count++;
            client_registered = 1;
            break;
        }
    }
    
    if (!client_registered) {
        ch->clients[free_slot] = 0;
        ch->client_count--;
        send_response(msg->data.client_qid, RESPONSE_ERROR, "Servidor lleno, no se pueden registrar más clientes");
        return;
    }
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, "Te has unido al canal exitosamente");
    
    struct message broadcast_msg;
    broadcast_msg.message_type = MSG_BROADCAST;
    broadcast_msg.data.client_qid = 0;
    broadcast_msg.data.sender_pid = 0;
    strcpy(broadcast_msg.data.sender_name, "Sistema");
    strcpy(broadcast_msg.data.channel_name, ch->name);
    snprintf(broadcast_msg.data.content, MAX_MESSAGE_SIZE, "%s se ha unido al canal", msg->data.sender_name);
    broadcast_msg.data.response_code = RESPONSE_SUCCESS;
    
    broadcast_message(srv, ch, &broadcast_msg);
}

void handle_leave_request(struct server *srv, struct message *msg) {
    // 1. Verificar si el cliente está en algún canal
    struct channel *current_channel = find_client_channel(srv, msg->data.sender_pid);
    if (current_channel == NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "No estás en ningún canal");
        return;
    }
    
    // 2. Obtener el nombre del cliente para el mensaje de notificación
    char client_name[MAX_NAME_LENGTH] = "Usuario desconocido";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid == msg->data.sender_pid) {
            strncpy(client_name, srv->clients[i].name, MAX_NAME_LENGTH - 1);
            client_name[MAX_NAME_LENGTH - 1] = '\0';
            break;
        }
    }
    
    // 3. Remover el cliente del canal
    remove_client_from_channel(current_channel, msg->data.sender_pid);
    
    // 4. Actualizar la información del cliente en el servidor
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid == msg->data.sender_pid) {
            srv->clients[i].current_channel_index = -1; // -1 indica que no está en ningún canal
            break;
        }
    }
    
    // 5. Enviar respuesta exitosa al cliente
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, "Has salido del canal exitosamente");
    
    // 6. Notificar a otros clientes del canal sobre la salida (solo si quedan clientes)
    if (current_channel->client_count > 0) {
        struct message broadcast_msg;
        broadcast_msg.message_type = MSG_BROADCAST;
        broadcast_msg.data.client_qid = 0;
        broadcast_msg.data.sender_pid = 0; // Sistema
        strcpy(broadcast_msg.data.sender_name, "Sistema");
        strcpy(broadcast_msg.data.channel_name, current_channel->name);
        snprintf(broadcast_msg.data.content, MAX_MESSAGE_SIZE, "%s ha salido del canal", client_name);
        broadcast_msg.data.response_code = RESPONSE_SUCCESS;
        
        broadcast_message(srv, current_channel, &broadcast_msg);
    }
}

void handle_send_message(struct server *srv, struct message *msg) {
    // 1. Verificar que el cliente esté en algún canal
    struct channel *current_channel = find_client_channel(srv, msg->data.sender_pid);
    if (current_channel == NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "No estás en ningún canal. Únete a un canal primero");
        return;
    }
    
    // 2. Verificar que el mensaje no esté vacío
    if (strlen(msg->data.content) == 0) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "El mensaje no puede estar vacío");
        return;
    }
    
    // 3. Obtener el nombre del cliente remitente
    char sender_name[MAX_NAME_LENGTH] = "Usuario desconocido";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].pid == msg->data.sender_pid) {
            strncpy(sender_name, srv->clients[i].name, MAX_NAME_LENGTH - 1);
            sender_name[MAX_NAME_LENGTH - 1] = '\0';
            break;
        }
    }
    
    // 4. Crear el mensaje de broadcast para enviar a todos los clientes del canal
    struct message broadcast_msg;
    broadcast_msg.message_type = MSG_BROADCAST;
    broadcast_msg.data.client_qid = 0; // No necesario para broadcast
    broadcast_msg.data.sender_pid = msg->data.sender_pid;
    strncpy(broadcast_msg.data.sender_name, sender_name, MAX_NAME_LENGTH - 1);
    broadcast_msg.data.sender_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(broadcast_msg.data.channel_name, current_channel->name, MAX_NAME_LENGTH - 1);
    broadcast_msg.data.channel_name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(broadcast_msg.data.content, msg->data.content, MAX_MESSAGE_SIZE - 1);
    broadcast_msg.data.content[MAX_MESSAGE_SIZE - 1] = '\0';
    broadcast_msg.data.response_code = RESPONSE_SUCCESS;
    
    // 5. Enviar el mensaje a todos los clientes del canal
    broadcast_message(srv, current_channel, &broadcast_msg);
    
    // 6. Confirmar al remitente que el mensaje fue enviado
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, "Mensaje enviado correctamente");
}

void handle_list_channels(struct server *srv, struct message *msg) {
    char channels_list[MAX_MESSAGE_SIZE];
    int offset = 0;
    
    if (srv->channel_count == 0) {
        send_response(msg->data.client_qid, RESPONSE_SUCCESS, "No hay canales disponibles");
        return;
    }
    
    offset += snprintf(channels_list + offset, MAX_MESSAGE_SIZE - offset, "Canales disponibles:\n");
    
    for (int i = 0; i < srv->channel_count && offset < MAX_MESSAGE_SIZE - 50; i++) {
        struct channel *ch = &srv->channels[i];
        
        if (strlen(ch->name) > 0) {
            int remaining_space = MAX_MESSAGE_SIZE - offset - 1;
            int written = snprintf(channels_list + offset, remaining_space, 
                                 "- %s (%d/%d usuarios)\n", 
                                 ch->name, ch->client_count, MAX_CLIENTS);
            
            if (written > 0 && written < remaining_space) {
                offset += written;
            } else {
                snprintf(channels_list + offset, remaining_space, "...(más canales)");
                break;
            }
        }
    }
    
    channels_list[MAX_MESSAGE_SIZE - 1] = '\0';
    
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, channels_list);
}

void handle_create_channel(struct server *srv, struct message *msg) {
    // 1. Verificar que el nombre del canal no esté vacío
    if (strlen(msg->data.channel_name) == 0) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "El nombre del canal no puede estar vacío");
        return;
    }
    
    // 2. Verificar que el canal no exista ya
    struct channel *existing_channel = find_channel_by_name(srv, msg->data.channel_name);
    if (existing_channel != NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "El canal ya existe");
        return;
    }
    
    // 3. Verificar que no se haya alcanzado el límite de canales
    if (srv->channel_count >= MAX_CHANNELS) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "Se ha alcanzado el límite máximo de canales");
        return;
    }
    
    // 4. Crear el nuevo canal
    struct channel *new_channel = create_channel(srv, msg);
    if (new_channel == NULL) {
        send_response(msg->data.client_qid, RESPONSE_ERROR, "Error interno creando el canal");
        return;
    }
    
    // 5. Enviar confirmación de éxito
    char success_msg[MAX_MESSAGE_SIZE];
    snprintf(success_msg, MAX_MESSAGE_SIZE, "Canal '%s' creado exitosamente", msg->data.channel_name);
    send_response(msg->data.client_qid, RESPONSE_SUCCESS, success_msg);
    
    // 6. Log del servidor
    printf("Canal '%s' creado por %s (PID: %d)\n", 
           msg->data.channel_name, msg->data.sender_name, msg->data.sender_pid);
}