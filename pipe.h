/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 * Reza Jokar, Gushu Li, 2016
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include "bp.h"
#include "cache.h"
#include <limits.h>


typedef struct CPU_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;

} CPU_State;


typedef struct Pipe_Reg_IFtoID{
	int instruction;
	int prediction;
  uint64_t PC;
  uint64_t target;
} Pipe_Reg_IFtoID;

extern Pipe_Reg_IFtoID IFtoID;

typedef struct Pipe_Reg_IDtoEX{
	bool HALT_FLAG;
	int fields1;
	int fields2;
	int fields3;
	int fields4;
	int fields5;
 	char instr_type;
	int prediction;
  uint64_t PC;
  uint64_t target;
} Pipe_Reg_IDtoEX;

extern Pipe_Reg_IDtoEX IDtoEX;

typedef struct Pipe_Reg_EXtoMEM{
	bool HALT_FLAG;
	int FLAG_Z;
	int FLAG_N;
	int64_t result;
	int dest_register;
	uint64_t mem_address;
	int nbits; // for STURx and LDURx
 	bool branch;
 	bool mem_read;
 	bool mem_write;
 	bool reg_write;
 	bool mem_to_reg;
} Pipe_Reg_EXtoMEM;

extern Pipe_Reg_EXtoMEM EXtoMEM;

typedef struct Pipe_Reg_MEMtoWB{
	bool HALT_FLAG;
	int FLAG_Z;
	int FLAG_N;
	int64_t result;
	int dest_register;
	bool reg_write;
	bool mem_to_reg;
} Pipe_Reg_MEMtoWB;

extern Pipe_Reg_MEMtoWB MEMtoWB;

int RUN_BIT;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
