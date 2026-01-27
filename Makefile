CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lws2_32
GDI_FLAGS = -lgdi32 -lopengl32

# Directories
SERVER_DIR = Server
CLIENT_DIR = Client
UTILS_DIR = Utilities
MENU_UTILS_DIR = $(UTILS_DIR)/MenuUtilities
OBJS_DIR = Objects
AI_DIR = AI

# Source Files
SERVER_SRC = $(SERVER_DIR)/server.c $(UTILS_DIR)/networkUtilities.c $(UTILS_DIR)/gameUtilities.c $(UTILS_DIR)/renderUtilities.c \
	$(UTILS_DIR)/openglUtilities.c $(UTILS_DIR)/cameraUtilities.c \
	$(MENU_UTILS_DIR)/commonMenuUtilities.c $(MENU_UTILS_DIR)/menuComponentUtilities.c $(MENU_UTILS_DIR)/colorPickerUtilities.c $(MENU_UTILS_DIR)/lobbyMenuUtilities.c $(MENU_UTILS_DIR)/lobbyPreviewUtilities.c $(MENU_UTILS_DIR)/gameOverUIUtilities.c \
	$(OBJS_DIR)/level.c $(OBJS_DIR)/planet.c $(OBJS_DIR)/starship.c $(OBJS_DIR)/vec2.c $(OBJS_DIR)/faction.c $(OBJS_DIR)/player.c \
	$(AI_DIR)/aiPersonality.c $(AI_DIR)/idleAI.c $(AI_DIR)/basicAI.c
CLIENT_SRC = $(CLIENT_DIR)/client.c \
	$(UTILS_DIR)/networkUtilities.c $(UTILS_DIR)/gameUtilities.c $(UTILS_DIR)/renderUtilities.c \
	$(UTILS_DIR)/openglUtilities.c $(UTILS_DIR)/cameraUtilities.c $(UTILS_DIR)/playerInterfaceUtilities.c \
	$(MENU_UTILS_DIR)/commonMenuUtilities.c $(MENU_UTILS_DIR)/menuComponentUtilities.c $(MENU_UTILS_DIR)/loginMenuUtilities.c $(MENU_UTILS_DIR)/colorPickerUtilities.c $(MENU_UTILS_DIR)/lobbyMenuUtilities.c $(MENU_UTILS_DIR)/lobbyPreviewUtilities.c $(MENU_UTILS_DIR)/gameOverUIUtilities.c \
	$(OBJS_DIR)/level.c $(OBJS_DIR)/planet.c $(OBJS_DIR)/starship.c $(OBJS_DIR)/vec2.c $(OBJS_DIR)/faction.c $(OBJS_DIR)/player.c \
	$(AI_DIR)/aiPersonality.c $(AI_DIR)/idleAI.c $(AI_DIR)/basicAI.c

# Targets
all: server client

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -I$(SERVER_DIR) $(SERVER_SRC) -o server.exe $(LDFLAGS) $(GDI_FLAGS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -I$(CLIENT_DIR) $(CLIENT_SRC) -o client.exe $(LDFLAGS) $(GDI_FLAGS)

clean:
	if exist server.exe del server.exe
	if exist client.exe del client.exe