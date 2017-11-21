/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include "bp.h"
#include "pipe.h"
#include "cache.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int inst_hit = 1;
int inst_empty;

int data_hit = 1;
int data_empty;

void free_queue(queue *q){

  node *tmp;

  while(q->head != NULL){
    tmp = q->head;
    q->head = q->head->next;
    //printf("freeing queue element\n");
    free(tmp);
  }
  //printf("freeing queue pointer\n");
  free(q);
}

int LRU_index(queue *q) {

  //printf("LRU lookup...\n");

  node *tmp = q->head;
  q->head = q->head->next;
  tmp->next = NULL;
  q->tail->next = tmp;
  q->tail = tmp;
  return tmp->index;

}

void enqueue(queue *q, int index) {

  //printf("beginning enqueue\n");
  node *new_node = (node*)malloc(sizeof(node));
  new_node->index = index;
  new_node->next = NULL;
  if (q->head == NULL) {
    q->head = new_node;
    q->tail = new_node;
  } else {
    // NOTE: what if head == tail?
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
  memset(new_line.block, 0, 32);
  //memset(new_line.block, 0, sizeof(new_line.block));
  //printf("sizeof(new_line.block): %lu\n", sizeof(new_line.block));
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

void cache_destroy(cache_t *c, int num_sets, int ways)
{
  int i, j, k;
  //int num_sets = sizeof(c->sets) / sizeof(c->sets[0]);
  //int ways = sizeof(c->sets->lines) / sizeof(c->sets->lines[0]);

  for(i = 0; i < num_sets; i++){
    for(j = 0; j < ways; j++){
      //printf("freeing blocks\n");
      free(c->sets[i].lines[j].block);
    }
    //printf("freeing lines\n");
    free(c->sets[i].lines);
    //printf("freeing queue\n");
    free_queue(c->sets[i].lru);
  }
  //printf("freeing sets\n");
  free(c->sets);
//  printf("freeing cache\n");
  free(c);
}


void fill_block(uint8_t *block, uint64_t addr){
  //printf("filling block...\n");
  int i, j;
  uint64_t tmp_addr = addr & 0xFFFFFFFFFFFFFFE0;
  uint32_t chunk;
  uint mask = 0xFF;
  for(i = 0; i < 32; i += 4){
    chunk = mem_read_32(tmp_addr + i);
    //printf("chunk: %08x\n", inst);

    for(j = i; j < i+4; j++){
      //printf("chunk assignment block[%d]\n", j);
      block[j] = (uint8_t)(chunk & mask);
      chunk = chunk >> 8;
    }
  }
}

// uint32_t inst_handle_miss(cache_t *c, uint64_t addr){
//
//   //printf("inst_empty: %d\n", inst_empty);
//
//   uint64_t tmp = addr;
//   uint mask = 0x3F;
//   tmp = tmp >> 5;
//   uint set_index = tmp & mask;
//   tmp = tmp >> 6;
//   unsigned long long tag = tmp;
//
//   set *c_set = &c->sets[set_index];
//
//   // Cache miss, open slot
//   uint32_t inst = mem_read_32(addr);
//   printf("miss handle inst: %08x\n", inst);
//   if(inst_empty != -1){
//
//   //  printf("inst_empty: %d\n", inst_empty);
//
//     // if(c_set->lines[inst_empty].block == NULL){
//     //   printf("block is null\n");
//     // }
//     // else
//     //   printf("block is not null\n");
//
//     fill_block(c_set->lines[inst_empty].block, addr);
//     c_set->lines[inst_empty].valid = 1;
//     c_set->lines[inst_empty].tag = tag;
//     enqueue(c_set->lru, inst_empty);
//   }
//   // Cache miss, full cache
//   else{
//     int idx = LRU_index(c_set->lru);
//     fill_block(c_set->lines[idx].block, addr);
//     c_set->lines[idx].tag = tag;
//  }
//
//   inst_hit = 1;
//   return inst;
// }


void inst_handle_miss(cache_t *c, uint64_t addr){

  //printf("inst_empty: %d\n", inst_empty);

  uint64_t tmp = addr;
  uint mask = 0x3F;
  tmp = tmp >> 5;
  uint set_index = tmp & mask;
  tmp = tmp >> 6;
  unsigned long long tag = tmp;

  set *c_set = &c->sets[set_index];

  // Cache miss, open slot
  //printf("miss handle inst: %08x\n", inst);
  if(inst_empty != -1){

  //  printf("inst_empty: %d\n", inst_empty);

    // if(c_set->lines[inst_empty].block == NULL){
    //   printf("block is null\n");
    // }
    // else
    //   printf("block is not null\n");

    fill_block(c_set->lines[inst_empty].block, addr);
    c_set->lines[inst_empty].valid = 1;
    c_set->lines[inst_empty].tag = tag;
    enqueue(c_set->lru, inst_empty);
  }
  // Cache miss, full cache
  else{
    int idx = LRU_index(c_set->lru);
    fill_block(c_set->lines[idx].block, addr);
    c_set->lines[idx].tag = tag;
 }

  inst_hit = 1;
}

uint32_t inst_cache_update(cache_t *c, uint64_t addr){
  //printf("accessing icache...\n");
  // initialize to miss
  inst_hit = 0;

  // if -1, there are no empty lines
  inst_empty = -1;

  uint32_t inst = 0;
  uint64_t tmp = addr;
  uint offset = tmp & 0x1F;
  tmp = tmp >> 5;
  uint set_index = tmp & 0x3f;
  tmp = tmp >> 6;
  unsigned long long tag = tmp;

  //printf("computed tag: %llx\n", tag);

  set *c_set = &c->sets[set_index];
  //int num_lines = sizeof(c_set->lines) / sizeof(c_set->lines[0]);
  //printf("num_lines: %d\n", num_lines);
  //printf("sizeof(c_set->lines): %lu\n", sizeof(c_set->lines));
  //printf("sizeof(c_set->lines[0]): %lu\n", sizeof(c_set->lines[0]));



  int i, j;
  for(i = 0; i < 4; i++){
    // Cache hit
    //printf("line index %d cache tag (%llx)\n", i, c_set->lines[i].tag);
    if(c_set->lines[i].tag == tag){
      //printf("hit\n");
      //printf("offset + 3 = %d\n", offset + 3);
      for(j = offset+3; j >= (int)offset; j--){
        //printf("offset = %d\n", offset);
        //printf("accessing block[%d]\n", j);
        inst = inst << 8;
        inst = inst | c_set->lines[i].block[j];
      }

      //printf("hit tag (%lx)\n", c_set->lines[i].tag);
      printf("icache hit (%lx)\n", addr);

      shift(c_set->lru, i);

      inst_hit = 1;
      printf("hit inst: %08x\n", inst);
      return inst;
    }
    else if( (inst_empty == -1) && (!c_set->lines[i].valid) ){
      inst_empty = i;
      //printf("found empty line at index: %d\n", i);
    }
  }

  printf("inst miss\n");
  return inst;
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

void write_back(line line, int idx){
  uint64_t tmp_addr = line.tag;
  tmp_addr = (tmp_addr << 8) | (uint)idx;
  tmp_addr = tmp_addr << 5;

  int i, j;
  uint mask = 0xFF;
  uint32_t chunk;
  for(i = 0; i < 32; i += 4){
    chunk = 0;

    for(j = i+3; j >= i; j--){
      chunk = chunk << 8;
      chunk = chunk | line.block[j];
    }

    mem_write_32(tmp_addr + i, chunk);
  }
}


// uint64_t data_handle_miss(cache_t *c, uint64_t addr, int type, int size, uint64_t val){
//   uint64_t data = 0;
//
//   uint64_t tmp = addr;
//   uint mask5 = 0x1F;
//   uint mask8 = 0xFF;
//   uint offset = tmp & mask5;
//   tmp = tmp >> 5;
//   uint set_index = tmp & mask8;
//   tmp = tmp >> 8;
//   unsigned long long tag = tmp;
//
//   set *c_set = &c->sets[set_index];
//
//   // Cache miss, empty slot
//   if(data_empty != -1){
//     fill_block(c_set->lines[data_empty].block, addr);
//     c_set->lines[data_empty].valid = 1;
//     c_set->lines[data_empty].tag = tag;
//
//     // Load
//     if(type == 1){
//       data = get_data(c_set, data_empty, offset, size);
//     }
//     // Store
//     else{
//       data = store_data(c_set, data_empty, offset, size, val);
//     }
//
//     enqueue(c_set->lru, data_empty);
//   }
//   // Cache miss, full cache
//   else{
//     int idx = LRU_index(c_set->lru);
//     // write-back on eviction
//     write_back(c_set->lines[idx], idx);
//     fill_block(c_set->lines[idx].block, addr);
//     c_set->lines[idx].tag = tag;
//
//     // Load
//     if(type == 1){
//       data = get_data(c_set, idx, offset, size);
//     }
//     // Store
//     else{
//       data = store_data(c_set, idx, offset, size, val);
//     }
//   }
//
//   data_hit = 1;
//   return data;
// }


void data_handle_miss(cache_t *c, uint64_t addr){

  //printf(" (handling miss) data address: %08lx\n", addr);

  uint64_t tmp = addr;
  uint mask5 = 0x1F;
  uint mask8 = 0xFF;
  uint offset = tmp & mask5;
  tmp = tmp >> 5;
  uint set_index = tmp & mask8;
  tmp = tmp >> 8;
  unsigned long long tag = tmp;

  set *c_set = &c->sets[set_index];

  // Cache miss, empty slot
  if(data_empty != -1){
    fill_block(c_set->lines[data_empty].block, addr);
    c_set->lines[data_empty].valid = 1;
    c_set->lines[data_empty].tag = tag;

    enqueue(c_set->lru, data_empty);
  }
  // Cache miss, full cache
  else{
    int idx = LRU_index(c_set->lru);
    // write-back on eviction
    write_back(c_set->lines[idx], idx);
    fill_block(c_set->lines[idx].block, addr);
    c_set->lines[idx].tag = tag;
  }

  data_hit = 1;
}

uint64_t data_cache_update(cache_t *c, uint64_t addr, int type, int size, uint64_t val){

  if(type){
    printf("data load...\n");
  }
  else
    printf("data store...\n");

  printf("data address: %08lx\n", addr);


  // if -1, there are no empty lines
  data_empty = -1;
  // initialize to miss
  data_hit = 0;

  uint64_t tmp = addr;
  uint mask5 = 0x1F;
  uint mask8 = 0xFF;
  uint offset = tmp & mask5;
  tmp = tmp >> 5;
  uint set_index = tmp & mask8;
  tmp = tmp >> 8;
  unsigned long long tag = tmp;

  set *c_set = &c->sets[set_index];
  //int num_lines = sizeof(c_set->lines) / sizeof(c_set->lines[0]);

  uint64_t data = 0;
  int i, j;
  for(i = 0; i < 8; i++){
    // Cache hit
    //printf("c_set->lines[i].tag = %llx\n", c_set->lines[i].tag);
    //printf("tag = %llx\n", tag);
    if(c_set->lines[i].tag == tag){

      //printf("data hit\n");

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
      printf("data hit (%lx)\n", data);
      return data;
    }
    else if( (data_empty == -1) && (!c_set->lines[i].valid) )
      data_empty = i;
  }

  printf("data miss\n");
  return data;
}
