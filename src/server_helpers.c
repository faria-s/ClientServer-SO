#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>

#include "cache.h"
#include "utils.h"
#include "server_helpers.h"

void handle_error(char *message) {
    char error[100];
    snprintf(error, sizeof(error), "%s%s", "Error: ", message);
    perror(error);
    exit(EXIT_FAILURE);
}


void build_command(Command *cmd, int argc, char *argv[]) {
    // ===========================Checking conditions=================================
    if (argc < 2) {
        handle_error("Error: insufficient arguments\n");
    }

    // ===========================Checking flag in right format '-x'=================================
    if (argv[1][0] != '-' || strlen(argv[1]) != 2) {
        handle_error("Error: invalid flag format\n");
    }

    // ===========================Saving variables in structure=================================
    cmd->flag = argv[1][1];
    cmd->processID = getpid();
    cmd->arguments[0] = '\0';
    cmd->number_arguments = argc;

    // ===========================Saving arguments splitted with '|'=================================
    for (int i = 2; i < argc; i++) {
        strncat(cmd->arguments, argv[i], MAX_ARGUMENTS_SIZE - strlen(cmd->arguments) - 1);
        if (i < argc - 1) {
            strncat(cmd->arguments, "|", MAX_ARGUMENTS_SIZE - strlen(cmd->arguments) - 1);
        }
    }
}

void handle_add(Command *cmd, Cache *cache, int *current_id, int save_fd, char* folder_path, int header[]) {
    // ===========================Setting up variables=================================
    char *args = strdup(cmd->arguments);
    char *title = strtok(args, "|");
    char *authors = strtok(NULL, "|");
    char *year = strtok(NULL, "|");
    char *file_path = strtok(NULL, "|");

    // ===========================Variable checking=================================
    if (!title || !authors || !year || !file_path) {
        perror("Error: invalid arguments with flag -a\n");
        free(args);
        return;
    }

    // ===========================Defininig path=================================
    char path[128];
    snprintf(path, sizeof(path), "%s%s", folder_path, file_path);

    // ===========================Unique identifier to send to client=================================
    int index = find_empty_index(header);

    // ===========================Creating and Filling Structure=================================
    DocumentInfo *doc = malloc(sizeof(DocumentInfo));
    doc->id = index;
    strncpy(doc->title, title, MAX_TITLE);
    strncpy(doc->authors, authors, MAX_AUTHORS);
    strncpy(doc->year, year, MAX_YEAR);
    strncpy(doc->path, path, MAX_PATH);

    // ===========================Verifying If Path Exists=================================
    char response[128];
    if(access(doc->path,F_OK) == 0){
        // ===========================Unique identifier to send to client=================================
        int *id = malloc(sizeof(int));
        *id = index;

        // ===========================Inserting in Hashtable=================================
        header[index] = index;
        handle_write_on_disk(save_fd, doc,cache,'a', doc->id);

        //for(int j= 0; j < HEADER_SIZE; j++){
            //printf("%d: %d\n", j, header[j]);
            //}

        // ===========================Building Response Message=================================
        snprintf(response, sizeof(response), "Document %d indexed\n", *id);
    }
    else{
        snprintf(response, sizeof(response), "Document path doesn't exist\n");
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        free(args);
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);
    free(args);
}


void handle_consult(Command *cmd, Cache *cache, int save_fd, int header[]) {
    // ===========================Getting key from the command=================================
    int key = atoi(cmd->arguments);

    // ===========================Searching the Hashtable=================================
    handle_file_exists(cache, save_fd, key, header);
    DocumentInfo *doc = cache_get(cache, key);

    // ===========================Setting up response to the client=================================
    char response[512];
    if (doc) {
        snprintf(response, sizeof(response),
                 "ID: %d \nTitle: %s\nAuthors: %s\nYear: %s\nPath: %s\n",
                 doc->id,doc->title, doc->authors, doc->year, doc->path);
    } else {
        snprintf(response, sizeof(response),
                 "Couldn't find document with ID %d\n", key);
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);
}

void handle_delete(Command *cmd, Cache *cache, int saved_fd,int header[]) {
    // ===========================Getting key from the command=================================
    int key = atoi(cmd->arguments);

    // ===========================Removing=================================
    int removed = handle_write_on_disk(saved_fd, NULL, cache, 'd', key);

    // ===========================Setting up response to the client=================================
    char response[128];
    if (removed) {
        cache_remove(cache, key);
        header[key] = 0;
        snprintf(response, sizeof(response), "Index entry %d deleted\n", key);
    } else {
        snprintf(response, sizeof(response), "Couldn't find document with ID %d\n", key);
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);
}

void handle_lines_with_keyword(Command *cmd, Cache *cache, int save_fd, int header[]) {
    // ===========================Getting arguments from the command=================================
    char *args = strdup(cmd->arguments);
    char *key_str = strtok(args, "|");
    char *keyword = strtok(NULL, "|");

    // ===========================Setting up response to the client=================================
    char response[128];

    if (!key_str || !keyword) {
        snprintf(response, sizeof(response), "Error: invalid arguments for flag -l\n");
    } else {
        int key = atoi(key_str);
        handle_file_exists(cache, save_fd, key, header);
        DocumentInfo *doc = cache_get(cache, key);

        if (!doc) {
            snprintf(response, sizeof(response), "Couldn't find document with ID %d\n", key);
        } else {
            // ===========================Setting up 'grep | wc -l' command=================================
            char command[512];
            snprintf(command, sizeof(command),
                     "grep -c '%s' '%s' 2>/dev/null", keyword, doc->path); // hides error messages

            FILE *fp = popen(command, "r");
            if (fp == NULL) {
                snprintf(response, sizeof(response), "Error executing grep command\n");
            } else {
                int count;
                fscanf(fp, "%d", &count);
                snprintf(response, sizeof(response), "%d\n", count);
                pclose(fp);
            }
        }
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);

    free(args);
}

