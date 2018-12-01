#include "socket_helper.h"
#include "time.h"
#include <fcntl.h>
#include <errno.h>

#define MAXDATASIZE 255
#define MAXWORDSIZE 255
#define MAXCLIENTS 255

int fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}


void run_game(){while(1);}

int main(int argc, char **argv) {
    /* Variavies relativas a conexao com o cliente*/
    struct sockaddr_in servaddr, cliinfo;
    struct timeval timeout;
    socklen_t cliinfo_len;
    int listenfd, connfd, pid=0, option=1, port=9000;
    int total_clients = 0, cid, rv, maxpipe;
    int in_pipes[MAXCLIENTS][2], out_pipes[MAXCLIENTS][2];
    int connections[MAXCLIENTS];
    char buffer[MAXDATASIZE];
    char client_info[MAXCLIENTS][255], batch_info[MAXCLIENTS * 255];

    fd_set listen_set, pipes_set;

    for(int i = 0; i < MAXCLIENTS; i++) connections[i] = 0;

    /* Queremos fazer um pooling continuo*/
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    /* Variaveis de controle do loby de jogo */
    int executioner_cid=-1, lobby[MAXCLIENTS], lobby_size = 0, menu = 1;
    int is_executioner=0, n_players=0;
    time_t timer;

    /* Socket do servidor */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, 10);

    timer = clock();

    while(1){
      /* Zeramos o set e marcamos o socket nele */
      FD_ZERO(&listen_set);
      FD_SET(listenfd, &listen_set);

      /* So chamamos accept se houver uma solicitacao de conexao */
      rv = select(listenfd + 1, &listen_set, NULL, NULL, &timeout);

      if (rv > 0){
        cliinfo_len = sizeof(cliinfo);
        connfd = Accept(listenfd, (struct sockaddr *) &cliinfo, &cliinfo_len);

        /* Guardando as informacoes de conexao com o cliente */
        sprintf(client_info[total_clients], "%s", inet_ntoa(cliinfo.sin_addr));

        /* Pipe para enviar mensagem para o child*/
        pipe(in_pipes[total_clients]);
        /* Pipe para receber mensagem do child*/
        pipe(out_pipes[total_clients]);

        /* Conexao marcada como ativa*/
        connections[total_clients] = 1;

        pid = fork();

        if(pid == 0){
          cid = total_clients;
          close(listenfd);
          listenfd = -1;

          /* Fecha a via de escrita de in_pipe*/
          close(in_pipes[cid][1]);
          /* Fecha a via de leitura de out_pipe*/
          close(out_pipes[cid][0]);

          /* Mantemos o cliente no menu */
          while(1){
            /* Espera ate que o cliente retorne a sua escolha de modo de jogo*/
            read(connfd, buffer, sizeof(buffer));
            printf("Decisao do parent: %s\n", buffer);
            if(buffer[0] == '2'){
              /* Repassamos a escolha para o parent process */
              write(out_pipes[cid][1], buffer, sizeof(buffer));

              /* Vemos qual a decisao do parent */
              read(in_pipes[cid][0], buffer, sizeof(buffer));

              /* O cliente nao se tornou carrasco */
              if(buffer[0] == 'x'){
                printf("N se tornou carrasco\n");
                write(connfd, "0\n", sizeof("0\n"));

                /* O cliente se tornou carrasco */
              }else{
                printf("Se tornou carrasco\n");

                write(connfd, "1\n", sizeof("1\n"));

                is_executioner = 1;
              }
            }else if (buffer[0] == '3'){
              /* Repassamos a escolha para o parent process*/
              write(out_pipes[cid][1], buffer, sizeof(buffer));

              /* Recebemos qualquer coisa so para sabermos que o client foi
              registrado*/
              read(in_pipes[cid][0], buffer, sizeof(buffer));

              bzero(buffer, sizeof(buffer));
              /* Esperamos pelo batch de info de clientes */
              read(in_pipes[cid][0], buffer, sizeof(buffer));

              /* Enviamos o batch */
              write(connfd, buffer, sizeof(buffer));

              /* No caso deste cliente ser o carrasco, esperamos as informacoes
                 dos outros players */
              if(is_executioner){
                  read(in_pipes[cid][0], buffer, sizeof(buffer));
                  write(connfd, buffer, sizeof(buffer));
              }

            }

          }
          close(connfd);
          exit(0);

        }else{
          total_clients++;

          /* Fecha a via de leitura de in_pipe*/
          close(in_pipes[total_clients][0]);
          /* Fecha a via de escrita de out_pipe*/
          close(out_pipes[total_clients][1]);

          close(connfd);
          connfd = -1;
        }
      }

      /* Neste trecho do codigo lidamos com a comunicacao entre parent e childs*/

      /* Zeramos o set e marcamos os respectivos pipes nele */
      FD_ZERO(&pipes_set); maxpipe = 0;

      /* Queremos fazer polling nos pipes de clientes ativos */
      for(int i = 0; i < MAXCLIENTS; i++)
        if(connections[i]){
          FD_SET(out_pipes[i][0], &pipes_set);
          if (out_pipes[i][0] > maxpipe){
            maxpipe = out_pipes[i][0];
          }
        }

      /*  */
      rv = select(maxpipe + 1, &pipes_set, NULL, NULL, &timeout);
      if(rv > 0){

        for(int i = 0; i < MAXCLIENTS; i++){

          /* Se o cliente i escreveu algo no pipe*/
          if(connections[i] && FD_ISSET(out_pipes[i][0], &pipes_set)){
            read(out_pipes[i][0], buffer, sizeof(buffer));

            /* O cliente i quer ser carrasco */
            if(buffer[0] == '2'){
              /* Nao existe carrasco ainda, logo o cliente i pode assumir a vaga*/
              if(executioner_cid == -1){
                executioner_cid = i;
                write(in_pipes[i][1], "a", sizeof("a"));

              /* Ja existe um carrasco */
              }else{
                /* Comunica o child */
                write(in_pipes[i][1], "x", sizeof("x"));
              }

            /* O cliente quer jogar multiplayer */
          }else if(buffer[0] == '3'){
              /* O cliente eh inserido no lobby */
              lobby[i] = 1;
              lobby_size++;

              for(int i = 0; i < 254; i++) buffer[i] = buffer[i+1];

              char * line = strtok(strdup(buffer), "\n");
              while(line) {
                 strcat(client_info[i], buffer);
                 line  = strtok(NULL, "\n");
              }

              printf("cliinfo: %s\n", client_info);
              write(in_pipes[i][1], "x", sizeof("x"));

          /* O cliente quer jogar singleplayer */
          }else if(buffer[0] == '1'){
              /* Abandona o lobby */
              if(lobby[i]){
                lobby[i] = 0;
                lobby_size--;
              }

              /* Nao existe mais carrasco */
              if(executioner_cid == i)
                executioner_cid = -1;
            }
          }
        }

      }

      /* A cada 45 segundos (ou 9  players) fechamos uma partida */
      if((clock() - timer)/CLOCKS_PER_SEC > 10 || lobby_size == 9){
        /* Notificamos os childs que estao no lobby */
        if(lobby_size > 0)
          for (int i = 0; i < MAXCLIENTS; i++)

            /* Para cada cliente que nao seja o carrasco */
            if(lobby[i] && i != executioner_cid){
              /* Passamos para eles o numero de jogadores, ip, nome e porta UDP */
              sprintf(buffer, "%d", lobby_size);
              strcat(buffer, " ");
              printf("Aqui!\n");

              for(int j = 0; j < 255; j++)
                if(lobby[j])
                  strcat(buffer, client_info[j]);

              printf("Batch: %s\n", buffer);
              write(in_pipes[i][1], buffer, sizeof(buffer));


            /* No caso do carrasco */
            }else if(i == executioner_cid){
              /* Passamos para ele as informacoes de conexao com clientes */
              sprintf(buffer, "%d\0", lobby_size);
              write(in_pipes[i][1], buffer, sizeof(buffer));
            }

        timer = clock();
      }
    }

    return 0;
}
