#include "socket_helper.h"

int Socket(int domain, int type, int protocol) {
   int sockfd;

   if ((sockfd = socket(domain, type, protocol)) < 0) {
      perror("socket error");
      exit(1);
   }

   return sockfd;
}

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
   int result;
   
   if ((result = bind(sockfd, addr, addrlen)) == -1) {
      perror("bind");
      exit(1);
   }

   return result;
}

int Listen(int sockfd, int backlog) {
   int result;
   
   if ((result = listen(sockfd, backlog)) == -1) {
      perror("listen");
      exit(1);
   }

   return result;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
   int connfd;

   if ((connfd = accept(sockfd, addr, addrlen)) == -1) {
      perror("accept");
      exit(1);
   }

   return connfd;
}

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
   int result;

   if ((result = connect(sockfd, addr, addrlen)) < 0) {
      perror("connect error");
      exit(1);
   }

   return result;
}

int Close(int fd) {
   int result;
   
   if ((result = close(fd)) < 0) {
      perror("connect error");
   }

   return result;
}

int Getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
   int result = getsockname(sockfd, addr, addrlen);
   
   if (result < 0) {
      perror("getsockname() failed");
   }

   return result;
}

struct sockaddr_in ServerSockaddrIn(int family, unsigned long ip, unsigned short port) {
   struct sockaddr_in addr;

   bzero(&addr, sizeof(addr));         
   addr.sin_family      = family;      
   addr.sin_addr.s_addr = htonl(ip);   
   addr.sin_port        = htons(port); 

   return addr;
}

struct sockaddr_in ClientSockaddrIn(int family, const char *ip, unsigned short port) {
   struct sockaddr_in addr;

   bzero(&addr, sizeof(addr));         
   addr.sin_family      = family; 
   addr.sin_port        = htons(port); 

   if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
      perror("inet_pton error");
      exit(1);
   }

   return addr;
}