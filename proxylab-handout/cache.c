  /*
   *  Author: Hana Wahi-Anwar 
   *  Email:  HQW5245@psu.edu
   *
   */

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "csapp.h"
  #include "cache.h"





  /* cache_init initializes the input cache linked list. */
  void cache_init(CacheList *list) {
  /* Set size to 0; first and last nodes to NULL. */
    list->size = 0;
    list->first = NULL;
    list->last = NULL;

}

  /* cache_URL adds a new cached item to the linked list. It takes the
   * URL being cached, a link to the content, the size of the content, and
   * the linked list being used. It creates a struct holding the metadata
   * and adds it at the beginning of the linked list.
   */
  void cache_URL(const char *URL, const char *headers, void *item, size_t size, CacheList *list) {
    /* Check to see if size of item exceeds MAX_OBJECT_SIZE: */
    if (size > MAX_OBJECT_SIZE) {
      /* Free pointer item and DO NOT allocate node: */
      free(item);
      return;
    }
    /* Check to see if we need to evict: */
    /* WHILE (cache size + file size) > max cache size: */
    while ( ((list->size) + size) > MAX_CACHE_SIZE ) {
      /* Evict; continue evicting until there is enough space. */
      /* How many to evict? */
      if (list->last) {
        if (list->last->prev) {
          /* Set prev to last to point to NULL. */
          list->last->prev->next = NULL;
        }
        /* Temporary node to hold new last node: */
        CachedItem *newLastNode = list->last->prev;
        /* Free url, headers, and item from last node: */
        free(list->last->url);
        free(list->last->headers);
        free(list->last->item_p);
        /* Update size of cache: */
        list->size = (list->size) - (list->last->size);
        /* Free last node: */
        free(list->last);
        list->last = newLastNode;
        if (list->last == NULL) {
          list->first = NULL;
        }

      }

    }

    /* Add to cache (given there is now enough space): */
    if ( ((list->size) + size) <= MAX_CACHE_SIZE ) {
      /* Add new node to front of list: */
      CachedItem *node = malloc(sizeof(CachedItem));
      node->next = list->first;
      node->prev = NULL;
      /* Store URL and pointers: */
      node->url = strdup(URL);
      node->headers = strdup(headers);
      node->item_p = item;
      node->size = size;
      /* Update overall size of cache: */
      list->size = list->size + size;

        /* If cache is empty: */
      if (list->last == NULL) {
        list->last = node;
      }
        /* If cache is not empty: */
      else {
        list->first->prev = node;
      }
      list->first = node;
    }

}


  /* find iterates through the linked list and returns a pointer to the
   * struct associated with the requested URL. If the requested URL is
   * not cached, it returns null.
   */
  CachedItem *find(const char *URL, CacheList *list) {
    /* If cache is empty, return NULL: */
    if (list->first == NULL) {
      return NULL;
    }

    /* Temporary iterator node: */
    CachedItem *iterNode = list->first;

    /* Step through every node: */
    /* Start from front: */
    while(iterNode->next != NULL) {
      /* If a match is found: */
      if (strcasecmp(iterNode->url, URL) == 0) {
        /* Move node to front of list: */

        /* CASE 1: If already at front: */
        if(iterNode->prev == NULL) {
          return iterNode;
        }
        /* CASE 2: If node is in the middle: */
        iterNode->prev->next = iterNode->next;
        iterNode->next->prev = iterNode->prev;
        iterNode->next = list->first;
        iterNode->prev = NULL;
        list->first = iterNode;
        return iterNode;
      }
      /* Increment iterator. */
      iterNode = iterNode->next;
    }
    /* At the end of this loop, iterNode->next == NULL. */
    /* We are at the last node; check it: */
    if (strcasecmp(iterNode->url, URL) == 0) {
      /* CASE 3: If node is at the end: */
      iterNode->prev->next = NULL;
      list->last = iterNode->prev;
      iterNode->next = list->first;
      iterNode->prev = NULL;
      list->first = iterNode;
      return iterNode;
    }
    return NULL;
}


  /* frees the memory used to store each cached object, and frees the struct
   * used to store its metadata. */
  void cache_destruct(CacheList *list) {

    /* Go through each item in the cache: */
    while(list->first != NULL) {
      /* Delete from the back of the list. */
      if (list->last) {
        if (list->last->prev) {
          /* Set prev to last to point to NULL. */
          list->last->prev->next = NULL;
        }
        /* Temporary node to hold new last node: */
        CachedItem *newLastNode = list->last->prev;
        /* Free url, headers, and item from last node: */
        free(list->last->url);
        free(list->last->headers);
        free(list->last->item_p);
        /* Update size of cache: */
        list->size = (list->size) - (list->last->size);
        /* Free last node: */
        free(list->last);
        list->last = newLastNode;
        if (list->last == NULL) {
          list->first = NULL;
        }
      }

    }//END while loop.
    /* After leaving this loop, list->first should = NULL. */


}
