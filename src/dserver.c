#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "lib.h"

#define PIPE_NAME "/tmp/doc_index_pipe"

int main() {
    // ===========================Setting variables=================================
    GHashTable *document_table;
    document_table = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);
    int current_id = 1;

    // ===========================Loads Data=================================
    if(access("meta_info.txt", F_OK) == 0){
        handle_load_metadata(document_table,"meta_info.txt");
    }

     // ===========================Avoid Zombies=================================
     signal(SIGCHLD, SIG_IGN);

    // ===========================Creating Main FIFO=================================
    if (access(PIPE_NAME, F_OK) == -1) {
        if (mkfifo(PIPE_NAME, 0666) != 0) {
            handle_error("Error creating main fifo server side\n");
        }
    }

    // ===========================Main Loop to receive queries=================================
    int running = 1;
    while (running) {
        // ===========================Opening FIFO=================================
        int fd = open(PIPE_NAME, O_RDONLY);
        if (fd == -1) {
            handle_error("Error opening server side FIFO\n");
        }

        // ===========================Reading command=================================
        Command cmd;
        ssize_t bytes = read(fd, &cmd, sizeof(Command));
        close(fd);

        if (bytes != sizeof(Command)) {
            handle_error("Error reading from FIFO server side\n");
        }

        // ===========================Creating pipe for father-children communication=================================
        int pfd[2];
        if (pipe(pfd) == -1) {
            handle_error("Error creating anonymous pipe\n");
        }

        // ===========================Creating child process=================================
        pid_t pid = fork();

        if (pid == 0) {
            // ===========================CHILD===========================
            close(pfd[0]);

            // ===========================Checking for modification flag===========================
            if (cmd.flag == 'a' || cmd.flag == 'd') {
                write(pfd[1], &cmd, sizeof(Command));
            } else {
                handle_client_response(&cmd, document_table);
            }

            close(pfd[1]);

            _exit(0);
        } else {
            // ===========================FATHER===========================
            close(pfd[1]);

            // ===========================Checking for modification flag===========================
            if (cmd.flag == 'a' || cmd.flag == 'd') {
                Command received;
                ssize_t r = read(pfd[0], &received, sizeof(Command));
                if (r == sizeof(Command)) {
                    if (received.flag == 'a') {
                        handle_add(&received, document_table, &current_id);
                    } else if (received.flag == 'd') {
                        handle_delete(&received, document_table);
                    }
                } else {
                    handle_error("Error reading from anonymous pipe\n");
                }
            }

            close(pfd[0]);
        }

        // ===========================Checking for server ending flag===========================
        if (cmd.flag == 'f') {
            handle_save_metadata(document_table, "meta_info.txt");
            handle_shutdown(&cmd,document_table);
            printf("Server is shutting down\n");
            g_hash_table_destroy(document_table);
            running = 0;
        }
    }

    unlink(PIPE_NAME);
    return 0;
}
