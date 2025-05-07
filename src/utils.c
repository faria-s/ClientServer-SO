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

void load_header(int fd, int header[]) {

    if (fd == -1) {
        perror("Error opening file");
        return ;
    }

    for (int i = 0; i < 128; i++) {
        if (read(fd, &header[i], sizeof(int)) != sizeof(int)) {
            perror("Error reading from file");
            return ;
        }
    }
}

int find_empty_index(int header[]){
   int index = 0;
   int i;

   for(i = 1; i < HEADER_SIZE; i++){
       if(header[i] == 0){
           index = i;
           break;
       }
   }

  if(i == HEADER_SIZE){
      perror("header full");
  }

  return index;
}
