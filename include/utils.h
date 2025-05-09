#ifndef UTILS_H
#define UTILS_H

#include <glib.h>

#define MAX_TITLE 200
#define MAX_AUTHORS 200
#define MAX_YEAR 5
#define MAX_PATH 64

#define HEADER_SIZE 512

typedef struct {
    int id;
    char title[MAX_TITLE];
    char authors[MAX_AUTHORS];
    char year[MAX_YEAR];
    char path[MAX_PATH];
} DocumentInfo;

DocumentInfo* find_document_by_id(GHashTable *cache, int id);

void print_doc(DocumentInfo *doc);

int create_save_file(char* path, int header[]);

void load_header(int fd, int header[]);

int find_empty_index(int header[]);

#endif
