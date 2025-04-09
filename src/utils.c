#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include "lib.h"


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

    // ===========================Saving arguments splitted with '|'=================================
    for (int i = 2; i < argc; i++) {
        strncat(cmd->arguments, argv[i], MAX_ARGUMENTS_SIZE - strlen(cmd->arguments) - 1);
        if (i < argc - 1) {
            strncat(cmd->arguments, "|", MAX_ARGUMENTS_SIZE - strlen(cmd->arguments) - 1);
        }
    }
}


// ===========================Server Helpers=================================

void handle_add(Command *cmd, GHashTable *table, int *current_id) {
    // ===========================Setting up variables=================================
    char *args = strdup(cmd->arguments);
    char *title = strtok(args, "|");
    char *authors = strtok(NULL, "|");
    char *year = strtok(NULL, "|");
    char *path = strtok(NULL, "|");

    // ===========================Variable checking=================================
    if (!title || !authors || !year || !path) {
        fprintf(stderr, "Error: invalid arguments with flag -a\n");
        free(args);
        return;
    }

    // ===========================Creating and Filling Structure=================================
    DocumentInfo *doc = malloc(sizeof(DocumentInfo));
    strncpy(doc->title, title, MAX_TITLE);
    strncpy(doc->authors, authors, MAX_AUTHORS);
    strncpy(doc->year, year, MAX_YEAR);
    strncpy(doc->path, path, MAX_PATH);

    // ===========================Unique identifier to send to client=================================
    int *id = malloc(sizeof(int));
    *id = (*current_id)++;

    // ===========================Inserting in Hashtable=================================
    g_hash_table_insert(table, id, doc);

    // ===========================Building Response Message=================================
    char response[128];
    snprintf(response, sizeof(response), "Document %d indexed\n", *id);

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

DocumentInfo* find_document_by_id(GHashTable *table, int id) {
    int *key = malloc(sizeof(int));
    *key = id;
    DocumentInfo *doc = g_hash_table_lookup(table, key);
    free(key);
    return doc;
}

void handle_consult(Command *cmd, GHashTable *table) {
    // ===========================Getting key from the command=================================
    int key = atoi(cmd->arguments);

    // ===========================Searching the Hashtable=================================
    DocumentInfo *doc = find_document_by_id(table, key);

    // ===========================Setting up response to the client=================================
    char response[512];
    if (doc) {
        snprintf(response, sizeof(response),
                 "Title: %s\nAuthors: %s\nYear: %s\nPath: %s\n",
                 doc->title, doc->authors, doc->year, doc->path);
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

void handle_delete(Command *cmd, GHashTable *table) {
    // ===========================Getting key from the command=================================
    int key = atoi(cmd->arguments);

    // ===========================Setting a copy to remove=================================
    int *key_ptr = &key;

    // ===========================Setting up response to the client=================================
    char response[128];
    if (g_hash_table_remove(table, key_ptr)) {
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

void handle_lines_with_keyword(Command *cmd, GHashTable *table) {
    // ===========================Getting arguments from the command=================================
    char *args = strdup(cmd->arguments);
    char *key_str = strtok(args, "|");
    char *keyword = strtok(NULL, "|");

    // ===========================Setting up response to the client=================================
    // ! ISTO PROVAVELMENTE NÃO ESTÁ BEM ATÉ À ABERTURA DO FIFO EM BAIXO - VERIFICAR !!!!
    // ! CONFIRMADO, NÃO ESTÁ BEM :(
    char response[128];

    if (!key_str || !keyword) {
        snprintf(response, sizeof(response), "Error: invalid arguments for flag -l\n");
    } else {
        int key = atoi(key_str);
        DocumentInfo *doc = g_hash_table_lookup(table, &key);

        if (!doc) {
            snprintf(response, sizeof(response), "Couldn't find document with ID %d\n", key);
        } else {
            // ===========================Setting up 'grep | wc -l' command=================================
            char command[512];
            snprintf(command, sizeof(command),
                     "grep -c '%s' '%s' 2>/dev/null", keyword, doc->path);

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

void handle_client_response(Command *cmd, GHashTable *table) {
    switch (cmd->flag)
    {
    case 'c':
        handle_consult(cmd, table);
        break;
    case 'l':
        handle_lines_with_keyword(cmd, table); // TODO : não está bem implementado :(
        break;
    case 's':
        // TODO logic for flag -s
        break;      
    default:
        break;
    }
}
