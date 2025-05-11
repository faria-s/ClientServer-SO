#ifndef SERVER_HELPERS_H
#define SERVER_HELPERS_H

#include <glib.h>

#include "utils.h"
#include "cache.h"

#define MAX_ARGUMENTS_SIZE 512

#define PIPE_NAME "/tmp/doc_index_pipe"
#define DISK_PATH "meta_info.txt"

#define NOT_FOUND 0
#define FOUND_IN_CACHE 1
#define FOUND_ON_DISK 2

typedef struct {
    char flag;                          // 'a', 'c', 'd', 'l', 's', 'f'
    char arguments[MAX_ARGUMENTS_SIZE]; // Todos os argumentos concatenados (separados por '|')
    int number_arguments;               // number arguments
    int processID;                      // Para identificação do cliente (pode ser útil futuramente)
} Command;



void handle_error(char *message);
void build_command(Command *cmd, int argc, char *argv[]);

// ===========================Server Helpers=================================
void handle_add(Command *cmd, Cache *cache, int *current_id, int save_fd, char* folder_path, int **header_ptr);
void handle_consult(Command *cmd, Cache *cache, int save_fd, int **header);
void handle_delete(Command *cmd, Cache *cache, int saved_fd,int header[]);
void handle_lines_with_keyword(Command *cmd, Cache *cache,int fd, int header[]);
void handle_client_response(Command *cmd, Cache* cache, int save_fd, int* current_id, char* path, int **header_ptr);
void handle_shutdown(Command *cmd, Cache *cache);
void handle_search(Command *cmd,Cache *cache, int save_fd, int header[]);
int handle_write_on_disk(int fd, DocumentInfo *doc, Cache *cache, char cmd, int id);
int handle_file_exists(Cache *cache, int fd, int index, int header[]);

#endif