void handle_search(Command *cmd,Cache *cache) {
    char *args = strdup(cmd->arguments);
    char response[512];
    response[0] = '\0';

    // ===========================Verifies If Has nr_processes=================================
    if (cmd->number_arguments == 3) {
        char *keyword = args;

        strcat(response, "[");

        // ===========================Gets Number of Indexed Documents=================================
        int size = g_hash_table_size(cache->cache);
        int first = 1;

        // ===========================Verifies Every Document=================================
        for (int i = 1; i <= size; i++) {
            DocumentInfo *doc = find_document_by_id(cache->cache, i);

            if (!doc) continue;

            // ===========================Creating Pipe=================================
            int pfd[2];
            if (pipe(pfd) == -1) {
                perror("pipe failed");
                continue;
            }

            // ===========================Creating Child Process To Execute grep=================================
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                close(pfd[0]);
                close(pfd[1]);
                continue;
            }

            if (pid == 0) {
                // ===========================CHILD=================================
                close(pfd[0]);
                // ===========================STDOUT Poiting At Writing Pipe=================================
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);

                // ===========================Executing Grep=================================
                execlp("grep", "grep", keyword, doc->path, NULL);

                perror("execlp failed");
                _exit(1);
            } else {
                // ===========================PARENT=================================
                close(pfd[1]);

                // ===========================Reads Grep Output=================================
                char buffer[256];
                ssize_t count = read(pfd[0], buffer, sizeof(buffer) - 1);
                close(pfd[0]);

                // ===========================Waits For Child Process To Finish=================================
                int status;
                waitpid(pid, &status, 0);

                // ===========================Verifies If Grep Output Is Empty=================================
                if (count > 0) {
                    // ===========================Adds Document ID To Response=================================
                    if (!first) {
                        strcat(response, ",");
                    }
                    char id_str[16];
                    snprintf(id_str, sizeof(id_str), "%d", i);
                    strcat(response, id_str);
                    first = 0;
                }
            }
        }

        strcat(response, "]\n");
    }
    else if(cmd->number_arguments == 4){
        // ===========================Getting arguments from the command=================================
        char *keyword = strtok(args, "|");
        char *nr_processes= strtok(NULL, "|");

        // ===========================Getting key from the command=================================
        int max_processes = atoi(nr_processes);
        if(!max_processes){
            perror("Invalid nr_processes input\n");
            free(args);
            return;
        }
    }
    else{
        perror("Incorrect number of arguments \n");
        free(args);
        return;
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        free(args);
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);
    free(args);
}

void handle_shutdown(Command *cmd, Cache *cache) {
    char response[128];
    snprintf(response, sizeof(response), "Server is shutting down\n");

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        return;
    }
    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    g_hash_table_destroy(cache->cache);
    close(fd);
}


void handle_client_response(Command *cmd, Cache* cache, int save_fd, int* current_id, char* path, int header[]) {
    switch (cmd->flag)
    {
    case 'a':
        handle_add(cmd, cache, current_id,save_fd,path,header);
        break;
    case 'd':
        handle_delete(cmd, cache, save_fd, header);
        break;
    case 'c':
        handle_consult(cmd, cache,save_fd, header);
        break;
    case 'l':
        handle_lines_with_keyword(cmd, cache, save_fd, header);
        break;
   case 's':
        handle_search(cmd, cache);
        break;
    default:
        break;
    }
}

int handle_write_on_disk(int fd, DocumentInfo *doc, Cache *cache, char cmd, int id){
    int writed = 0;

    if (fd == -1) {
        handle_error("Write on save file");
    }

    if (lseek(fd, (id)*sizeof(int), SEEK_SET) == -1) {
        handle_error("lseek to beginning failed");
    }

    if(cmd == 'a'){

        if (write(fd, &id, sizeof(int)) != sizeof(int)) {
            handle_error("Failed to write current_id");
        }

        if (lseek(fd, 0, SEEK_END) == -1) {
            handle_error("lseek to end failed");
        }

        if (write(fd, doc, sizeof(DocumentInfo)) != sizeof(DocumentInfo)) {
            handle_error("Failed to write DocumentInfo");
        }

        cache_put(cache, doc);
        writed = 1;
    }
    else{
        int set_zero = 0;

        if (write(fd, &set_zero, sizeof(int)) != sizeof(int)) {
            handle_error("Failed to write current_id");
        }

        writed = 1;
    }

    return writed;
}

int handle_file_exists(Cache *cache, int fd, int index, int header[]) {
    int where_exists = NOT_FOUND;

    int key = index;

    if (g_hash_table_contains(cache->cache, &key)) {
        where_exists = FOUND_IN_CACHE;
    } else{
        int isIndexed;

        lseek(fd, index * sizeof(int), SEEK_SET);
        if(read(fd, &isIndexed, sizeof(int)) != sizeof(int)){
            handle_error("couldn't read header");
        }

        if(isIndexed > 0){
            DocumentInfo *doc = malloc(sizeof(DocumentInfo));
            if (!doc) {
                handle_error("doc allocate memory fail");
            }

            printf("Entrou");
            lseek(fd, ((index-1) * sizeof(DocumentInfo)) + (HEADER_SIZE * sizeof(int)), SEEK_SET);

            if (read(fd, doc, sizeof(DocumentInfo)) == sizeof(DocumentInfo)) {
                where_exists = FOUND_ON_DISK;
                cache_put(cache, doc);
            } else {
                free(doc);
            }
        }
    }

    return where_exists;
}
