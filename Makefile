CC = gcc
PKG_CONFIG := pkg-config
GLIB_CFLAGS := $(shell $(PKG_CONFIG) --cflags glib-2.0)
GLIB_LIBS := $(shell $(PKG_CONFIG) --libs glib-2.0)

CFLAGS = -Wall -g -Iinclude $(GLIB_CFLAGS)
LDFLAGS = $(GLIB_LIBS)

# Lista de todos os arquivos fonte
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

# Targets dos executáveis
SERVER_EXEC = bin/dserver
CLIENT_EXEC = bin/dclient

# Regras principais
all: folders $(SERVER_EXEC) $(CLIENT_EXEC)

folders:
	@mkdir -p obj bin tmp

# Compilar dserver
$(SERVER_EXEC): obj/dserver.o obj/utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Compilar dclient
$(CLIENT_EXEC): obj/dclient.o obj/utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Regra geral para compilar objetos
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/* bin/* tmp/* obj bin tmpa
