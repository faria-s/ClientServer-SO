#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>

#include "utils.h"

DocumentInfo* find_document_by_id(GHashTable *cache, int id) {
    int *key = malloc(sizeof(int));
    *key = id;
    DocumentInfo *doc = g_hash_table_lookup(cache, key);
    free(key);
    return doc;
}

void build_command(Command *cmd, int argc, char *argv[]) {
    // ===========================Checking conditions=================================
    if (argc < 2) {
        perror("Error: invalid flag format\n");
        return;
    }

    // ===========================Checking flag in right format '-x'=================================
    if (argv[1][0] != '-' || strlen(argv[1]) != 2) {
        perror("Error: invalid flag format\n");
        return;
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



void print_doc(DocumentInfo *doc){
    if(doc){
        printf("Printing...\nID: %d \nTitle: %s\nAuthors: %s\nYear: %s\nPath: %s\n",
        doc->id,doc->title, doc->authors, doc->year, doc->path);
    }
    else{
        printf("File Empty");
    }
}

int create_save_file(char* path, int header[]){

 int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
 if(fd == -1){
     perror("Error opening file");
     return -1;
 }

 for (int i = 0; i < HEADER_SIZE; i++) {
     write(fd, &header[i], sizeof(int));
 }

 return fd;
}


void load_header(int fd, int header[], int NUMBER_OF_HEADERS) {


    if (fd == -1) {
        perror("Error opening file");
        return ;
    }


    for (int header_index = 0; header_index < NUMBER_OF_HEADERS; header_index++) {
            off_t header_position = header_index * (HEADER_SIZE * sizeof(int) + HEADER_SIZE * sizeof(DocumentInfo));

            if (lseek(fd, header_position, SEEK_SET) == -1) {
                perror("Error seeking to header position while loading header");
                return;
            }

            for (int i = 0; i < HEADER_SIZE; i++) {
                int global_index = header_index * HEADER_SIZE + i;
                if (read(fd, &header[global_index], sizeof(int)) != sizeof(int)) {
                    perror("Error reading header entry from file");
                    return;
                }
            }
        }
}

int find_empty_index(int **header_ptr, int save_fd) {
    int *header = *header_ptr;
    int index = 0;
    int NUMBER_OF_HEADERS = header[0];
    int i;

    for (i = 1; i < HEADER_SIZE * NUMBER_OF_HEADERS; i++) {
        if (header[i] == 0) {
            index = i;
            break;
        }
    }

    if (i == HEADER_SIZE * NUMBER_OF_HEADERS) {
        create_new_header(header_ptr, save_fd);
        header = *header_ptr; // Update local pointer after realloc
        NUMBER_OF_HEADERS = header[0];
        index = HEADER_SIZE * (NUMBER_OF_HEADERS - 1);
    }

    if (index == 0) {
        perror("header full");
    }

    return index;
}

void create_new_header(int **header_ptr, int save_fd) {
    int *header = *header_ptr;
    header[0]++;
    int NUMBER_OF_HEADERS = header[0];

    int *temp = realloc(header, HEADER_SIZE * NUMBER_OF_HEADERS * sizeof(int));
    if (!temp) {
        perror("Header Realloc");
        return;
    }

    *header_ptr = temp;

    int old_size = (NUMBER_OF_HEADERS - 1) * HEADER_SIZE;
    memset(*header_ptr + old_size, 0, HEADER_SIZE * sizeof(int));

    int *new_header = *header_ptr + old_size;
    lseek(save_fd, 0, SEEK_END);
    if (write(save_fd, new_header, HEADER_SIZE * sizeof(int)) != HEADER_SIZE * sizeof(int)) {
        perror("Write new header failed");
        return;
    }

}

void append_to_response(char **response, size_t *buffer_size, size_t *response_len, const char *data) {
    size_t data_len = strlen(data);

    if (*response_len + data_len + 1 >= *buffer_size) {
        while (*response_len + data_len + 1 >= *buffer_size) {
            *buffer_size *= 2;
        }
        *response = realloc(*response, *buffer_size);
        if (!*response) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
    }

    strcat(*response, data);
    *response_len += data_len;
}
