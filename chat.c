//
// Created by artur on 01/12/2018.
//

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket_helper.h"
#include "chat.h"

Chat chat_setup(unsigned int chat_size, char *name)
{
    Chat chat;
    chat.username = (char *) calloc(strlen(name) + 1, sizeof(char));
    strcpy(chat.username, name);
    chat.connections = (int *) calloc(chat_size, sizeof(int));
    chat.participants = (char **) calloc(chat_size, sizeof(char *));

    chat.user_count = 0;
    chat.chat_size = chat_size;

    return chat;
}

int chat_add_user(Chat chat, char *name, char *ip_address, int port)
{
    struct sockaddr_in user_address;
    if (chat.chat_size <= chat.user_count) {
        return -1;
    }
    user_address = ClientSockaddrIn(AF_INET, ip_address, port);

    chat.participants[chat.user_count] = (char *) calloc(strlen(name) + 1, sizeof(char));
    strcpy(chat.participants[chat.user_count], name);
    chat.connections[chat.user_count] = Socket(AF_INET, SOCK_DGRAM, 0);
    Connect(chat.connections[chat.user_count], (struct sockaddr *) &user_address, sizeof(user_address));

    chat.user_count++;
    return 0;
}

void chat_close(Chat chat) {
    int i;
    struct sockaddr_in disconnect_address;
    disconnect_address = ClientSockaddrIn(AF_UNSPEC, NULL, 0);

    for (i = 0; i < chat.user_count; i++) {
        free(chat.participants[i]);
        Connect(chat.connections[i], (struct sockaddr *) &disconnect_address, sizeof(disconnect_address));
    }

    free(chat.participants);
    free(chat.connections);
    free(chat.username);

    chat.chat_size = 0;
}

void chat_send_message(Chat chat, char *receiver_name, char *message) {
    int i;

    for (i = 0; i < chat.user_count; i++) {
        if (strcmp(chat.participants[i], receiver_name) == 0) {
            send(chat.connections[i], message, strlen(message), MSG_DONTWAIT);
        }
    }
}

int chat_receive_messages(Chat chat, char *buffer, unsigned int count)
{
    int i, max_fd = 0;
    fd_set read_udp_set;

    FD_ZERO(&read_udp_set);
    for (i = 0; i < chat.user_count; i++) {
        FD_SET(chat.connections[i], &read_udp_set);
        if (max_fd <= chat.connections[i]) {
            max_fd = chat.connections[i] + 1;
        }
    }

    if (select(max_fd, &read_udp_set, NULL, NULL, NULL)) {
        for (i = 0; i < chat.user_count; i++) {
            if (FD_ISSET(chat.connections[i], &read_udp_set)) {
                return read(chat.connections[i], buffer, count);
            }
        }
    }

    return 0;
}