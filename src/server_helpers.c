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

void handle_add(Command *cmd, Cache *cache, int *current_id, int save_fd, char* folder_path, int **header_ptr) {
    int* header = *header_ptr;

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

    int index = find_empty_index(header_ptr, save_fd);
    header = *header_ptr;


    // ===========================Creating and Filling Structure=================================
    DocumentInfo *doc = malloc(sizeof(DocumentInfo));

    if(!doc){
        handle_error("Creating Doc");
    }
    // ===========================Initializing the Struct to Zero=================================
    memset(doc, 0, sizeof(DocumentInfo));


    doc->id = index;
    strncpy(doc->title, title, MAX_TITLE);
    strncpy(doc->authors, authors, MAX_AUTHORS);
    strncpy(doc->year, year, MAX_YEAR);
    strncpy(doc->path, path, MAX_PATH);

    // ===========================Verifying If Path Exists=================================
    char response[128];
    if(access(doc->path,F_OK) == 0){
        // ===========================Inserting in Hashtable=================================
        header[index] = index;
        handle_write_on_disk(save_fd, doc,cache,'a', doc->id);

        // ===========================Building Response Message=================================

        snprintf(response, sizeof(response), "Document %d indexed\n", doc->id);


    }
    else{
        snprintf(response, sizeof(response), "Document path doesn't exist\n");
        free(doc);
    }

    // ===========================Setting up FIFO name=================================
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d", cmd->processID);

    // ===========================Opening FIFO=================================
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Error opening response FIFO server side\n");
        free(args);
        free(doc);
        return;
    }

    // ===========================Sending Response to client=================================
    write(fd, response, strlen(response));
    close(fd);
    free(doc);
    free(args);
}


