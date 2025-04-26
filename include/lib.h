#ifndef LIB_H
#define LIB_H

#include <glib.h>

#define MAX_ARGUMENTS_SIZE 512

#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_YEAR 4
#define MAX_PATH 64

typedef struct {
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
} DocumentInfo;

typedef struct {
    char flag;                          // 'a', 'c', 'd', 'l', 's', 'f'
    char arguments[MAX_ARGUMENTS_SIZE]; // Todos os argumentos concatenados (separados por '|')
    int processID;                      // Para identificação do cliente (pode ser útil futuramente)
} Command;



void handle_error(char *message);
void build_command(Command *cmd, int argc, char *argv[]);

// ===========================Server Helpers=================================
void handle_add(Command *cmd, GHashTable *table, int *current_id);
void handle_consult(Command *cmd, GHashTable *table);
void handle_delete(Command *cmd, GHashTable *table);
void handle_lines_with_keyword(Command *cmd, GHashTable *table);
void handle_client_response(Command *cmd, GHashTable *table);
void handle_shutdown(Command *cmd, GHashTable *table);


#endif
