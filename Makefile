CFLAGS = -Wall -g

SERVER_LIB_CODE = socket_helper.c
SERVER_LIB_HEADER = socket_helper.h

CLIENT_LIB_CODE = socket_helper.c chat.c
CLIENT_LIB_HEADER = socket_helper.h chat.h

all: cliente servidor

cliente: cliente.c $(CLIENT_LIB_CODE) $(CLIENT_LIB_HEADER)
	gcc $(CFLAGS) cliente.c $(CLIENT_LIB_CODE) -o cliente

servidor: servidor.c $(SERVER_LIB_CODE) $(SERVER_LIB_HEADER)
	gcc $(CFLAGS) servidor.c $(SERVER_LIB_CODE) -o servidor

.PHONY: clean
cleancliente: clean
cleanservidor: clean
clean:
	rm -f cliente servidor