void handle_consult(Command *cmd, Cache *cache, int save_fd, int **header) {

    // ===========================Getting key from the command=================================
    int key = atoi(cmd->arguments);

    DocumentInfo *doc = NULL;
    DocumentInfo *no_cache = NULL;
    if(key > 0){
        // ===========================Searching the Hashtable=================================
        no_cache = handle_file_exists(cache, save_fd, key, *header);
        doc = (cache->size == 0) ? no_cache : cache_get(cache,key);
    }

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

    if(cache->size == 0) free(doc);
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
    int removed = 0;

    if(key > 0){
        removed = handle_write_on_disk(saved_fd, NULL, cache, 'd', key);
    }

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
        DocumentInfo *doc = NULL;

        if(key > 0){
            DocumentInfo *no_cache = handle_file_exists(cache, save_fd, key, header);
            doc = (cache->size == 0) ? no_cache : cache_get(cache,key);
        }

        if (!doc) {
            snprintf(response, sizeof(response), "Couldn't find document with ID %d\n", key);
        } else {

            // ===========================Creating Pipe=================================
            int pfd[2];
            if (pipe(pfd) == -1) {
                perror("pipe failed");
            }

            // ===========================Creating Child Process To Execute grep=================================
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                close(pfd[0]);
                close(pfd[1]);
            }

            if (pid == 0) {
                // ===========================CHILD=================================
                close(pfd[0]);
                // ===========================STDOUT Poiting At Writing Pipe=================================
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);

                // ===========================Executing Grep=================================
                execlp("grep", "grep", "-c", keyword, doc->path, NULL);

                free(doc);
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

                if(cache->size == 0) free(doc);
                // ===========================Verifies If Grep Output Is Empty=================================
                if (count > 0) {
                    // ===========================Adds Document ID To Response=================================
                    snprintf(response, sizeof(response), "%d\n", atoi(buffer));
                }
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

void handle_search(Command *cmd,Cache *cache, int save_fd, int header[]) {
    char *args = strdup(cmd->arguments);
    size_t buffer_size = 3000;
    char *response = malloc(buffer_size);

    if (!response) {
        free(args);
        handle_error("malloc failed");
    }

    response[0] = '\0';
    size_t response_len = 0;

    int NUMBER_OF_HEADERS = header[0];
    int NUMBER_OF_FILES = HEADER_SIZE*NUMBER_OF_HEADERS;

    // ===========================Verifies If Has nr_processes=================================
    if (cmd->number_arguments == 3) {
        char *keyword = args;

        strcat(response, "[");

        // ===========================Gets Number of Indexed Documents=================================

        int first = 1;

        // ===========================Verifies Every Document=================================

        for (int i = 1; i < NUMBER_OF_FILES; i++) {
            DocumentInfo *no_cache = handle_file_exists(cache, save_fd, i, header);
            DocumentInfo *doc = (cache->size == 0) ? no_cache : cache_get(cache,i);


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
                    buffer[count] = '\0';

                    // ===========================Adds Document ID To Response=================================
                    if (!first) {
                        append_to_response(&response, &buffer_size, &response_len, ",");
                    }

                    char id_str[16];
                    snprintf(id_str, sizeof(id_str), "%d", i);
                    append_to_response(&response, &buffer_size, &response_len, id_str);

                    first = 0;
                }
            }

            if(cache->size == 0) free(doc);
        }

        append_to_response(&response, &buffer_size, &response_len, "]\n");
    }
    else if(cmd->number_arguments == 4){
        // ===========================Getting arguments from the command=================================
        char *keyword = strtok(args, "|");
        char *nr_processes= strtok(NULL, "|");

        // ===========================Getting key from the command=================================

        int NUMBER_PROCESSES= atoi(nr_processes);
        if(NUMBER_PROCESSES <= 0){

            perror("Invalid nr_processes input\n");
            free(args);
            return;
        }


        int files_per_process = NUMBER_OF_FILES / NUMBER_PROCESSES;
        int remaining_files = NUMBER_OF_FILES % NUMBER_PROCESSES;

        strcat(response, "[");
        int first = 1;

        // =========================== Creates Pipe For Each Process ============================
        int pipes[NUMBER_PROCESSES][2];
        for (int i = 0; i < NUMBER_PROCESSES; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe failed");
                free(args);
                return;
            }
        }

        // =========================== Creates Fork For Each Process ============================
        for (int i = 0; i < NUMBER_PROCESSES; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                free(args);
                return;
            }

            if (pid == 0) {
                // =========================== CHILD PROCESS ============================
                close(pipes[i][0]);

                int start = i * files_per_process;
                int end = start + files_per_process;
                if (i == NUMBER_PROCESSES- 1) {
                    end += remaining_files;
                }

                char child_response[3000];
                child_response[0] = '\0';
                int child_first = 1;

                if(start == 0) start = 1;
                for (int j = start; j < end; j++) {
                    DocumentInfo *no_cache = handle_file_exists(cache, save_fd, j, header);
                    DocumentInfo *doc = (cache->size == 0) ? no_cache : cache_get(cache,j);

                    if (!doc){
                        continue;
                    }

                    int pfd[2];
                    if (pipe(pfd) == -1) {
                        perror("pipe failed");
                        continue;
                    }

                    pid_t grep_pid = fork();
                    if (grep_pid == -1) {
                        perror("fork failed");
                        close(pfd[0]);
                        close(pfd[1]);
                        continue;
                    }

                    int temp_fd = open("tmp/error.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (temp_fd == -1) {
                        handle_error("open tmp file");
                    }

                    if (grep_pid == 0) {
                        // =========================== GREP CHILD ============================
                        close(pfd[0]);
                        dup2(pfd[1], STDOUT_FILENO);
                        close(pfd[1]);
                        dup2(temp_fd, STDERR_FILENO);
                        close(temp_fd);

                        execlp("grep", "grep", keyword, doc->path, NULL);

                        perror("execlp failed");
                        _exit(1);
                    } else {
                        // =========================== GREP PARENT ============================
                        close(pfd[1]);

                        // =========================== Reads Child Response ============================
                        char buffer[3000];
                        ssize_t count = read(pfd[0], buffer, sizeof(buffer) - 1);
                        close(pfd[0]);

                        // =========================== Waits for Especif Child Process Death ============================
                        waitpid(grep_pid, NULL, 0);

                        if (count > 0) {
                            buffer[count] = '\0';

                            if (!child_first) {
                                strncat(child_response, ",", sizeof(child_response) - strlen(child_response) - 1);
                            }

                            char id_str[16];
                            snprintf(id_str, sizeof(id_str), "%d", j);
                            strncat(child_response, id_str, sizeof(child_response) - strlen(child_response) - 1);

                            child_first = 0;
                        }
                    }

                    if (cache->size == 0) free(doc);
                }

                write(pipes[i][1], child_response, strlen(child_response));
                close(pipes[i][1]);
                _exit(0);
            }
        }

        for (int i = 0; i < NUMBER_PROCESSES; i++) {
            close(pipes[i][1]);
        }

        for (int i = 0; i < NUMBER_PROCESSES; i++) {
            char buffer[3000];
            ssize_t count = read(pipes[i][0], buffer, sizeof(buffer) - 1);
            close(pipes[i][0]);

            if (count > 0) {
                buffer[count] = '\0';

                if (!first) {
                    append_to_response(&response, &buffer_size, &response_len, ",");
                }

                append_to_response(&response, &buffer_size, &response_len, buffer);
                first = 0;
            }

            wait(NULL);
        }

        append_to_response(&response, &buffer_size, &response_len, "]\n");
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
    free(response);
    free(args);
}


void handle_shutdown(Command *cmd, Cache *cache, int *header) {

    cache_free(cache);
    free(header);


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

    close(fd);
}

