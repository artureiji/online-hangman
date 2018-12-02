//
// Created by artur on 01/12/2018.
//

#ifndef ONLINE_HANGMAN_CHAT_H
#define ONLINE_HANGMAN_CHAT_H

    typedef struct Chat {
        char *username;
        char **participants;
        int *connections;
        int read_fd;
        int read_port;
        int user_count;
        int chat_size;
    } Chat;

    Chat chat_setup(unsigned int chat_size, char *name);

    int chat_add_user(Chat chat, char *name, char *ip_address, int port);

    void chat_close(Chat chat);

    void chat_send_message(Chat chat, char *receiver_name, char *message);

    int chat_receive_messages(Chat chat, char *buffer, unsigned int count);
#endif //ONLINE_HANGMAN_CHAT_H
