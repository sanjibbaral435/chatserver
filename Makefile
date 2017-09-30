#Makefile

all: client server

client: Client.c
	gcc Client.c -Wall -o Client

server: Server.c
	gcc Server.c -Wall -o Server

clean: rm -f client server
