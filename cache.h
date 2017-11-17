/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include <stdlib.h>
#include <stdio.h>

int inst_hit;
int data_hit;

void free_queue(queue *q){

  node *tmp;

  while(q->head != NULL){
    tmp = q->head;
    q->head = q->head->next;
    free(tmp);
  }

  free(q);
}

int LRU_index(queue *q) {

  node *tmp = q->head;
  q->head = q->head->next;
  tmp->next = NULL;
  q->tail->next = tmp;
  return tmp->index;

}

void enqueue(queue *q, int index) {

  node *new_node = (node*)malloc(sizeof(node));
  new_node->index = index;
  new_node->next = NULL;
  if (q->head == NULL) {
    q->head = new_node;
    q->tail = new_node;
  } else {
    q->tail->next = new_node;
    q->tail = new_node;
  }

}

void shift(queue *q, int index) {

  node *tmp;
  node *prev;
  for(tmp = q->head; tmp->next != NULL; tmp = tmp->next) {
    if (tmp->index == index) {
      break;
    }
    prev = tmp;
  }

  if (tmp == q->tail) {
    return;
  }
  else if (tmp == q->head) {
    LRU_index(q);
  }
  else {
    prev->next = tmp->next;
    tmp->next = NULL;
    q->tail->next = tmp;
    q->tail = tmp;
  }

}


// create block & initialize values
line create_line(int block){
  line new_line;
  new_line.valid = 0;
  new_line.tag = 0;
  new_line.block = (uint8_t*)malloc(sizeof(uint8_t)*block);
  memset(&new_line.block, 0, sizeof(new_line.block));
  return new_line;
}


// create set & fill with blocks
set create_set(unsigned int ways, int block){
  int i;
  set new_set;
  new_set.lru = (queue*)malloc(sizeof(queue));
  new_set.lru->head = NULL;
  new_set.lru->tail = NULL;
  line *set_lines = (line*)malloc(sizeof(line)*ways);
	new_set.lines = set_lines;
  for(i = 0; i < ways; i++){
    new_set.lines[i] = create_line(block);
  }

  return new_set;
}

cache_t *cache_new(int sets, int ways, int block)
{
  int i;
	cache_t *new_cache = (cache_t*)malloc(sizeof(cache_t));
	set *cache_sets = (set*)malloc(sizeof(set)*sets);
	new_cache->sets = cache_sets;
	for(i = 0; i < sets; i++){
		new_cache->sets[i] = create_set(ways, block);
	}
	new_cache->sets = cache_sets;
	return new_cache;
}

void cache_destroy(cache_t *c)
{
  int i, j, k;
  int num_sets = sizeof(c->sets) / sizeof(c->sets[0]);
  int ways = sizeof(c->sets->lines) / sizeof(c->sets->lines[0]);

  for(i = 0; i < num_sets; i++){
    for(j = 0; j < ways; j++){
      free(c->sets[i].lines[j].block);
    }
    free(c->sets[i].lines);
    free_queue(c->sets[i].lru);
  }
  free(c->sets);
}


void fill_block(uint8_t *block, uint64_t addr){
  int i, j;
  uint64_t tmp_addr = addr & 0xFFFFFFFFFFFFFFE0;
  uint32_t chunk;
  uint mask = 0xFF;
  for(i = 0; i < 32; i += 4){
    chunk = mem_read_32(tmp_addr + i);

    for(j = i; j < i+4; j++){
      block[j] = (uint8_t)(chunk & mask);
      chunk >> 8;
    }
  }
}

