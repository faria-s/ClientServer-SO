/**
 * @file server_helpers.h
 * @brief Header file containing server-side command handlers and utility functions.
 */

#ifndef SERVER_HELPERS_H
#define SERVER_HELPERS_H

#include <glib.h>
#include "utils.h"
#include "cache.h"

/**
 * @def PIPE_NAME
 * The named pipe for client-server communication.
 */
#define PIPE_NAME "/tmp/doc_index_pipe"

/**
 * @def DISK_PATH
 * Path to the saved metadata information file.
 */
#define DISK_PATH "meta_info.txt"

/**
 * @brief Prints an error message and exits the application.
 *
 * @param message The error message to display.
 */
void handle_error(char *message);

/**
 * @brief Builds a Command structure from command-line arguments.
 *
 * @param cmd Pointer to the Command structure to populate.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 */
void build_command(Command *cmd, int argc, char *argv[]);


/**
 * @brief Handles the addition of a new document to the cache and storage.
 *
 * @param cmd Command structure containing the document details.
 * @param cache Cache structure to store the document.
 * @param current_id Pointer to the current document ID.
 * @param save_fd File descriptor for metadata storage.
 * @param folder_path Path to the storage folder.
 * @param header_ptr Pointer to the header array.
 * @param pfd pipe to father.
 */
void handle_add(Command *cmd, Cache *cache, int *current_id, int save_fd, char* folder_path, int **header_ptr, int pfd[]);

/**
 * @brief Handles the consultation of a document by its ID.
 *
 * @param cmd Command structure containing the document ID.
 * @param cache Cache structure where documents are indexed.
 * @param save_fd File descriptor for metadata storage.
 * @param header Pointer to the header array.
 * @param pfd pipe to father.
 */
void handle_consult(Command *cmd, Cache *cache, int save_fd, int **header, int pfd[]);

/**
 * @brief Handles the deletion of a document by its ID.
 *
 * @param cmd Command structure containing the document ID.
 * @param cache Cache structure where documents are indexed.
 * @param saved_fd File descriptor for metadata storage.
 * @param header Header array for index tracking.
 */
void handle_delete(Command *cmd, Cache *cache, int saved_fd, int header[]);

/**
 * @brief Handles the search for lines containing a specific keyword in a document.
 *
 * @param cmd Command structure containing the document ID and keyword.
 * @param cache Cache structure where documents are indexed.
 * @param fd File descriptor for metadata storage.
 * @param header Header array for index tracking.
 */
void handle_lines_with_keyword(Command *cmd, Cache *cache, int fd, int header[], int pfd[]);

/**
 * @brief Routes client commands to the appropriate handler based on the command flag.
 *
 * @param cmd Command structure containing the client request.
 * @param cache Cache structure where documents are indexed.
 * @param save_fd File descriptor for metadata storage.
 * @param current_id Pointer to the current document ID.
 * @param path Path to the storage folder.
 * @param header_ptr Pointer to the header array.
 * @param pfd pipe to father.
 */
void handle_client_response(Command *cmd, Cache* cache, int save_fd, int* current_id, char* path, int **header_ptr, int pfd[]);

/**
 * @brief Handles the server shutdown process, freeing memory and resources.
 *
 * @param cmd Command structure with the shutdown request.
 * @param cache Cache structure where documents are indexed.
 * @param header Header array for index tracking.
 */
void handle_shutdown(Command *cmd, Cache *cache, int *header);

/**
 * @brief Handles the search of documents by keyword with parallel processing.
 *
 * @param cmd Command structure containing the search keyword.
 * @param cache Cache structure where documents are indexed.
 * @param save_fd File descriptor for metadata storage.
 * @param header Header array for index tracking.
 */
void handle_search(Command *cmd, Cache *cache, int save_fd, int header[]);

/**
 * @brief Writes document information to disk or handles deletion based on command.
 *
 * @param fd File descriptor for metadata storage.
 * @param doc Document structure to write or delete.
 * @param cache Cache structure where documents are indexed.
 * @param cmd Command flag ('a' for add, 'd' for delete).
 * @param id Document ID for storage or deletion.
 * @return int 1 if the operation was successful, 0 otherwise.
 */

int handle_write_on_disk(int fd, DocumentInfo *doc, Cache *cache, char cmd, int id);

/**
 * @brief Checks if a document exists in cache or on disk.
 *
 * @param cache Cache structure where documents are indexed.
 * @param fd File descriptor for metadata storage.
 * @param index Document index to search for.
 * @param header Header array for index tracking.
 * @return doc if found or NULL if not found.
 */
DocumentInfo *handle_file_exists(Cache *cache, int fd, int index, int header[]);

#endif
