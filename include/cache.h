/**
* @file cache.h
* @brief Header file for cache management using LRU (Least Recently Used) policy.
*/
#ifndef CACHE_H
#define CACHE_H

#include "glib.h"
#include "utils.h"

/**
* @struct Cache_entry
* @brief Represents an entry in the cache's doubly linked list.
*/
typedef struct node {
    int id;                    // Unique identifier for the cached document. */
    DocumentInfo *doc;         // Pointer to the document information. */
    struct node *next;         // Pointer to the next cache entry in the list. */
    struct node *prev;         // Pointer to the previous cache entry in the list. */
} Cache_entry;

/**
* @struct Cache
* @brief Represents the cache structure with LRU eviction policy.
*/
typedef struct cache {
    int size;                  // Maximum size of the cache. */
    Cache_entry *head;         // Head of the doubly linked list (most recently used). */
    Cache_entry *tail;         // Tail of the doubly linked list (least recently used). */
    GHashTable *cache;         // Hash table for quick access to cache entries. */
} Cache;

/**
* @brief Creates a new cache entry node.
*
* @param doc Pointer to the document information.
* @param next Pointer to the next node in the list.
* @param prev Pointer to the previous node in the list.
* @param current_id Unique identifier for the document.
* @return Cache_entry* Pointer to the newly created cache entry.
*/
Cache_entry *cache_entry_new(DocumentInfo *doc, Cache_entry *next, Cache_entry *prev, int current_id);

/**
* @brief Initializes a new cache with a specified maximum size.
*
* @param N Maximum size of the cache.
* @return Cache* Pointer to the newly created cache.
*/
Cache *cache_new(int N);

/**
* @brief Checks if the cache is full.
*
* @param cache Pointer to the cache.
* @return int 1 if the cache is full, 0 otherwise.
*/
int cache_is_full(Cache *cache);

/**
* @brief Removes a cache entry by its ID.
*
* @param cache Pointer to the cache.
* @param index The ID of the cache entry to remove.
* @return int 1 if the entry was successfully removed, 0 if not found.
*/
int cache_remove(Cache *cache, int index);

/**
* @brief Removes the least recently used (LRU) entry from the cache.
*
* @param cache Pointer to the cache.
*/
void cache_remove_LRU(Cache *cache);

/**
* @brief Stores a document in the cache. If the cache is full, it removes the LRU entry.
*
* @param cache Pointer to the cache.
* @param doc Pointer to the document information to store.
*/
void cache_put(Cache *cache, DocumentInfo* doc);

/**
* @brief Moves a cache entry to the head of the doubly linked list (most recently used).
*
* @param entry Pointer to the cache entry.
* @param cache Pointer to the cache.
*/
void cache_set_head(Cache_entry *entry, Cache *cache);

/**
* @brief Retrieves a document from the cache by its ID.
*
* @param cache Pointer to the cache.
* @param id The ID of the document to retrieve.
* @return DocumentInfo* Pointer to the document information if found, NULL otherwise.
*/
DocumentInfo *cache_get(Cache *cache, int id);

/**
* @brief Frees the memory allocated for the cache and its entries.
*
* @param cache Pointer to the cache.
*/
void cache_free(Cache *cache);

/**
* @brief Frees the memory allocated for a single cache entry.
*
* @param entry Pointer to the cache entry to free.
*/
void cache_entry_free(Cache_entry *entry);

void cache_entry_free(Cache_entry *entry);

#endif
