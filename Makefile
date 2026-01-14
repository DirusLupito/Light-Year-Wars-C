all: server client

server: main.c
	gcc -Wall main.c -o LightYearWarsC.exe -lgdi32 -lws2_32

client: client.c
	gcc -Wall client.c -o client.exe -lws2_32