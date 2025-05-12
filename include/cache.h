#ifndef CACHE_H
#define CACHE_H

#include "glib.h"
#include "utils.h"

typedef struct node{
    int id;
    DocumentInfo *doc;
    struct node* next;
    struct node* prev;
}Cache_entry;

typedef struct cache{
    int size;
    Cache_entry *head;
    Cache_entry *tail;
    GHashTable *cache;
}Cache;

Cache_entry *cache_entry_new(DocumentInfo *doc, Cache_entry *next, Cache_entry *prev, int current_id);

Cache *cache_new(int N);

int cache_is_full(Cache *cache);

int cache_remove(Cache *cache, int index);

void cache_remove_LRU(Cache* cache);

void disk_put(DocumentInfo* doc, int id);

DocumentInfo *disk_get(int id);

void cache_put(Cache* cache, DocumentInfo* doc);

DocumentInfo *cache_get(Cache* cache, int id);

void cache_free(Cache *cache);

void cache_entry_free(Cache_entry *entry);

void cache_set_head(Cache_entry *entry, Cache *cache);

#endif
