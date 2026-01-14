CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lws2_32
GDI_FLAGS = -lgdi32

# Directories
SERVER_DIR = Server
CLIENT_DIR = Client
UTILS_DIR = Utilities

# Source Files
SERVER_SRC = $(SERVER_DIR)/server.c $(UTILS_DIR)/networkUtilities.c $(UTILS_DIR)/gameUtilities.c
CLIENT_SRC = $(CLIENT_DIR)/client.c $(UTILS_DIR)/networkUtilities.c

# Targets
all: server client

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -I$(SERVER_DIR) $(SERVER_SRC) -o server.exe $(LDFLAGS) $(GDI_FLAGS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -I$(CLIENT_DIR) $(CLIENT_SRC) -o client.exe $(LDFLAGS)

clean:
	if exist server.exe del server.exe
	if exist client.exe del client.exe