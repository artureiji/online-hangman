#include "socket_helper.h"

#define MAXDATASIZE 255
#define MAXWORDSIZE 255
#define MAXCLIENTS 255

int main(int argc, char **argv) {
    /* Variavies relativas a conexao com o cliente*/
    struct sockaddr_in servaddr, cliinfo;
    struct timeval timeout;
    socklen_t cliinfo_len;
    int listenfd, connfd, pid, option=1, port=9100;
    int total_clients = 0, cid, rv, maxpipe;
    int in_pipes[MAXCLIENTS][2], out_pipes[MAXCLIENTS][2];
    int connections[MAXCLIENTS];
    char buffer[MAXDATASIZE];
    fd_set listen_set, pipes_set;

    for(int i = 0; i < MAXCLIENTS; i++) connections[i] = 0;

    /* Queremos fazer um pooling continuo*/
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    /* Variaveis de controle do loby de jogo */
    int timer, executioner_cid=-1, lobby[MAXCLIENTS];

    /* Socket do servidor */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, 10);

    while(1){
      /* Zeramos o set e marcamos o socket nele */
      FD_ZERO(&listen_set);
      FD_SET(listenfd, &listen_set);

      /* So chamamos accept se houver uma solicitacao de conexao */
      rv = select(listenfd + 1, &listen_set, NULL, NULL, &timeout);

      if (rv > 0){

        connfd = Accept(listenfd, (struct sockaddr *) &cliinfo, &cliinfo_len);
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

          /* Fecha a via de escrita de in_pipe*/
          close(in_pipes[cid][1]);
          /* Fecha a via de leitura de out_pipe*/
          close(out_pipes[cid][0]);


          /* Espera ate que o cliente retorne a sua escolha de modo de jogo*/
          read(connfd, buffer, sizeof(buffer));
          printf("Cliente escolheu %s\n", buffer);

          if(buffer[0] == '2'){
            /* Repassamos a escolha para o parent process */
            write(out_pipes[cid][1], buffer, sizeof(buffer));

            /* Vemos qual a decisao do parent */
            read(in_pipes[cid][0], buffer, sizeof(buffer));

            bzero(buffer, sizeof(buffer));

            /* O cliente nao se tornou carrasco */
            if(buffer[0] == 'x'){
              sprintf(buffer, "A partida ja possui um carrasco");
              write(connfd, buffer, sizeof(buffer));

            /* O cliente se tornou carrasco */
            }else{
              sprintf(buffer, "Voce eh o carrasco da partida");
              write(connfd, buffer, sizeof(buffer));

            }

          }else if (buffer[0] == '3'){
            /* Repassamos a escolha para o parent process */
            write(out_pipes[cid][1], buffer, sizeof(buffer));
          }


          /* Atribui o modo de jogo ao cliente */
          switch(buffer[0]){
            case '1':
            /* Caso simples; so eh necessario chamar o loop de jogo*/
            printf("Option %c", buffer[0]);
            break;

            case '2':
            /* Jogador tenta ser o carrasco do lobby */
            printf("Option %c", buffer[0]);
            break;

            case '3':
            printf("Option %c", buffer[0]);
            break;
          }

        }else{
          total_clients++;

          /* Fecha a via de leitura de in_pipe*/
          close(in_pipes[total_clients][0]);
          /* Fecha a via de escrita de out_pipe*/
          close(out_pipes[total_clients][1]);
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

            }else if(out_pipes[i][0] == '3'){

            }
          }
        }

      }

    }
    return 0;
}
