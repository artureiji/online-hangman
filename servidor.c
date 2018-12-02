#include "socket_helper.h"
#include "time.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define MAXDATASIZE 255
#define MAXWORDSIZE 33
#define MAXCLIENTS 255
#define MAXPLAYS 10

typedef struct Game {
  enum TYPE {SINGLE_PLAYER, MULTIPLAYER_LEADERLESS, MULTIPLAYER} type;
  char *word;
  char guesses[MAXWORDSIZE], guess_state[2*MAXWORDSIZE];
  int guess_count;
  int hits;
} Game;

typedef struct Player {
  int connfd;
  int isLeader;
  char ip[20], *name;
  int udp_port;
  enum STATE { PLAYER_ON_LOBBY, PLAYER_INGAME_MP, PLAYER_INGAME_SP } state;
  int games_count;
  int lives;
  int misses;
  char used_words[MAXPLAYS][MAXWORDSIZE];
  Game *game;
} Player;


Player setup_new_player(int connfd, struct sockaddr_in address)
{
  Player player;
  player.connfd = connfd;
  inet_ntop(AF_INET, &address.sin_addr, player.ip, sizeof(player.ip));
  player.state = PLAYER_ON_LOBBY;
  player.games_count = 0;

  return player;
};

void start_single_player_game(char** words, int word_count, Player *player)
{
  int i, j, available;
  Game *game = malloc(sizeof(Game));

  game->word = NULL;
  for (i = 0; i < word_count && game->word == NULL; i++) {
    printf("Checking availability of word %s\n", words[i]);
    available = 1;
    for (j = 0; j < player->games_count; j++) {
      printf("Comparing with word %s\n", player->used_words[i]);
      if (strcmp(words[i], player->used_words[j]) == 0) {
        available = 0;
      }
    }
    if (available) {
      game->word = malloc(sizeof(words[i]));
      strcpy(game->word, words[i]);
    }
  }

  game->type = SINGLE_PLAYER;
  game->hits = 0;
  game->guess_count = 0;
  for (i = 0; i < strlen(game->word); i++) {
    game->guess_state[2 * i] = '_';
    game->guess_state[2 * i + 1] = ' ';
  }

  player->game = game;
  player->state = PLAYER_INGAME_SP;
  strcpy(player->used_words[player->games_count], game->word);
  player->games_count++;

  player->lives = 6;
  player->misses = 0;
}

void processa_tentativa(Player *player, char *command)
{
  int i, tiles_left, new_guess;
  char buffer[255], guess, guess_hits = 0;
  
  new_guess = 1;
  if (strlen(command) == 1) {
    guess = toupper(command[0]);

    for (i = 0; i < player->game->guess_count; i++) {
      if (guess == player->game->guesses[i]) {
        new_guess = 0;
      }
    }

    if (new_guess) {
      player->game->guesses[player->game->guess_count] = guess;
      player->game->guess_count++;
      for (i = 0; i < strlen(player->game->word); i++) {
        if (toupper(command[0]) == toupper(player->game->word[i])) {
          player->game->guess_state[2 * i] = toupper(player->game->word[i]);
          player->game->hits++;
          guess_hits++;
        }
      }

      if (guess_hits == 0) {
        player->lives--;
        player->misses++;
      }
    }
  } else {
    if (strcasecmp(command, player->game->word) == 0) {
      player->game->hits = strlen(player->game->word);
    } else {
      player->lives = 0;
      player->misses++;
    }
  }

  tiles_left = strlen(player->game->word) - player->game->hits;
  write(player->connfd, &tiles_left, 1);
  write(player->connfd, &player->lives, 1);
  write(player->connfd, player->game->guess_state, strlen(player->game->guess_state));

  if (tiles_left <= 0) {
      snprintf(buffer, sizeof(buffer), "\nParabéns, você adivinhou a palavra '%s'.\n", player->game->word);
      write(player->connfd, buffer, strlen(buffer));
      bzero(buffer, sizeof(buffer));

      player->state = PLAYER_ON_LOBBY;
  } else if (player->lives <= 0) {
      snprintf(buffer, sizeof(buffer), "\nForca!, você fez %d tentativas incorretas...\n", player->misses);
      write(player->connfd, buffer, strlen(buffer));
      bzero(buffer, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), "\nA palavra correta era '%s', você perdeu!\n", player->game->word);
      write(player->connfd, buffer, strlen(buffer));
      bzero(buffer, sizeof(buffer));

      player->state = PLAYER_ON_LOBBY;
  } else if (!new_guess) {
    snprintf(buffer, sizeof(buffer), "\nA letra '%c' já foi utilizada.\n", command[0]);
    write(player->connfd, buffer, strlen(buffer));
    bzero(buffer, sizeof(buffer));
  } else if (guess_hits <= 0) {
    snprintf(buffer, sizeof(buffer), "\nA palavra não tem nenhuma letra '%c'.\n", command[0]);
    write(player->connfd, buffer, strlen(buffer));
    bzero(buffer, sizeof(buffer));
  }
}

