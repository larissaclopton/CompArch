/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "shell.h"
#include "pipe.h"

typedef struct node{
  int index;
  struct node* next;
}node;

typedef struct{
  node* head;
  node* tail;
}queue;

typedef struct{
        unsigned int valid;
        unsigned long long tag;
        uint8_t *block;
}line;


typedef struct{
        line* lines;
        queue* lru;
}set;

typedef struct
{
  set* sets;
} cache_t;

extern int inst_hit;
extern int inst_empty;
extern int data_hit;
extern int data_empty;

cache_t *cache_new(int sets, int ways, int block);
void cache_destroy(cache_t *c, int num_sets, int ways);
void inst_handle_miss(cache_t *c, uint64_t addr);
void data_handle_miss(cache_t *c, uint64_t addr);
uint32_t inst_cache_update(cache_t *c, uint64_t addr);
// type == 0 -> store, type == 1 -> load.
uint64_t data_cache_update(cache_t *c, uint64_t addr, int type, int size, uint64_t val);

#endif
