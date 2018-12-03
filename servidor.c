#include "socket_helper.h"
#include "time.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define MAXDATASIZE 255
#define MAXWORDSIZE 33
#define MAXCLIENTS 255
#define MAX_PLAYERS 9
#define MAXPLAYS 10
#define TIMELIMITMP 2.0

struct Player;

typedef struct Game {
  enum TYPE {SINGLE_PLAYER, MULTIPLAYER_LEADERLESS, MULTIPLAYER} type;
  char *word;
  char guesses[MAXWORDSIZE], guess_state[2*MAXWORDSIZE];
  int guess_count;
  int hits;
  struct Player* hangman;
  struct Player* players[MAX_PLAYERS];
  int player_count;
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


Player *setup_new_player(int connfd, struct sockaddr_in address)
{
  Player *player = malloc(sizeof(Player));
  player->connfd = connfd;
  inet_ntop(AF_INET, &address.sin_addr, player->ip, sizeof(player->ip));
  player->state = PLAYER_ON_LOBBY;
  player->games_count = 0;
  player->name = NULL;

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
  game->hangman = NULL;
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

void start_multi_player_game(char **words, int word_count, Game *game)
{
  int available;
  char info_batch[MAXCLIENTS * 40], buff[50];

  bzero(info_batch, sizeof(info_batch));

  for (int i = 0; i < word_count && game->word == NULL; i++) {
    printf("Checking availability of word %s\n", words[i]);
    available = 1;

    for (int p = 0; p < game->player_count; p++){
      for (int j = 0; j < game->players[p]->games_count; j++) {
        printf("Comparing with word %s\n", game->players[p]->used_words[i]);
        if (strcmp(words[i], game->players[p]->used_words[j]) == 0) {
          available = 0;
        }
      }
    }

    if (available) {
      game->word = malloc(sizeof(words[i]));
      strcpy(game->word, words[i]);
    }
  }

  for (int p = 0; p < game->player_count; p++){
    sprintf(buff, "%s %s %d ", game->players[p]->ip, game->players[p]->name, game->players[p]->udp_port);
    strcat(info_batch, buff);
  }

  for (int p = 0; p < game->player_count; p++){
    write(game->players[p]->connfd, info_batch, strlen(info_batch));
  }

}

void processa_tentativa(Player *player, char *command)
{
  int i, tiles_left, new_guess;
  char buffer[255], guess, guess_hits = 0;

  new_guess = 1;
  if (player->game->hits < strlen(player->game->word)) {
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
  }

  tiles_left = strlen(player->game->word) - player->game->hits;
  write(player->connfd, &tiles_left, 1);
  write(player->connfd, &player->lives, 1);
  write(player->connfd, player->game->guess_state, strlen(player->game->guess_state));

  if (tiles_left <= 0) {
      snprintf(buffer, sizeof(buffer), "\nParabéns %s ,você adivinhou a palavra '%s'.\n", player->name, player->game->word);
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

void destroy_user(Player *player)
{
  free(player->name);

  if (player->game->type == SINGLE_PLAYER) {
    free(player->game);
  }
}

Game* create_mp_game() {
  Game *game;
  game = malloc(sizeof(Game));
  game->word = NULL;
  game->type = MULTIPLAYER_LEADERLESS;
  game->hangman = NULL;
  game->player_count = 0;

  return game;
}

char **load_dictionary(int *word_count){
  char **dict;
  char *line;
  size_t len = 0;
  ssize_t read;
  FILE *file;
  file = fopen("dictionary.txt", "r");

  (*word_count) = 0;

  while ((read = getline(&line, &len, file)) != -1) (*word_count)++;

  dict = malloc((*word_count) * sizeof(char*));

  (*word_count) = 0;

  file = fopen("dictionary.txt", "r");
  while ((read = getline(&line, &len, file)) != -1 && *word_count < 50) {
      dict[(*word_count)] = malloc((strlen(line) + 1) * sizeof(char));
      strcpy(dict[(*word_count)], line);
      if (dict[(*word_count)][read - 1] == '\n') {
        dict[(*word_count)][read - 1] = 0;
      }
      (*word_count)++;
  }

  return dict;
}

int main(int argc, char **argv) {
    Player *clientes[FD_SETSIZE];
    Game *curr_game, *open_mp_game;
    struct sockaddr_in servaddr, cliinfo;
    struct timeval tv;
    socklen_t cliinfo_len;
    int word_size, maxfd, listenfd, connfd, option=1, port=9000;
    int n, word_count, curr_fd, total_clients, lobby_size;
    char buffer[MAXDATASIZE];
    char **words = load_dictionary(&word_count);
    fd_set active_set, read_set;
    time_t timer;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    bzero(clientes, sizeof(clientes));

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
    total_clients = 0;
    open_mp_game = NULL;

    printf("Booting server.\n");
    while(1){

      if (open_mp_game == NULL) {
        printf("Opening new multiplayer game\n");
        open_mp_game = create_mp_game();
        lobby_size = 0;
        timer = clock();
      }

      if(open_mp_game != NULL && (((clock() - timer)/CLOCKS_PER_SEC) > TIMELIMITMP || lobby_size >= 2)) {
        if (lobby_size > 1) {
          printf("Current multiplayer game starting with players(%d):\n", open_mp_game->player_count);
          for(int p; p < open_mp_game->player_count; p++){
            printf(" - %s\n", open_mp_game->players[p]->name);
          }
          printf("\n");
          start_multi_player_game(words, word_count, open_mp_game);

          open_mp_game = NULL;
        }

        timer = clock();
      }

      if( (clock() - timer)/CLOCKS_PER_SEC > TIMELIMITMP ){
        printf("Not enough players, resetting timer\n");
        timer = clock();
      }


      active_set = read_set;
      select(maxfd + 1, &active_set, NULL, NULL, &tv);

      /* So chamamos accept se houver uma solicitacao de conexao */
      for (curr_fd = 0; curr_fd <= maxfd; curr_fd++) {
          if (FD_ISSET(curr_fd, &active_set)) {
           if (curr_fd == listenfd && total_clients < MAXCLIENTS) {
            cliinfo_len = sizeof(cliinfo);
            connfd = Accept(listenfd, (struct sockaddr *) &cliinfo, &cliinfo_len);

            /* Guardando as informacoes de conexao com o cliente */
            clientes[connfd] = setup_new_player(connfd, cliinfo);
            FD_SET(connfd, &read_set);
            if (connfd > maxfd) {
              maxfd = connfd;
            }
            total_clients++;
            printf("Created user %d\n", connfd);
           } else if (clientes[curr_fd] != NULL) {
            n = recv(curr_fd, buffer, sizeof(buffer), MSG_PEEK);
            if (n <= 0) {
              recv(curr_fd, buffer, sizeof(buffer), MSG_DONTWAIT);
              close(curr_fd);
              FD_CLR(curr_fd, &read_set);
              destroy_user(clientes[curr_fd]);
              free(clientes[curr_fd]);
              clientes[curr_fd] = NULL;
              continue;
            }

            switch (clientes[curr_fd]->state) {
              case PLAYER_ON_LOBBY:
                bzero(buffer, sizeof(buffer));
                n = read(curr_fd, buffer, sizeof(buffer));
                printf("Cliente (%s) chose mode %s (%d bytes received)\n", clientes[curr_fd]->ip, buffer, n);
                switch (buffer[0]) {
                  case '1':
                    start_single_player_game(words, 2, clientes[curr_fd]);
                    curr_game = clientes[curr_fd]->game;
                    printf("Starting single player word '%s' for (%d, %s).\n", curr_game->word, curr_fd, clientes[curr_fd]->ip);

                    word_size = strlen(curr_game->word);
                    write(curr_fd, &word_size, 1);
                    write(curr_fd, &clientes[curr_fd]->lives, 1);
                    write(curr_fd, curr_game->guess_state, strlen(curr_game->guess_state));
                    break;
                  case '2':
                    if (open_mp_game->hangman == NULL) {
                      bzero(buffer, sizeof(bzero));
                      snprintf(buffer, sizeof(buffer), "Você será o carrasco da partida!\n");
                      write(curr_fd, buffer, strlen(buffer));
                      open_mp_game->hangman = clientes[curr_fd];
                    } else {
                      bzero(buffer, sizeof(bzero));
                      snprintf(buffer, sizeof(buffer), "A partida já terá um carrasco!\n");
                      write(curr_fd, buffer, strlen(buffer));
                    }
                    break;
                  case '3':
                    if (open_mp_game->hangman != NULL) {
                      if (curr_fd != ((Player *) open_mp_game->hangman)->connfd) {
                        open_mp_game->players[open_mp_game->player_count] = clientes[curr_fd];
                        open_mp_game->player_count++;
                        printf("New player!\n");
                      } else {
                        open_mp_game->type = MULTIPLAYER;
                      }
                    }else{
                      open_mp_game->players[open_mp_game->player_count] = clientes[curr_fd];
                      open_mp_game->player_count++;
                    }

                    clientes[curr_fd]->game = open_mp_game;
                    clientes[curr_fd]->state = PLAYER_INGAME_MP;
                    lobby_size++;

                    strtok(buffer, " ");
                    clientes[curr_fd]->name = strtok(NULL, " ");
                    clientes[curr_fd]->udp_port = atoi(strtok(NULL, " "));
                    printf("Player %s (udp %d) joinned the MP game\n", clientes[curr_fd]->name,clientes[curr_fd]->udp_port);

                    break;
                }
                break;
              case PLAYER_INGAME_SP:
                printf("Reading user guess\n");
                bzero(buffer, sizeof(buffer));
                n = read(curr_fd, buffer, sizeof(buffer));
                printf("Handling guess %s from player (%d, %s) (%d bytes received)\n", buffer, curr_fd, clientes[curr_fd]->ip, n);
                processa_tentativa(clientes[curr_fd], buffer);
                break;
              case PLAYER_INGAME_MP:
                if (clientes[curr_fd]->game->word == NULL) {
                  continue;
                }
                printf("Reading user guess\n");
                bzero(buffer, sizeof(buffer));
                n = read(curr_fd, buffer, sizeof(buffer));
                printf("Handling guess %s from player (%d, %s) (%d bytes received)\n", buffer, curr_fd, clientes[curr_fd]->ip, n);
                processa_tentativa(clientes[curr_fd], buffer);
                break;
            }
          }
        }
      }
    }

    return 0;
}
