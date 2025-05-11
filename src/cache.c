#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>


#include "cache.h"
#include "glib.h"
#include "utils.h"

Cache_entry *cache_entry_new(DocumentInfo *doc, Cache_entry *next, Cache_entry *prev, int current_id){
    Cache_entry *new_entry = malloc(sizeof(struct node));
    new_entry->id = current_id;

    new_entry->doc = malloc(sizeof(DocumentInfo));
    memcpy(new_entry->doc, doc, sizeof(DocumentInfo));

    new_entry->next = next;
    new_entry->prev = prev;

    return new_entry;
}

Cache *cache_new(int N){
    Cache *new_cache = malloc(sizeof(struct cache));
    new_cache->size = N;
    new_cache->head = NULL;
    new_cache->tail = NULL;
    new_cache->cache = g_hash_table_new_full(g_int_hash, g_int_equal, free, (GDestroyNotify)cache_entry_free);

    return new_cache;
}

int cache_is_full(Cache *cache){
    return (g_hash_table_size(cache->cache) >= cache->size);
}

int cache_remove(Cache *cache, int index){

    if (!g_hash_table_contains(cache->cache, &index)) {
        return 0;
    }

    Cache_entry *entry = g_hash_table_lookup(cache->cache, &index);

    if (cache->tail == entry) {
        cache->tail = entry->prev;
    }
    if (cache->head == entry) {
        cache->head = entry->next;
    }
    if (entry->prev) {
        entry->prev->next = entry->next;
    }
    if (entry->next) {
        entry->next->prev = entry->prev;
    }

    g_hash_table_remove(cache->cache, &index);

    return 1;
}

void cache_remove_LRU(Cache* cache){
    if (!cache->tail) {
        return; // Cache is empty
    }

    int lru_id = cache->tail->id;
    cache_remove(cache, lru_id);
}

/*void print_cache_entry(Cache_entry *entry){
    if(entry){
        printf("ID: %d\nNext: %d\nPrev: %d\n", entry->id, entry->next,  entry->prev);
    }
    else{
        printf("Empty Entry");
    }
}*/

void cache_put(Cache* cache, DocumentInfo* doc){

    int *id = malloc(sizeof(int));
    *id = doc->id;

    if (g_hash_table_contains(cache->cache, id)) {
        cache_remove(cache, *id);
    }
    // ===========================Check If Cache Has Reached Capacity=================================
    else if(cache_is_full(cache)){
        // ===========================Remove LRU=================================
        cache_remove_LRU(cache);
    }

    // ===========================Create new Entry=================================
    Cache_entry *new_entry = cache_entry_new(doc, cache->head, NULL, doc->id);

    // ===========================Sets Tail If Cache Empty=================================
    if (cache->head) {
        cache->head->prev = new_entry;
    }

    // ===========================Update New Entry Position=================================
    cache->head = new_entry;

    if (!cache->tail) {
        cache->tail = new_entry;
    }

    // ===========================Add New Entry=================================
    g_hash_table_insert(cache->cache, id, new_entry);
}

void cache_set_head(Cache_entry *entry, Cache *cache){
    if (entry == cache->head) return;

    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;

    if (cache->tail == entry) {
        cache->tail = entry->prev;
    }

    entry->next = cache->head;
    entry->prev = NULL;

    if (cache->head){
        cache->head->prev = entry;
    }

    cache->head = entry;

    if (!cache->tail){
        cache->tail = entry;
    }
}

DocumentInfo *cache_get(Cache* cache, int id){
    Cache_entry *entry = g_hash_table_lookup(cache->cache, &id);
    if (entry) {
        cache_set_head(entry, cache);
        return entry->doc;
    }
    return NULL;
}

void cache_free(Cache *cache) {
    if (!cache) {
        return;
    }

    g_hash_table_destroy(cache->cache); // Automatically frees all entries
    free(cache);
}

void cache_entry_free(Cache_entry *entry) {
    if (entry) {
        if(entry->doc){
            free(entry->doc);
        }
        free(entry);
    }
}

/*
* put checks if the node already exists
* update value of the node, updating to the front
* If key does not exist, check if the cache has already reached capacity
* remove the LRU (tail)
* New node front of the Linked list
*
*/

/*
* Getcheck if the algorithm exists
* Use the dictionary to found the correct node
* Move node to front
*/

/*
* HashTable that points at node given Id (never changes pointing at node)
* Only nodes prox change
*/
