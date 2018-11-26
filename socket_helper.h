#ifndef SOCKET_HELPER
#define SOCKET_HELPER

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Close(int fd);

int Getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

struct sockaddr_in ServerSockaddrIn(int family, unsigned long ip, unsigned short port);

struct sockaddr_in ClientSockaddrIn(int family, const char *ip, unsigned short port);

#endif