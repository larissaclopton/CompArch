/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 */

#ifndef _BP_H_
#define _BP_H_

#include "shell.h"
#include "stdbool.h"
#include "bp.h"
#include <limits.h>
#include "pipe.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct{
  uint8_t ghr;
  int PHT[256];
} gshare;

typedef struct{
  uint64_t addr;
  uint64_t target;
  int valid;
  int br_type;
} BTB_entry;

typedef struct{
  /* gshare */
  gshare gs;
  /* BTB */
  BTB_entry btb[1024];
} bp_t;


void bp_initialize();
void bp_predict(uint64_t PC);
void bp_update(uint64_t PC, uint64_t target, uint taken, int br_type);

#endif