int main(int argc, char **argv) {
    Player clientes[MAXCLIENTS];
    Game *curr_game, *open_mp_game;
    /* Variavies relativas a conexao com o cliente*/
    struct sockaddr_in servaddr, cliinfo;
    socklen_t cliinfo_len;
    int word_size, maxfd, listenfd, connfd, option=1, port=9000;
    int n, total_clients = 0;
    char buffer[MAXDATASIZE];
    char *words[] = {"batata", "sockets"};

    fd_set active_set, read_set;

    /* Socket do servidor */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, 10);

    /* Zeramos o set e marcamos o socket nele */
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);
    maxfd = listenfd;
    while(1){
      active_set = read_set;
      select(maxfd + 1, &active_set, NULL, NULL, NULL);

      /* So chamamos accept se houver uma solicitacao de conexao */
      if (FD_ISSET(listenfd, &active_set) && total_clients < MAXCLIENTS) {
        cliinfo_len = sizeof(cliinfo);
        connfd = Accept(listenfd, (struct sockaddr *) &cliinfo, &cliinfo_len);

        /* Guardando as informacoes de conexao com o cliente */
        clientes[total_clients] = setup_new_player(connfd, cliinfo);
        FD_SET(connfd, &read_set);
        if (connfd > maxfd) {
          maxfd = connfd;
        }
        total_clients++;
        printf("Created user %d\n", connfd);
      }
      
      for(int i = 0; i < total_clients; i++) {
        if (FD_ISSET(clientes[i].connfd, &active_set)) {
          n = recv(clientes[i].connfd, buffer, sizeof(buffer), MSG_PEEK);
          if (n <= 0) {
            recv(clientes[i].connfd, buffer, sizeof(buffer), MSG_DONTWAIT);
            close(clientes[i].connfd);
            FD_CLR(clientes[i].connfd, &read_set);
            continue;
          }
          
          if (clientes[i].state == PLAYER_ON_LOBBY) {
            n = read(clientes[i].connfd, buffer, sizeof(buffer));
            printf("Cliente (%s) escolheu opção %s (%d bytes received)\n", clientes[i].ip, buffer, n);

            switch (buffer[0]) {
              case '1':
                start_single_player_game(words, 2, &clientes[i]);
                curr_game = clientes[i].game;
                printf("Starting single player word '%s' for (%d, %s).\n", curr_game->word, i, clientes[i].ip);

                word_size = strlen(curr_game->word);
                write(clientes[i].connfd, &word_size, 1);
                write(clientes[i].connfd, &clientes[i].lives, 1);
                write(clientes[i].connfd, curr_game->guess_state, strlen(curr_game->guess_state));
                break;
              case '2':
                start_single_player_game(words, 2, &clientes[i]);
                curr_game = clientes[i].game;
                printf("Starting single player word '%s' for (%s).\n", curr_game->word, clientes[i].ip);

                word_size = strlen(curr_game->word);
                write(clientes[i].connfd, &word_size, 1);
                write(clientes[i].connfd, &clientes[i].lives, 1);
                write(clientes[i].connfd, curr_game->guess_state, strlen(curr_game->guess_state));
                break;
              case '3':
                start_single_player_game(words, 2, &clientes[i]);
                curr_game = clientes[i].game;
                printf("Starting single player word '%s' for (%s).\n", curr_game->word, clientes[i].ip);

                word_size = strlen(curr_game->word);
                write(clientes[i].connfd, &word_size, 1);
                write(clientes[i].connfd, &clientes[i].lives, 1);
                write(clientes[i].connfd, curr_game->guess_state, strlen(curr_game->guess_state));
                break;
            }
          } else if (clientes[i].state == PLAYER_INGAME_SP) {
            printf("Reading user guess\n");
            bzero(buffer, sizeof(buffer));
            n = read(clientes[i].connfd, buffer, sizeof(buffer));
            printf("Handling guess %s from player (%d, %s) (%d bytes received)\n.", buffer, i, clientes[i].ip, n);
            processa_tentativa(&clientes[i], buffer);
          }
        }
      }
    }

    return 0;
}
