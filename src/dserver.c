#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>

#include "server_helpers.h"
#include "cache.h"
#include "utils.h"


int main(int argc, char *argv[]) {
    // ===========================Verifying Input=================================
    if(argc != 3 || (access(argv[1],F_OK) == -1) || (atoi(argv[2]) == 0)){
        handle_error("Invalid Server Start Input\n");
    }
    char *path = strdup(argv[1]);
    int cache_size = atoi(argv[2]);

    int current_id = 1;
    int save_fd = -1;
    // ===========================Setting Cache=================================
    Cache *cache = cache_new(cache_size);

    // ===========================Setting Cache=================================
    int NUMBER_OF_HEADERS;
    int* header;

    // ===========================Opens Saved Data File=================================
    if(access(DISK_PATH, F_OK) == 0){
        save_fd = open(DISK_PATH, O_RDWR, 0644);

        if(save_fd == -1){
            handle_error("Opening save file");
        }

        if(read(save_fd, &NUMBER_OF_HEADERS, sizeof(int)) != sizeof(int)){
            handle_error("reading number of headers while loading header");
        }

        header = calloc(HEADER_SIZE*NUMBER_OF_HEADERS, sizeof(int));
        load_header(save_fd, header, NUMBER_OF_HEADERS);
    }
    else{
        NUMBER_OF_HEADERS = 1;
        header = calloc(HEADER_SIZE,sizeof(int));
        header[0] = 1;
        save_fd = create_save_file(DISK_PATH, header);
    }
    if (save_fd == -1) {
        handle_error("opening metadata file for writing");
    }


    // ===========================Avoid Zombies=================================

     signal(SIGCHLD, SIG_IGN);

    // ===========================Creating Main FIFO=================================
    if (access(PIPE_NAME, F_OK) == -1) {
        if (mkfifo(PIPE_NAME, 0666) != 0) {
            handle_error("creating main fifo server side\n");
        }
    }

    // ===========================Main Loop to receive queries=================================
    int running = 1;
    while (running) {
        // ===========================Opening FIFO=================================
        int fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            handle_error("opening server side FIFO\n");
        }

        // ===========================Reading command=================================
        Command cmd;
        ssize_t bytes = read(fd, &cmd, sizeof(Command));
        close(fd);

        if (bytes != sizeof(Command)) {
            handle_error("reading from FIFO server side\n");
        }

        // ===========================Creating pipe for father-children communication=================================
        int pfd[2];
        if (pipe(pfd) == -1) {
            handle_error("creating anonymous pipe\n");
        }

        // ===========================Creating child process=================================
        pid_t pid = fork();

        if (pid == 0) {
            // ===========================CHILD===========================
            close(pfd[0]);

            // ===========================Checking for modification flag===========================

            write(pfd[1], &cmd, sizeof(Command));

            close(pfd[1]);

            _exit(0);
        } else {
            // ===========================FATHER===========================
            close(pfd[1]);

            // ===========================Checking for modification flag===========================
            if (cmd.flag == 'a' || cmd.flag == 'd' || cmd.flag == 'c' || cmd.flag == 'l' || cmd.flag == 's' ||cmd.flag == 'p') {
                Command received;
                ssize_t r = read(pfd[0], &received, sizeof(Command));
                if (r == sizeof(Command)) {
                    handle_client_response(&received, cache, save_fd,&current_id, path, &header);
                } else {
                    handle_error("reading from anonymous pipe\n");
                }
            }

            close(pfd[0]);
        }

        // ===========================Checking for server ending flag===========================
        if (cmd.flag == 'f') {

            handle_shutdown(&cmd, cache);

            printf("Server is shutting down\n");
            running = 0;
        }
    }

    if(save_fd){
        close(save_fd);
    }
    free(path);
    unlink(PIPE_NAME);
    return 0;
}