uint32_t inst_cache_update(cache_t *c, uint64_t addr){
  // if -1, there are no empty lines
  int empty = -1;
  // initialize to miss
  inst_hit = 0;

  int tmp = addr;
  uint mask = 0x1F;
  uint offset = tmp & mask;
  tmp = tmp >> 5;
  uint set_index = tmp & mask;
  tmp = tmp >> 5;
  unsigned long long tag = tmp;

  set *c_set = &c->sets[set_index];
  int num_lines = sizeof(c_set->lines) / sizeof(c_set->lines[0]);

  int i, j;
  for(i = 0; i < num_lines; i++){
    // Cache hit
    if(c_set->lines[i].tag == tag){
      inst_hit = 1;

      uint32_t inst = 0;
      for(j = offset+3; j >= offset; j--){
        inst = inst << 8;
        inst = inst | c_set->lines[i].block[j];
      }

      shift(c_set->lru, i);

      return inst;
    }
    else if( (empty == -1) && (!c_set->lines[i].valid) )
      empty = i;
  }

  // Cache miss, open slot
  uint32_t inst = mem_read_32(addr);
  if(empty != -1){
    fill_block(c_set->lines[empty].block, addr);
    c_set->lines[empty].valid = 1;
    c_set->lines[empty].tag = tag;
    enqueue(c_set->lru, empty);

    return inst;
  }
  // Cache miss, full cache
  else{
    int idx = LRU_index(c_set->lru);
    fill_block(c_set->lines[idx].block, addr);
    c_set->lines[idx].tag = tag;

  }
}

uint64_t get_data(set *c_set, int index, int offset, int size){
  uint64_t data = 0;
  int j;
  for(j = offset+size-1; j >= offset; j--){
    data = data << 8;
    data = data | c_set->lines[index].block[j];
  }

  return data;
}

uint64_t store_data(set *c_set, int index, int offset, int size, uint64_t val){
  uint64_t data = val;
  uint mask8 = 0xFF;

  int j;
  for(j = offset; j < offset+size; j++){
    c_set->lines[index].block[j] = data & mask8;
    data = data >> 8;
  }

  return data;
}

void write_back(uint8_t *block, uint64_t addr){
  int i, j;
  uint64_t tmp_addr = addr & 0xFFFFFFFFFFFFFFE0;
  uint mask = 0xFF;
  uint32_t chunk;
  for(i = 0; i < 32; i += 4){
    chunk = 0;

    for(j = i+3; j >= i; j--){
      chunk = chunk << 8;
      chunk = chunk | block[j];
    }

    mem_write_32(tmp_addr + i, chunk);
  }
}

uint64_t data_cache_update(cache_t *c, uint64_t addr, int type, int size, uint64_t val){
  // if -1, there are no empty lines
  int empty = -1;
  // initialize to miss
  data_hit = 0;

  int tmp = addr;
  uint mask5 = 0x1F;
  uint mask8 = 0xFF;
  uint offset = tmp & mask5;
  tmp = tmp >> 5;
  uint set_index = tmp & mask8;
  tmp = tmp >> 8;
  unsigned long long tag = tmp;

  set *c_set = &c->sets[set_index];
  int num_lines = sizeof(c_set->lines) / sizeof(c_set->lines[0]);

  uint64_t data;
  int i, j;
  for(i = 0; i < num_lines; i++){
    // Cache hit
    if(c_set->lines[i].tag == tag){
      data_hit = 1;

      // Load Hit
      if(type == 1){
        data = get_data(c_set, i, offset, size);
      }
      // Store Hit
      else{
        data = store_data(c_set, i, offset, size, val);
      }

      shift(c_set->lru, i);
      return data;
    }
    else if( (empty == -1) && (!c_set->lines[i].valid) )
      empty = i;
  }

  // Cache miss, empty slot
  if(empty != -1){
    fill_block(c_set->lines[empty].block, addr);
    c_set->lines[empty].valid = 1;
    c_set->lines[empty].tag = tag;

    // Load
    if(type == 1){
      data = get_data(c_set, i, offset, size);
    }
    // Store
    else{
      data = store_data(c_set, i, offset, size, val);
    }

    enqueue(c_set->lru, empty);
    return data;
  }
  // Cache miss, full cache
  else{
    int idx = LRU_index(c_set->lru);
    // write-back on eviction
    write_back(c_set->lines[idx].block, addr);
    fill_block(c_set->lines[idx].block, addr);
    c_set->lines[idx].tag = tag;

    // Load
    if(type == 1){
      data = get_data(c_set, i, offset, size);
    }
    // Store
    else{
      data = store_data(c_set, i, offset, size, val);
    }

    return data;
  }
}
