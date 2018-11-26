#include "socket_helper.h"

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    int listenfd;

    servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

    Bind(listenfd, servaddr, sizeof(servaddr));
    Listen(listenfd, 10);

    return 0;
}
