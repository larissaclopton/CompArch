/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#include "pipe.h"
#include "shell.h"
#include "bp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


// NOTE: Team Members
// Larissa Clopton cloptla
// Nathan Chortek	nchortek


/* global pipeline state */
CPU_State CURRENT_STATE;

void pipe_init()
{
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;

    bp_inialize();
}


typedef struct Pipe_Reg_IFtoID{
	int instruction;
  uint64_t PC;
  uint64_t target;
} Pipe_Reg_IFtoID;

static Pipe_Reg_IFtoID IFtoID;

typedef struct Pipe_Reg_IDtoEX{
	bool HALT_FLAG;
	int fields1;
	int fields2;
	int fields3;
	int fields4;
	int fields5;
 	char instr_type;
  uint64_t PC;
  uint64_t target;
} Pipe_Reg_IDtoEX;

static Pipe_Reg_IDtoEX IDtoEX = {false, 0, 0, 0, 0, 0, '0'};

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

static Pipe_Reg_EXtoMEM EXtoMEM = {false, 0, 0, 0, 0, 0, 0, false, false, false, false, false};

typedef struct Pipe_Reg_MEMtoWB{
	bool HALT_FLAG;
	int FLAG_Z;
	int FLAG_N;
	int64_t result;
	int dest_register;
	bool reg_write;
	bool mem_to_reg;
} Pipe_Reg_MEMtoWB;

static Pipe_Reg_MEMtoWB MEMtoWB = {false, 0, 0, 0, 0, false, false};

bool IF_FLUSH = false;
bool ID_FLUSH = true;
bool EX_FLUSH = true;
bool MEM_FLUSH = true;
bool WB_FLUSH = true;

bool IF_STALL = false;
bool ID_STALL = false;
bool EX_STALL = false;
bool MEM_STALL = false;
bool WB_STALL = false;

bool TMP_STALL_EX = false;
bool TMP_STALL_IF = false;


void IF_bubble(){
	IFtoID.instruction = 0;
	ID_FLUSH = true;
 	printf("IF bubble\n");
}

void ID_bubble(){
	IDtoEX.fields1 = 0;
  	IDtoEX.fields2 = 0;
  	IDtoEX.fields3 = 0;
  	IDtoEX.fields4 = 0;
  	IDtoEX.fields5 = 0;
  	IDtoEX.instr_type = '0';
  	printf("ID bubble\n");
  	EX_FLUSH = true;
}

void EX_bubble(){
    EXtoMEM.result = 0;
    EXtoMEM.dest_register = 0;
    EXtoMEM.mem_address = 0;
    EXtoMEM.nbits = 0; // for STURx and LDURx
    EXtoMEM.branch = false;
 	EXtoMEM.mem_read = false;
 	EXtoMEM.mem_write = false;
 	EXtoMEM.reg_write = false;
 	EXtoMEM.mem_to_reg = false;
	printf("EX bubble\n");
	MEM_FLUSH = true;
}

void MEM_bubble(){
    MEMtoWB.result = 0;
	MEMtoWB.dest_register = 0;
	MEMtoWB.reg_write = 0;
	MEMtoWB.mem_to_reg = 0;
	printf("MEM bubble\n");
	WB_FLUSH = true;
}


int64_t* forward(int regs[], int len){
  	int64_t *res = (int64_t*)malloc(len*sizeof(int64_t));
	int stall = 0;

	int i;
	for(i = 0; i < len; i++){
  		if(EXtoMEM.mem_read){
    		if(EXtoMEM.dest_register == regs[i]){
      			stall = 1; //temp flag so we know not to use the return value
				res[i] = 0;
				continue;
    		}
  		}

  		if(EXtoMEM.reg_write){
    		if(EXtoMEM.dest_register == regs[i]){
      			res[i] = EXtoMEM.result;
				continue;
    		}
  		}

  		if(MEMtoWB.reg_write){
    		if(MEMtoWB.dest_register == regs[i]){
      			res[i] = MEMtoWB.result;
				continue;
    		}
  		}

  		res[i] = CURRENT_STATE.REGS[regs[i]];

	}

	if(stall){
		printf("forward decides to stall\n");
		IF_STALL = true;
		ID_STALL = true;
		EX_FLUSH = true;

	  TMP_STALL_EX = true;
	}
	else{
		printf("forward decides NOT to stall\n");
		IF_STALL = false;
		ID_STALL = false;
		EX_FLUSH = false;
	}

	return res;
}


