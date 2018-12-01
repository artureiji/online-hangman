//
// Created by artur on 01/12/2018.
//

#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
    struct sockaddr_in local_address;
    socklen_t len = sizeof(local_address);

    chat.username = (char *) calloc(strlen(name) + 1, sizeof(char));
    strcpy(chat.username, name);
    chat.connections = (int *) calloc(chat_size, sizeof(int));
    chat.participants = (char **) calloc(chat_size, sizeof(char *));

    chat.user_count = 0;
    chat.chat_size = chat_size;

    chat.read_fd = Socket(AF_INET, SOCK_DGRAM, 0);
    local_address = ServerSockaddrIn(AF_INET, INADDR_ANY, 0);
    Bind(chat.read_fd, (struct sockaddr *) &local_address, sizeof(local_address));

    bzero(&local_address, sizeof(local_address));
    getsockname(chat.read_fd, (struct sockaddr *) &local_address, &len);
    chat.read_port = ntohs(local_address.sin_port);

    return chat;
}

int chat_add_user(Chat chat, char *name, char *ip_address, int port)
{
    struct sockaddr_in user_address;
    if (chat.chat_size <= chat.user_count) {
        return -1;
    }
    user_address = ClientSockaddrIn(AF_INET, ip_address, htons(port));

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
    fd_set read_udp_set;

    FD_ZERO(&read_udp_set);
    FD_SET(chat.read_fd, &read_udp_set);
    if (select(chat.read_fd, &read_udp_set, NULL, NULL, NULL)) {
        if (FD_ISSET(chat.read_fd, &read_udp_set)) {
            return read(chat.read_fd, buffer, count);
        }
    }

    return 0;
}