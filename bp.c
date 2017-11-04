/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 * Gushu Li and Reza Jokar
 */

#include "bp.h"
#include "pipe.h"
#include "shell.h"
#include <stdlib.h>
#include <stdio.h>

// typedef struct{
//   uint8_t ghr;
//   int PHT[256];
// } gshare;
//
// typedef struct{
//   uint64_t addr;
//   uint64_t target;
//   int valid;
//   int br_type;
// } BTB_entry;
//
// typedef struct{
//   /* gshare */
//   gshare gs;
//   /* BTB */
//   BTB_entry btb[1024]
// } bp_t;


static bp_t predictor;

void bp_initialize(){
  int PHT[256] = {0};
  gshare gs = {0, PHT};
  predictor.gs = gs;

  int i;
  for(i = 0; i < 1024; i++){
    BTB_entry tmp = {0, 0, 0, 0};
    predictor.btb[i] = tmp;
  }

}

void bp_predict(uint64_t PC)
{
    uint mask1 = 0x3FF;
    uint mask2 = 0xFF;
    uint btb_index = (PC >> 2) & mask1;
    uint pht_index = ((PC >> 2) & mask2) ^ predictor.gs.ghr;

    if(predictor.btb[btb_index].addr == PC){
      if(predictor.btb[btb_index].valid){
        if(!predictor.btb[btb_index].br_type || predictor.gs.PHT[pht_index] >= 2)
          CURRENT_STATE.PC  = predictor.btb[btb_index].target;
      }
      else
        CURRENT_STATE.PC += 4;
    }
    else
      CURRENT_STATE.PC += 4;
}

void bp_update(uint64_t PC, uint64_t target, uint taken, int br_type)
{
    uint mask1 = 0x3FF;
    uint btb_index = (PC >> 2) & mask1;

    /* Update BTB */
    predictor.btb[btb_index].addr = PC;
    predictor.btb[btb_index].valid = 1;
    predictor.btb[btb_index].target = target;
    predictor.btb[btb_index].br_type = br_type;

    if(br_type){ // only do these if the branch is conditional
      uint mask2 = 0xFF;
      uint pht_index = ((PC >> 2) & mask2) ^ predictor.gs.ghr;
      int status = predictor.gs.PHT[pht_index];
    /* Update gshare directional predictor */
    if(taken)
      status++;
    else
      status--;

    if(status == 4)
      status = 3;
    else if(status == -1)
      status = 0;

    predictor.gs.PHT[pht_index] = status;

    /* Update global history register */
    predictor.gs.ghr = (predictor.gs.ghr << 1) | taken;
    }
}
