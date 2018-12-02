#include "chat.h"
#include "socket_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_PLAYERS 10

void printIntro() {
  printf("Bem vindo ao jogo da forca!\n");
  printf("-----\n\n");
  printf("1) Iniciar partida simples\n");
  printf("2) Ser carrasco ao iniciar partida\n");
  printf("3) jogar no modo multiplayer\n");
  printf("4) sair.\n");
}

void ask_executor_privileges(int connfd)
{
    char command[100], buffer[100], line[255];
    bzero(command, sizeof(command));
    snprintf(command, 100, "2");
    send(connfd, command, strlen(command), 0);

    bzero(line, sizeof(line));
    recv(connfd, buffer, sizeof(buffer), 0);
    strcat(line, buffer);
    while (recv(connfd, buffer, sizeof(buffer), MSG_DONTWAIT) >= 0) {
        strcat(line, buffer);
    }

    printf("Linha recebida: %s\n", line);
}

void play_singleplayer(int connfd)
{
    char buffer[100], line[255];
    int vidas = 1, letras, letras_restantes;

    // sends to server game mode identifier
    bzero(buffer, sizeof(buffer));
    snprintf(buffer, 2, "1");
    send(connfd, buffer, 1, 0);

    letras = 0;
    do {
        // first byte sent by server is word length
        read(connfd, buffer, 1);
        letras_restantes = buffer[0];
        if (letras == 0) letras = letras_restantes;

        // second byte is lives count
        read(connfd, buffer, 1);
        vidas = buffer[0];

        printf("A partida de jogo da forca começou!\nVocê possui %d vidas.\nA palavra possui %d letras.\n\n", vidas, letras);
        
        bzero(line, sizeof(line));
        // remainder of stream is message from server, including word with guesses hits.
        recv(connfd, buffer, sizeof(buffer), 0);
        strcat(line, buffer);
        while (recv(connfd, buffer, sizeof(buffer), MSG_DONTWAIT) >= 0) {
            strcat(line, buffer);
        }
        printf("%s", line);

        if (letras_restantes <= 0 || vidas <= 0) {
            break;
        }

        printf("\nDigite seu palpite (letra ou palavra): ");
        scanf("%s", buffer);
        send(connfd, buffer, strlen(buffer), 0);
    } while (1);

    printf("\n\n");
}

void play_multiplayer(int connfd)
{
    Chat game_chat;
    char buffer[100], line[255];
    char * name, * ip, * port;
    int i, user_count;

    printf("Digite o seu nome:");
    scanf(" %s", buffer);
    game_chat = chat_setup(MAX_PLAYERS, buffer);

    // sends to server this client chat identificaion.
    bzero(buffer, sizeof(buffer));
    snprintf(buffer, 100, "3\n%s\n%d\n", game_chat.username, game_chat.read_port);
    send(connfd, buffer, strlen(buffer), 0);

    printf("Aguardando inicio da partida...");
    read(connfd, buffer, 1);
    buffer[1] = 0;
    user_count = atoi(buffer);

    bzero(line, sizeof(line));
    while (recv(connfd, buffer, sizeof(buffer), MSG_DONTWAIT) >= 0) {
        strcat(line, buffer);
    }
    
    ip = strtok(line, " ");
    for (i = 0; i < user_count; i++) {
        name = strtok(NULL, " ");
        port = strtok(NULL, " ");
        chat_add_user(game_chat, name, ip, atoi(port));
        ip = strtok(NULL, " ");
    }

    printf("Fim de jogo!\n");
}

int main(int argc, char **argv) {
    int connfd;
    char error[300], option;
    struct sockaddr_in servaddr;

    if (argc != 2) {
      strcpy(error, "uso: ");
      strcat(error, argv[0]);
      strcat(error, " <IPaddress>");
      perror(error);
      exit(1);
   }

    connfd = Socket(AF_INET, SOCK_STREAM, 0);

    servaddr = ClientSockaddrIn(AF_INET, argv[1], 9000);

    Connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    option = '0';
    while (option != '4') {
        printIntro();
        printf("Digite sua escolha:");
        scanf(" %c", &option);

        switch (option) {
            case '1':
                play_singleplayer(connfd);
                break;
            case '2':
                ask_executor_privileges(connfd);
                break;
            case '3':
                play_multiplayer(connfd);
                break;
        }
    }

    close(connfd);

    return 0;
}
