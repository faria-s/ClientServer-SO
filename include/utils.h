/**
 * @file utils.h
 * @brief Header file containing utility functions and structures for document handling and header management.
 */

#ifndef UTILS_H
#define UTILS_H

#include <glib.h>

#define MAX_TITLE 200       /**< Maximum size for the document's title. */
#define MAX_AUTHORS 200     /**< Maximum size for the document's authors. */
#define MAX_YEAR 5          /**< Maximum size for the document's publication year. */
#define MAX_PATH 64         /**< Maximum size for the document's file path. */
#define HEADER_SIZE 512     /**< Number of slots in each header segment. */

/**
 * @struct DocumentInfo
 * @brief Represents a document with its metadata.
 */
typedef struct {
    int id;                          /**< Unique identifier for the document. */
    char title[MAX_TITLE];           /**< Title of the document. */
    char authors[MAX_AUTHORS];       /**< Authors of the document. */
    char year[MAX_YEAR];             /**< Publication year of the document. */
    char path[MAX_PATH];             /**< File path where the document is stored. */
} DocumentInfo;

/**
 * @brief Finds a document in the cache by its unique ID.
 *
 * @param cache The cache where documents are stored.
 * @param id The unique identifier of the document to find.
 * @return DocumentInfo* Pointer to the document if found, NULL otherwise.
 */
DocumentInfo* find_document_by_id(GHashTable *cache, int id);

/**
 * @brief Prints the metadata of a document.
 *
 * @param doc The document to print.
 */
void print_doc(DocumentInfo *doc);

/**
 * @brief Creates and initializes a save file with a header.
 *
 * @param path The path of the save file.
 * @param header The header data to write to the file.
 * @return int The file descriptor of the created file, or -1 on failure.
 */
int create_save_file(char* path, int header[]);

/**
 * @brief Loads header information from a file.
 *
 * @param fd The file descriptor of the save file.
 * @param header The array to load the header information into.
 * @param NUMBER_OF_HEADERS The number of header blocks to load.
 */
void load_header(int fd, int header[], int NUMBER_OF_HEADERS);

/**
 * @brief Finds the next available empty index in the header.
 *
 * @param header_ptr Pointer to the header array.
 * @param save_fd The file descriptor of the save file.
 * @return int The index of the next available slot.
 */
int find_empty_index(int **header_ptr, int save_fd);

/**
 * @brief Creates a new header block and updates the file.
 *
 * @param header_ptr Pointer to the header array.
 * @param save_fd The file descriptor of the save file.
 */
void create_new_header(int **header_ptr, int save_fd);


#endif
