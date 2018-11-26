#include "socket_helper.h"
#include <string.h>

void printIntro() {
  printf("Bem vindo ao jogo da forca!\n");
  printf("-----\n\n");
  printf("1) Iniciar partida simples\n");
  printf("2) Ser carrasco ao iniciar partida\n");
  printf("3) jogar no modo multiplayer\n");
}

int main(int argc, char **argv) {
    int connfd;
    char error[300], choice;
    struct sockaddr_in servaddr;

    if (argc != 2) {
      strcpy(error, "uso: ");
      strcat(error, argv[0]);
      strcat(error, " <IPaddress>");
      perror(error);
      exit(1);
   }

    connfd = Socket(AF_INET, SOCK_STREAM, 0);

    servaddr = ClientSockaddrIn(AF_INET, argv[1], 10201);

    Connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    printIntro();

    printf("Digite sua escolha:");
    choice = getchar();

    return 0;
}