void handle_cache(Command *cmd, Cache *cache){
    if (!cache) {
        printf("Cache is NULL.\n");
        return;
    }

    Cache_entry *current = cache->head;
    if (!current) {
        printf("Cache is empty.\n");
    }

    // ===========================Building Response=================================
    char response[1024];
    response[0] = '\0';

    snprintf(response, sizeof(response), "Cache Size: %d\n", cache->size);

    while (current) {
        char entry[128];
        snprintf(entry, sizeof(entry), "ID: %d\n", current->id);
        strncat(response, entry, sizeof(response) - strlen(response) - 1);
        current = current->next;
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
    if (write(fd, response, strlen(response)) == -1) {
        perror("Error writing response to FIFO server side\n");
    }

    close(fd);
}

void handle_client_response(Command *cmd, Cache* cache, int save_fd, int* current_id, char* path, int **header_ptr){
    switch (cmd->flag)
    {
    case 'a':
        handle_add(cmd, cache, current_id,save_fd,path,header_ptr);
        break;
    case 'd':
        handle_delete(cmd, cache, save_fd, *header_ptr);
        break;
    case 'c':
        handle_consult(cmd, cache,save_fd, header_ptr);
        break;
    case 'l':
        handle_lines_with_keyword(cmd, cache, save_fd, *header_ptr);
        break;
   case 's':
        handle_search(cmd, cache,save_fd, *header_ptr);
        break;
    case 'p':
        handle_cache(cmd,cache);

        break;
    default:
        break;
    }
}

int handle_write_on_disk(int fd, DocumentInfo *doc, Cache *cache, char cmd, int id){
    int writed = 0;

    if (fd == -1) {

        handle_error("Invalid file descriptor for meta_info.txt");
    }

    int NUMBER_OF_HEADERS;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        handle_error("Failed to seek to the beginning of the file");
    }
    if (read(fd, &NUMBER_OF_HEADERS, sizeof(int)) != sizeof(int)) {
        handle_error("Failed to read the number of headers");
    }

    int header_index = id / HEADER_SIZE;
    int header_offset = id % HEADER_SIZE;

    if (header_index >= NUMBER_OF_HEADERS) {
        NUMBER_OF_HEADERS++;
        if (lseek(fd, 0, SEEK_SET) == -1) {
            handle_error("Failed to seek to the beginning of the file to update NUMBER_OF_HEADERS");
        }
        if (write(fd, &NUMBER_OF_HEADERS, sizeof(int)) != sizeof(int)) {
            handle_error("Failed to update the number of headers");
        }

        int zero = 0;
        if (lseek(fd, header_index * (HEADER_SIZE * sizeof(int) + HEADER_SIZE * sizeof(DocumentInfo)), SEEK_END) == -1) {
            handle_error("Failed to seek to the end of the file to add a new header");
        }
        for (int i = 0; i < HEADER_SIZE; i++) {
            if (write(fd, &zero, sizeof(int)) != sizeof(int)) {
                handle_error("Failed to initialize new header block");
            }
        }
    }

    int search_header = header_index * (HEADER_SIZE * sizeof(int) + HEADER_SIZE * sizeof(DocumentInfo)) +
                        header_offset * sizeof(int);

    if (lseek(fd, search_header, SEEK_SET) == -1) {
        handle_error("Failed to seek to header position");
    }

    if (cmd == 'a') {
        if (write(fd, &id, sizeof(int)) != sizeof(int)) {
            handle_error("Failed to write ID to header");
        }

        int search_doc_struct = header_index * (HEADER_SIZE * sizeof(int) + HEADER_SIZE * sizeof(DocumentInfo)) +
                                (HEADER_SIZE * sizeof(int)) + (header_offset * sizeof(DocumentInfo));

        if (lseek(fd, search_doc_struct, SEEK_SET) == -1) {
            handle_error("Failed to seek to document metadata position");
        }

        if (write(fd, doc, sizeof(DocumentInfo)) != sizeof(DocumentInfo)) {
            handle_error("Failed to write DocumentInfo to disk");

        }

        cache_put(cache, doc);
        writed = 1;

    } else if (cmd == 'd') {
        int zero = 0;
        if (write(fd, &zero, sizeof(int)) != sizeof(int)) {
            handle_error("Failed to write zero to header for deletion");
        }

        cache_remove(cache, id);

        writed = 1;
    }

    return writed;

}

DocumentInfo *handle_file_exists(Cache *cache, int fd, int index, int header[]) {


    DocumentInfo *doc= cache_get(cache, index);
    if(!doc){
        int header_index = index / HEADER_SIZE;
        int header_offset = index % HEADER_SIZE;

        int header_position = header_index * (HEADER_SIZE * sizeof(int) + HEADER_SIZE * sizeof(DocumentInfo));

        if (lseek(fd, header_position + header_offset * sizeof(int), SEEK_SET) == -1) {
            perror("Error seeking to header position");
            return NULL;
        }

        int isIndexed;
        if (read(fd, &isIndexed, sizeof(int)) != sizeof(int)) {
            perror("Error reading isIndexed value");
            return NULL;
        }

        if (isIndexed > 0) {
            doc = malloc(sizeof(DocumentInfo));

            if (!doc) {
                handle_error("Failed to allocate memory for DocumentInfo");
            }


            off_t doc_position = header_position + (HEADER_SIZE * sizeof(int)) + (header_offset * sizeof(DocumentInfo));

            if (lseek(fd, doc_position, SEEK_SET) == -1) {
                perror("Error seeking to document position");
                free(doc);

                return NULL;

            }


            if (read(fd, doc, sizeof(DocumentInfo)) == sizeof(DocumentInfo)) {
                if(cache->size == 0) return doc;
                cache_put(cache, doc);
                free(doc);
            } else {

                perror("Error reading document metadata");

                free(doc);
            }
        }
    }

    return doc;
}