// Function implementations

void ADDx(char instr_type, int set_flag, int fields[]){
	printf("executing ADDx\n");
	if(instr_type == 'R'){
		int regs[2] = {fields[1], fields[3]};
		int64_t *res = forward(regs, 2);

		if(!EX_FLUSH){
			EXtoMEM.result = res[0] + res[1];
			EXtoMEM.dest_register = fields[4];
		}
		free(res);
	}
	else{
		int regs[1] = {fields[2]};
		int64_t *res = forward(regs, 1);

    	if(!EX_FLUSH){
    		EXtoMEM.result = fields[1] + res[0];
			EXtoMEM.dest_register = fields[3];
		}
		free(res);
	}

	//printf("ADDx result: %16lx\n", EXtoMEM.result);
	//printf("ADDx dest reg: %d\n", EXtoMEM.dest_register);

	if(EX_FLUSH){
		EX_bubble();
		EX_FLUSH = false;
		return;
	}

	if(set_flag){
		if(EXtoMEM.result < CURRENT_STATE.REGS[31])
     		EXtoMEM.FLAG_N = 1;
    	else
     		EXtoMEM.FLAG_N = 0;

    	if(EXtoMEM.result == CURRENT_STATE.REGS[31])
        	EXtoMEM.FLAG_Z = 1;
    	else
        	EXtoMEM.FLAG_Z = 0;
	}

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void SUBx(char instr_type, int set_flag, int fields[]){
	if(instr_type == 'R'){
		int regs[2] = {fields[3], fields[1]};
		int64_t *res = forward(regs, 2);
		//TODO: check if registers are waiting to be written
		EXtoMEM.result = res[0] - res[1];
		EXtoMEM.dest_register = fields[4];
		free(res);
	}
	else{
		int regs[1] = {fields[2]};
		int64_t *res = forward(regs, 1);
		//TODO: check if register waiting to be written
		EXtoMEM.result = (int64_t)res[0] - (int64_t)fields[1];
		EXtoMEM.dest_register = fields[3];
		free(res);
	}

	if(EX_FLUSH){
		EX_bubble();
		EX_FLUSH = false;
    CURRENT_STATE.REGS[31] = 0;
		return;
	}

	//TODO: still set flags in current state?
	if(set_flag){
		if(EXtoMEM.result < CURRENT_STATE.REGS[31])
    		EXtoMEM.FLAG_N = 1;
   		else
      		EXtoMEM.FLAG_N = 0;

    	if(EXtoMEM.result == CURRENT_STATE.REGS[31])
      		EXtoMEM.FLAG_Z = 1;
    	else
    		EXtoMEM.FLAG_Z = 0;

			//set 0-register to 0
			CURRENT_STATE.REGS[31] = 0;
	}

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void ANDx(int set_flag, int fields[]) {
	//TODO: check if registers are waiting to be written
	int regs[2] = {fields[3], fields[1]};
	int64_t *res = forward(regs, 2);

	int64_t temp = res[0] & res[1];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;

	if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	//TODO: still set flags in current state?
	if(set_flag){
		if(temp < CURRENT_STATE.REGS[31])
			EXtoMEM.FLAG_N = 1;
		else
			EXtoMEM.FLAG_N = 0;

		if(temp == CURRENT_STATE.REGS[31])
			EXtoMEM.FLAG_Z = 1;
		else
			EXtoMEM.FLAG_Z = 0;
	}

	free(res);


	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void MUL(int fields[]){
	int regs[2] = {fields[3], fields[1]};
	int64_t *res = forward(regs, 2);

	//TODO: check if registers are waiting to be written
	int64_t temp = res[0] * res[1];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;

	if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	free(res);

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;

}

void EOR(int fields[]){
	//TODO: check if registers are waiting to be written
	int regs[2] = {fields[3], fields[1]};
	int64_t *res = forward(regs, 2);
	int64_t temp = res[1] ^ res[0];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;

	if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	free(res);

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void ORR(int fields[]){
	int regs[2] = {fields[3], fields[1]};
	int64_t *res = forward(regs, 2);

	//TODO: check if registers are waiting to be written
	int64_t temp = res[1] | res[0];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;

	if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	free(res);

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

//TODO: add data dependency shit (ex flush check, handle pc in some way)
void BR(int fields[]){

	//TODO: check if register is waiting to be written
	//TODO: how to squash instructions already in pipeline?
	int regs[1] = {fields[3]};
	int64_t *res = forward(regs, 1);
  int64_t tmp = res[0];

  if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	free(res);

  if(IDtoEX.target != tmp){
    //misprediction handling
    CURRENT_STATE.PC = tmp;
  }
  bp_update(IDtoEX.PC, tmp, 1, 0);

	EXtoMEM.nbits = 0;
	EXtoMEM.branch = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;

  ID_FLUSH = true;
  TMP_STALL_IF = true;
}



void MOVZ(int fields[]){
	int64_t val = (int64_t)fields[2];

	// no action required if fields[1] == 0
	if(fields[1] == 1) {
		val <<= 16;
	}
	else if(fields[1] == 2){
		val <<= 32;
	}
	else if(fields[1] == 3){
		val <<= 48;
	}

	EXtoMEM.dest_register = fields[3];
	EXtoMEM.result = val;

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void LSx(int fields[]){
	int shamt = fields[1] & 0x3F;
	int right_shift = (fields[1] >> 6) & 0x3F;
	int mask = (-1) << 6;
	int left_shift = right_shift | mask;
	left_shift = (~left_shift) + 1;

	int regs[] = {fields[2]};
	int64_t *res = forward(regs,1);
	uint64_t val = (uint64_t)res[0];

	if(shamt == 0x3F){
		val >>= right_shift;
	}
	else {
		val <<= left_shift;
	}

	EXtoMEM.dest_register = fields[3];
	EXtoMEM.result = (int64_t)val;

	EXtoMEM.mem_address = 0;
	EXtoMEM.nbits = 0;
	EXtoMEM.branch = false; //NOTE: will need to be reset in BR
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = false;
	EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
	EXtoMEM.mem_to_reg = false;
}

void STURx(int nbits, int fields[]){

	int regs[] = {fields[3], fields[4]};
	int64_t *res = forward(regs, 2);
	uint64_t start_addr = (uint64_t)(res[0] + fields[1]);
	int64_t val = res[1];
	int64_t result;
	if(nbits == 64){
		result = val;
		//mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
		//mem_write_32(start_addr+4, (uint32_t)((val >> 32) & 0xFFFFFFFF));
	}
	else if(nbits == 16){
		uint32_t mem = mem_read_32(start_addr);
		int mask_16 = 0xFFFF << 16;
		mem = mem & mask_16;

		uint32_t val2 = (uint32_t)val & 0xFFFF;
		mem = mem + val2;
		result = (int64_t)mem;
	}
	else{ // bits = 8
		uint32_t mem = mem_read_32(start_addr);
		int mask_24 = 0xFFFFFF << 8;
		mem = mem & mask_24;

		uint32_t val2 = (uint32_t)val & 0xFF;
		mem = mem + val2;
		result = (int64_t)mem;
	}

	if(EX_FLUSH){
		EX_bubble();
		free(res);
		EX_FLUSH = false;
		return;
	}

	free(res);

	EXtoMEM.branch = false;
	EXtoMEM.dest_register = 0;
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = true;
	EXtoMEM.reg_write = false;
	EXtoMEM.mem_to_reg = false;
	EXtoMEM.mem_address = start_addr;
	EXtoMEM.result = result;
	EXtoMEM.nbits = nbits;

}

void LDURx(int nbits, int fields[]){
	printf("executing LDUR\n");

	int regs[] = {fields[3]};
	int64_t *res = forward(regs, 1);
	uint64_t start_addr = (uint64_t)(res[0] + fields[1]);
	//printf("load start addr: %08lx\n", start_addr);
	//printf("load start addr+32: %08lx\n", start_addr + 4);

	if(EX_FLUSH){
		EX_bubble();
		EX_FLUSH = false;
		free(res);
		return;
	}

	free(res);


	EXtoMEM.branch = false;
	EXtoMEM.result = 0;
	EXtoMEM.mem_read = true;
	EXtoMEM.mem_write = false;
	EXtoMEM.mem_to_reg = true;
	EXtoMEM.reg_write = true;
	EXtoMEM.mem_address = start_addr;
	EXtoMEM.nbits = nbits;
	EXtoMEM.dest_register = fields[4];

}

//TODO: control dependency
void BRANCH_IMM(int fields[]){
	//printf("branching IMM\n");
	int64_t addr = (((((int64_t)fields[1]) << 38 ) >> 38) << 2);
  if(IDtoEX.target != IDtoEX.PC + addr){
    //misprediction handling
    CURRENT_STATE.PC = IDtoEX.PC + addr;
  }
  bp_update(IDtoEX.PC, IDtoEX.PC + addr, 1, 0);


  EXtoMEM.branch = true;
  EXtoMEM.result = 0;
  EXtoMEM.dest_register = 0;
  EXtoMEM.mem_address = 0;
  EXtoMEM.mem_read = false;
  EXtoMEM.mem_write = false;
  EXtoMEM.reg_write = false;
  EXtoMEM.mem_to_reg = false;

  // if the branch is taken (which it is)
  ID_FLUSH = true;
  TMP_STALL_IF = true;
}

//TODO: data & control dependency
// so EX_FLUSH --> bubble & free shit
void CBNZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B

	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	int regs[] = {fields[2]};
	int64_t *res = forward(regs, 1);
  int64_t tmp = res[0];

  if(EX_FLUSH){
		EX_bubble();
		EX_FLUSH = false;
		free(res);
		return;
	}

  free(res);


	if(tmp != CURRENT_STATE.REGS[31]) {
    EXtoMEM.branch = true;
    //printf("branching...\n");
    if(IDtoEX.target != IDtoEX.PC + addr){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + addr;
    }
    bp_update(IDtoEX.PC, IDtoEX.PC + addr, 1, 1);

    ID_FLUSH = true;
    TMP_STALL_IF = true;
	}
	else {
    //printf("not branching...\n");
    if(IDtoEX.target != IDtoEX.PC + 4){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + 4;
    }
    bp_update(IDtoEX.PC, IDtoEX.PC + 4, 0, 1);

		EXtoMEM.branch = false;
    ID_bubble();
    ID_STALL = true;
    TMP_STALL_IF = true;
	}


  EXtoMEM.result = 0;
  EXtoMEM.dest_register = 0;
  EXtoMEM.mem_address = 0;
  EXtoMEM.mem_read = false;
  EXtoMEM.mem_write = false;
  EXtoMEM.reg_write = false;
  EXtoMEM.mem_to_reg = false;
}


//TODO: data & control dependency
// so EX_FLUSH --> bubble & free shit
void CBZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B
	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	int regs[] = {fields[2]};
	int64_t *res = forward(regs, 1);
  int64_t tmp = res[0];


  if(EX_FLUSH){
		EX_bubble();
		EX_FLUSH = false;
		free(res);
		return;
	}

  free(res);


	if(tmp == CURRENT_STATE.REGS[31]) {
    EXtoMEM.branch = true;
    //printf("branching...\n");
    if(IDtoEX.target != IDtoEX.PC + addr){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + addr;
    }
    bp_update(IDtoEX.PC, IDtoEX.PC + addr, 1, 1);

    ID_FLUSH = true;
    TMP_STALL_IF = true;
	}
	else {
    //printf("not branching...\n");
    if(IDtoEX.target != IDtoEX.PC + 4){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + 4;
    }
    bp_update(IDtoEX.PC, IDtoEX.PC + 4, 0, 1);

		EXtoMEM.branch = false;
    ID_bubble();
    ID_STALL = true;
    TMP_STALL_IF = true;
	}


  EXtoMEM.result = 0;
  EXtoMEM.dest_register = 0;
  EXtoMEM.mem_address = 0;
  EXtoMEM.mem_read = false;
  EXtoMEM.mem_write = false;
  EXtoMEM.reg_write = false;
  EXtoMEM.mem_to_reg = false;

}


//TODO: control dependency
void B_COND(int fields[]){

	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	int cond = fields[2] & 0x0000000F;
	int branch = 0;

	switch (cond) {
		case 0x00000000: { //equal
			//printf("executing beq...\n");
			//if Z = 1,
			if(EXtoMEM.FLAG_Z == 1)
				branch = 1;
		} break;
		case 0x00000001: { // not equal
			//printf("executing bne...\n");
			if(EXtoMEM.FLAG_Z == 0)
				branch = 1;
		} break;
		case 0x0000000A: { // greater than or equal
			//printf("executing bge...\n");
			if(EXtoMEM.FLAG_N == 0)
				branch = 1;
		} break;
		case 0x0000000B: { // less than
			//printf("executing bl...\n");
			if(EXtoMEM.FLAG_N == 1)
				branch = 1;
		} break;
		case 0x0000000C: {//greater than
			//printf("executing bg...\n");
			if(EXtoMEM.FLAG_Z == 0 && EXtoMEM.FLAG_N == 0)
				branch = 1;
		} break;
		case 0x0000000D: {//less than or equal to
			//printf("executing ble...\n");
			if(!(EXtoMEM.FLAG_Z == 0 && EXtoMEM.FLAG_N == 0))
				branch = 1;
		} break;
	}

	if(branch){
		EXtoMEM.branch = true;
		//printf("branching...\n");
    if(IDtoEX.target != IDtoEX.PC + addr){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + addr;
    }
    bp_update(IDtoEX.PC, IDtoEX.PC + addr, 1, 1);

    ID_FLUSH = true;
    TMP_STALL_IF = true;
	}
	else{
		//printf("not branching...\n");
    if(IDtoEX.target != IDtoEX.PC + 4){
      //misprediction handling
      CURRENT_STATE.PC = IDtoEX.PC + 4;
    }
  	//CURRENT_STATE.PC = CURRENT_STATE.PC - 8 + addr;
    bp_update(IDtoEX.PC, IDtoEX.PC + 4, 0, 1);

		EXtoMEM.branch = false;
    ID_bubble();
    ID_STALL = true;
    TMP_STALL_IF = true;
	}

  EXtoMEM.result = 0;
  EXtoMEM.dest_register = 0;
  EXtoMEM.mem_address = 0;
  EXtoMEM.mem_read = false;
  EXtoMEM.mem_write = false;
  EXtoMEM.reg_write = false;
  EXtoMEM.mem_to_reg = false;
}

void STURx_MEM(uint64_t start_addr, int nbits, int64_t val){

	if(nbits == 64){
		mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
		mem_write_32(start_addr+4, (uint32_t)((val >> 32) & 0xFFFFFFFF));
	}
	else if(nbits == 16){
		mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
	}
	else{ // bits = 8
		mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
	}

}

int64_t LDURx_MEM(uint64_t start_addr, int nbits){

	printf("LDURx_MEM\n");

	if(nbits == 64){
		//printf("calling ldur\n");
		uint64_t lower32 = (uint64_t)mem_read_32(start_addr);
		//should this be not 32, but 4?)
		uint64_t upper32 = (uint64_t)mem_read_32(start_addr + 4);
		//printf("upper 32: %16lx \n", upper32);
		//printf("lower 32: %16lx \n", lower32);
		return (upper32 << 32) + (lower32);
	}
	else if(nbits == 16){
		//printf("calling ldurh\n");
		uint64_t val = (uint64_t)mem_read_32(start_addr);
		uint64_t temp = val & 0xFFFF;
			//printf("set reg %ld", temp);
		return val & 0xFFFF; // gets bottom 16 bits
	}
	else { // nbits = 8
		//printf("calling ldurb\n");
		uint64_t val = (uint64_t)mem_read_32(start_addr);
		return val & 0xFF; // gets bottom 8 bits
	}

}


// Execution implementation
void execute(char instr_type, int fields[])
{
  //int branch_reg = 0;
  switch (instr_type){
    case 'R': {
      //printf("case R\n");
		//EXtoMEM.mem_address = 0;
		//EXtoMEM.nbits = 0;
		//EXtoMEM.branch = false; //NOTE: will need to be reset in BR
		//EXtoMEM.mem_read = false;
		//EXtoMEM.mem_write = false;
		//EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
		//EXtoMEM.mem_to_reg = false;
      switch(fields[0]) {

              case 0x459: // ADD extended format treated as ADD
              case 0x458: { // ADD
                  //printf("add...\n");
                  ADDx('R', 0, fields);
                } break;
                case 0x559: // ADDS extended format treated as ADDS
                case 0x558: { // ADDS
                  //printf("ADDS ... \n");
                  ADDx('R', 1, fields);
                } break;
                case 0x450: { // AND
                  ANDx(0, fields);
                } break;
                case 0x750: { // ANDS
                  ANDx(1, fields);
                } break;
                case 0x650: { // EOR
                  EOR(fields);
                } break;
                case 0x550: { // ORR
                  ORR(fields);
                } break;
                case 0x659: // SUB extended format treated as SUB
                case 0x658: { // SUB
                  SUBx('R', 0, fields);
                } break;
                case 0x759: // SUBS extended format treated as SUBS
                case 0x758: { // SUBS
                  //printf("SUBS ... \n");
                  SUBx('R', 1, fields);
                } break;
                case 0x4D8: { // MUL
                  MUL(fields);
                } break;
                case 0x6B0: { // BR
                  BR(fields);
                } break;
      }

    } break;
    case 'I': {
      //printf("case I\n");
		//EXtoMEM.mem_address = 0;
		//EXtoMEM.nbits = 0;
		//EXtoMEM.branch = false;
		//EXtoMEM.mem_read = false;
		//EXtoMEM.mem_write = false;
		//EXtoMEM.reg_write = true;
		//EXtoMEM.mem_to_reg = false;
      switch(fields[0]) {

        case 0x244: { // ADDI
          //printf("addi...\n");
          ADDx('I', 0, fields);
        } break;
        case 0x2c4: { // ADDIS
          //printf("ADDIS ... \n");
          ADDx('I', 1, fields);
        } break;
        case 0x344: { // SUBI
          SUBx('I', 0, fields);
        } break;
        case 0x3c4: { // SUBIS
          SUBx('I', 1, fields);
        } break;
        case 0x1A5: {//MOVZ
          MOVZ(fields);
        } break;
        case 0x34D: { //lsx
          LSx(fields);
        } break;

      }

      //NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    } break;
    case 'D': {
      switch(fields[0]) {

        case 0x7C0: { // STUR
          //printf("calling stur...\n");
          STURx(64, fields);
        } break;
        case 0x3C0: { // STURH
          //printf("calling sturh...\n");
          STURx(16, fields);
        } break;
        case 0x1C0: { // STURB
          //printf("calling sturb...\n");
          STURx(8, fields);
        }	break;
        case 0x7C2: { // LDUR
          //printf("calling ldur...\n");
          LDURx(64, fields);
        } break;
        case 0x3C2: { // LDURH
          //printf("calling ldurh...\n");
          LDURx(16, fields);
        } break;
        case 0x1C2: { // LDURB
        //	printf("calling ldurb...\n");
          LDURx(8, fields);
        } break;

      }

      //NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    } break;
    case 'C': {
		// EXtoMEM.result = 0;
		// EXtoMEM.dest_register = 0;
		// EXtoMEM.mem_address = 0;
		// EXtoMEM.mem_read = false;
		// EXtoMEM.mem_write = false;
		// EXtoMEM.reg_write = false;
		// EXtoMEM.mem_to_reg = false;
      switch (fields[0]) {
        case 0xB5: { //CBNZ
          CBNZ(fields);
        } break;
        case 0xB4: { //CBZ
          CBZ(fields);
        } break;
        case 0x54: { //b.cond
          //printf("executing b.cond...\n");
          B_COND(fields);
        } break;
      }
    } break;
    case 'B': {
		// EXtoMEM.branch = true;
		// EXtoMEM.result = 0;
		// EXtoMEM.dest_register = 0;
		// EXtoMEM.mem_address = 0;
		// EXtoMEM.mem_read = false;
		// EXtoMEM.mem_write = false;
		// EXtoMEM.reg_write = false;
		// EXtoMEM.mem_to_reg = false;
      //call function (immediate branching)
      //printf("immediate branching...\n");
      BRANCH_IMM(fields);
    } break;

  }

  return;
}


// Decoders for each function type

void R_decoder(int instruct_no)
{


	int five_bit_mask = 0x0000001F;

	int Rd = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int Rn = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int shamt = instruct_no & 0x0000003F;
	instruct_no >>= 6;
	int Rm = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int opcode = instruct_no & 0x000007FF;

	//special case for LSx
	if (((unsigned)opcode) >> 1 == 0x0000034D)
		opcode = 0x0000034D;

	IDtoEX.fields1 = opcode;
 	IDtoEX.fields2 = Rm;
 	IDtoEX.fields3 = shamt;
 	IDtoEX.fields4 = Rn;
 	IDtoEX.fields5 = Rd;

	//int i;
	//for(i = 0; i < 5; i++){
	//	printf("%08x \n", fields[i]);
	//}
  	IDtoEX.instr_type = 'R';
}

void I_decoder(int instruct_no)
{

	int five_bit_mask = 0x0000001F;
	//int fields[5];

	//special case for MOVZ
	if((unsigned)instruct_no >> 23 == 0x000001A5){
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int imm = instruct_no & 0x0000FFFF;
		instruct_no >>= 16;
		int op2 = instruct_no & 0x00000003;
		instruct_no >>= 2;
		int opcode = instruct_no & 0x000001FF;

		IDtoEX.fields1 = opcode;
		IDtoEX.fields2 = op2;
		IDtoEX.fields3 = imm;
		IDtoEX.fields4 = Rd;
   	IDtoEX.fields5 = 0;
	}
	else{
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int Rn = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int immediate = instruct_no & 0x00000FFF;
		instruct_no >>= 12;
		int opcode = instruct_no & 0x000003FF;

		IDtoEX.fields1 = opcode;
		IDtoEX.fields2 = immediate;
		IDtoEX.fields3 = Rn;
		IDtoEX.fields4 = Rd;
   	IDtoEX.fields5 = 0;

		//int i;
		//for(i = 0; i < 4; i++){
    //     	printf("%08x \n", fields[i]);
     //	}
	}

 	IDtoEX.instr_type = 'I';
}

void D_decoder(int instruct_no)
{

	int five_bit_mask = 0x0000001F;
	int Rt = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int Rn = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int op2 = instruct_no & 0x00000003;
	instruct_no >>= 2;
	int offset = instruct_no & 0x000001FF;
	instruct_no >>= 9;
	int opcode = instruct_no & 0x000007FF;

	IDtoEX.fields1 = opcode;
 	IDtoEX.fields2 = offset;
 	IDtoEX.fields3 = op2;
 	IDtoEX.fields4 = Rn;
 	IDtoEX.fields5 = Rt;

 	IDtoEX.instr_type = 'D';
}

void B_decoder(int instruct_no){

	//TODO: FLUSH
  //TMP_STALL_IF = true;
  //ID_FLUSH = true;

	//nonconditional immediate branching if 6 bit opcode is 0x05
	if ((((unsigned) instruct_no) >> 26) == 0x00000005) {
		int br_addr = instruct_no & 0x03FFFFFF;
		int opcode = ((unsigned)instruct_no) >> 26;

		IDtoEX.fields1 = opcode;
   	IDtoEX.fields2 = br_addr;
   	IDtoEX.fields3 = 0;
   	IDtoEX.fields4 = 0;
   	IDtoEX.fields5 = 0;

    	IDtoEX.instr_type = 'B';
	}
	//nonconditional register branching if 7 bit opcode 0x6b
	else if ((((unsigned)instruct_no) >> 25) == 0x0000006b) {
    //NOTE: instr_type and fields will be set in R_decoder
		R_decoder(instruct_no);
	}
	//otherwise it's conditional branching, with 8-bit op codes
	else {
		int Rt = instruct_no & 0x0000001F;
		instruct_no >>= 5;
		int br_addr = instruct_no & 0x0007FFFF;
		instruct_no >>= 19;
		int opcode = instruct_no & 0x000000FF;

		IDtoEX.fields1 = opcode;
    	IDtoEX.fields2 = br_addr;
    	IDtoEX.fields3 = Rt;
    	IDtoEX.fields4 = 0;
    	IDtoEX.fields5 = 0;

    	IDtoEX.instr_type = 'C';
	}
}


void decode(int instruct_no)
{
	printf("initial decoding\n");
	//unique commands
	if(instruct_no == 0xd4400000){ // HLT
		//printf("HLT command detected");
		IDtoEX.HALT_FLAG = 1;
		IF_FLUSH = true;
		CURRENT_STATE.PC +=4;
		return;
	}


	unsigned int op0 = ((unsigned)(instruct_no << 3) >> 28);

	unsigned int first_3 = op0 >> 1;
	unsigned int last_3 = op0 & 0x00000007;

	//printf("first_3: %03x | last_3: %03x \n", first_3, last_3);

	//Branches, Exception Generating and System Instructions
	if(first_3 == 5){
		B_decoder(instruct_no);
    	return;
	}
	//Data Processing -- Immediate
	else if(first_3 == 4){
		//printf("I decoding...\n");
		I_decoder(instruct_no);
    	return;
	}
	//Unallocated
	else if((first_3 >> 1) == 0){
		printf("unallocated\n");
	}
	//Data Processing -- Register
	else if(last_3 == 5){
		//printf("R decoding...\n");
		R_decoder(instruct_no);
    	return;
	}
	//Data Processing -- Scalar Floating-Point and Advanced SIMD
	else if(last_3 == 7){
		printf("data processing - scalar floating point\n");
	}
	//Loads and Stores
	else{
		D_decoder(instruct_no);
    	return;
	}

	printf("ERROR: Unrecognized Command.\n");
	RUN_BIT = 0;
}




void pipe_stage_wb()
{
  if(!WB_STALL){
	  if(!WB_FLUSH){
		  printf("retiring instruction...\n");
		  stat_inst_retire++;
		  if(MEMtoWB.HALT_FLAG){
			   RUN_BIT = 0;
			   return;
		  }
		  CURRENT_STATE.FLAG_Z = MEMtoWB.FLAG_Z;
		  CURRENT_STATE.FLAG_N = MEMtoWB.FLAG_N;
		  if(MEMtoWB.reg_write) {
			   printf("WB result: %16lx\n", MEMtoWB.result);
			   printf("WB dest reg: %d\n", MEMtoWB.dest_register);
			   CURRENT_STATE.REGS[MEMtoWB.dest_register] = MEMtoWB.result;
         CURRENT_STATE.REGS[31] = 0;
	    }
	  }
	  else{
		  printf("[FLUSHED] WB result: %16lx\n", MEMtoWB.result);
		  printf("[FLUSHED] WB dest reg: %d\n", MEMtoWB.dest_register);
	  }
  }
}

void pipe_stage_mem()
{
  if(!MEM_STALL){
	  if(!MEM_FLUSH){
		  MEMtoWB.HALT_FLAG = EXtoMEM.HALT_FLAG;
		  MEMtoWB.FLAG_Z = EXtoMEM.FLAG_Z;
		  MEMtoWB.FLAG_N = EXtoMEM.FLAG_N;
		  MEMtoWB.result = EXtoMEM.result;
		  MEMtoWB.dest_register = EXtoMEM.dest_register;
		  MEMtoWB.reg_write = EXtoMEM.reg_write;
		  MEMtoWB.mem_to_reg = EXtoMEM.mem_to_reg;

		  if(EXtoMEM.mem_read){
			   MEMtoWB.result = LDURx_MEM(EXtoMEM.mem_address, EXtoMEM.nbits);
		  } else if(EXtoMEM.mem_write){
			   STURx_MEM(EXtoMEM.mem_address, EXtoMEM.nbits, EXtoMEM.result);
		  }

		  printf("WB unFLUSHed\n");
		  WB_FLUSH = false;
	  }
	  else {
	  	  MEM_bubble();
	  }
  }
}

void pipe_stage_execute()
{
  if(!EX_STALL){
    if(!EX_FLUSH){
      EXtoMEM.HALT_FLAG = IDtoEX.HALT_FLAG;
      int fields[] = {IDtoEX.fields1, IDtoEX.fields2, IDtoEX.fields3, IDtoEX.fields4, IDtoEX.fields5};
      execute(IDtoEX.instr_type, fields);

	     if(!TMP_STALL_EX){
	  	     printf("MEM unFLUSHed\n");
      	   MEM_FLUSH = false;
	     }
	     else
		      TMP_STALL_EX = false;
    }
    else{
	  EX_bubble();
	}
  }
}

void pipe_stage_decode()
{
  if(!ID_STALL){
    if(!ID_FLUSH){
      IDtoEX.PC = IFtoID.PC;
      IDtoEX.target = IFtoID.target;
      decode(IFtoID.instruction);
	    printf("EX unFLUSHed\n");
	    EX_FLUSH = false;
	  }
	  else{
		  ID_bubble();
	  }
  }
}

void pipe_stage_fetch()
{
  if(!IF_STALL){
    if(!IF_FLUSH){
      if(!TMP_STALL_IF){
 		     uint32_t temp = mem_read_32(CURRENT_STATE.PC);
		    //printf("%08x \n", temp);
		    IFtoID.instruction = temp;
        printf("incrementing PC...\n");
        //CURRENT_STATE.PC += 4;
        IFtoID.PC = CURRENT_STATE.PC;
        bp_predict(CURRENT_STATE.PC);
        IFtoID.target = CURRENT_STATE.PC;

		    printf("ID unFLUSHed\n");
		    ID_FLUSH = false;
      }
      else{
        TMP_STALL_IF = false;

        if(EXtoMEM.branch)
          IF_bubble();
        else
          ID_STALL = false;

      }

	  }
	  else{
		  IF_bubble();
	  }
  }
}

void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
}
