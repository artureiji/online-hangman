#include "socket_helper.h"
#include <string.h>

void printIntro() {
  printf("Bem vindo ao jogo da forca!\n");
  printf("-----\n\n");
  printf("1) Iniciar partida simples\n");
  printf("2) Ser carrasco ao iniciar partida\n");
  printf("3) jogar no modo multiplayer\n");
  printf("4) sair.\n");
}

void play_multiplayer() {

}

int main(int argc, char **argv) {
    int connfd, option;
    char error[300], command[100];
    struct sockaddr_in servaddr;

    if (argc != 2) {
      strcpy(error, "uso: ");
      strcat(error, argv[0]);
      strcat(error, " <IPaddress>");
      perror(error);
      exit(1);
   }

    connfd = Socket(AF_INET, SOCK_STREAM, 0);

    servaddr = ClientSockaddrIn(AF_INET, argv[1], 9100);

    Connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        
    option = '0';
    while (option != '4') {
        printIntro();
        printf("Digite sua escolha:");
        option = getchar();

        switch (option) {
            case '3':
                send(connfd, "Hello world!", 12, NULL);
                play_multiplayer();
                break;
        }
    }

    return 0;
}
