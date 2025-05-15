#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "server_helpers.h"

#define PIPE_NAME "/tmp/doc_index_pipe"

int main(int argc, char *argv[]) {

    // ===========================Checking conditions=================================
    if (argc < 2) {
        handle_error("Uso: dclient -<flag> [args...]\n");
    }

    // ===========================Setting variables=================================
    Command cmd;
    build_command(&cmd, argc, argv);

    // ===========================Creating Individual FIFO=================================
    char response_fifo[64];
    snprintf(response_fifo, sizeof(response_fifo), "/tmp/client_%d", cmd.processID);

    unlink(response_fifo);

    if (mkfifo(response_fifo, 0666) == -1) {
        handle_error("Error creating response client FIFO\n");
    }

    // ===========================Opening FIFO=================================
    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        handle_error("Error opening client side FIFO\n");
    }

    // ===========================Sending struct to Server=================================
    if (write(fd, &cmd, sizeof(Command)) != sizeof(Command)) {
        close(fd);
        handle_error("Error writing struct Command to FIFO client side\n");
    }

    close(fd);

    // ===========================Opening FIFO=================================
    int response_fd = open(response_fifo, O_RDONLY);
    if (response_fd == -1) {
        handle_error("Error opening response FIFO client side\n");
    }

    // ===========================Reading Server response=================================
    char response[512];
    ssize_t bytes_read;
    while((bytes_read = read(response_fd, response, sizeof(response) - 1)) > 0){
        if (bytes_read < sizeof(response_fd))
                response[bytes_read] = '\0';
        write(STDOUT_FILENO, response, strlen(response));
    }

    close(response_fd);
    unlink(response_fifo);


    return 0;
}
