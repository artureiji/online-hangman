CFLAGS = -Wall -g

LIB_CODE = socket_helper.c
LIB_HEADER = socket_helper.h

all: cliente servidor

cliente: cliente.c $(LIB_CODE) $(LIB_HEADER)
	gcc $(CFLAGS) cliente.c $(LIB_CODE) -o cliente

servidor: servidor.c $(LIB_CODE) $(LIB_HEADER)
	gcc $(CFLAGS) servidor.c $(LIB_CODE) -o servidor

.PHONY: clean
cleancliente: clean
cleanservidor: clean
clean:
	rm -f cliente servidor
