/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>



/* global pipeline state */
CPU_State CURRENT_STATE;

void pipe_init()
{
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;
}


typedef struct Pipe_Reg_IFtoID{
	int instruction;
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

bool IF_STALL = false;
bool ID_STALL = true;
bool EX_STALL = true;
bool MEM_STALL = true;
bool WB_STALL = true;

// Function implementations

void ADDx(char instr_type, int set_flag, int fields[]){
	int64_t temp;

	if(instr_type == 'R'){
		// TODO: check if registers are waiting to be written
		temp = CURRENT_STATE.REGS[fields[1]] + CURRENT_STATE.REGS[fields[3]];
		EXtoMEM.dest_register = fields[4];
	}
	else{
		//TODO: check if register is waiting to be written
		temp = fields[1] + CURRENT_STATE.REGS[fields[2]];
		EXtoMEM.dest_register = fields[3];
	}
	
	EXtoMEM.result = temp;
	printf("ADDx result: %16lx\n", EXtoMEM.result);
	printf("ADDx dest reg: %d\n", EXtoMEM.dest_register);

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
}

void SUBx(char instr_type, int set_flag, int fields[]){
	int64_t temp;

	if(instr_type == 'R'){
		//TODO: check if registers are waiting to be written
		temp = CURRENT_STATE.REGS[fields[3]] - CURRENT_STATE.REGS[fields[1]];
		EXtoMEM.dest_register = fields[4];
	}
	else{
		//TODO: check if register waiting to be written
		temp = (int64_t)CURRENT_STATE.REGS[fields[2]] - (int64_t)fields[1];
		EXtoMEM.dest_register = fields[3];
	}
	EXtoMEM.result = temp;

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

			//set 0-register to 0
			CURRENT_STATE.REGS[31] = 0;
	}
}

void ANDx(int set_flag, int fields[]) {
	//TODO: check if registers are waiting to be written
	int64_t temp = CURRENT_STATE.REGS[fields[3]] & CURRENT_STATE.REGS[fields[1]];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;

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
}

void MUL(int fields[]){
	//TODO: check if registers are waiting to be written
	int64_t temp = (CURRENT_STATE.REGS[fields[3]] * CURRENT_STATE.REGS[fields[1]]);
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;
}

void EOR(int fields[]){
	//TODO: check if registers are waiting to be written
	int64_t temp = CURRENT_STATE.REGS[fields[1]] ^ CURRENT_STATE.REGS[fields[3]];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;
}

void ORR(int fields[]){
	//TODO: check if registers are waiting to be written
	int64_t temp = CURRENT_STATE.REGS[fields[1]] | CURRENT_STATE.REGS[fields[3]];
	EXtoMEM.dest_register = fields[4];
	EXtoMEM.result = temp;
}

void BR(int fields[]){
	EXtoMEM.reg_write = false;
	EXtoMEM.branch = true;

	//TODO: check if register is waiting to be written
	//TODO: how to squash instructions already in pipeline?
	CURRENT_STATE.PC = CURRENT_STATE.REGS[fields[3]];
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
}

void LSx(int fields[]){
	int shamt = fields[1] & 0x3F;
	int right_shift = (fields[1] >> 6) & 0x3F;
	int mask = (-1) << 6;
	int left_shift = right_shift | mask;
	left_shift = (~left_shift) + 1;

	//TODO: check if register is waiting to be written
	uint64_t val = (uint64_t)CURRENT_STATE.REGS[fields[2]];

	if(shamt == 0x3F){
		val >>= right_shift;
	}
	else {
		val <<= left_shift;
	}

	EXtoMEM.dest_register = fields[3];
	EXtoMEM.result = (int64_t)val;

}

void STURx(int nbits, int fields[]){

	EXtoMEM.dest_register = 0;
	EXtoMEM.mem_read = false;
	EXtoMEM.mem_write = true;
	EXtoMEM.reg_write = false;
	EXtoMEM.mem_to_reg = false;

	//TODO: check if register is waiting to be written
	uint64_t start_addr = CURRENT_STATE.REGS[fields[3]] + fields[1];
	//TODO: check if register is waiting to be written
	int64_t val = CURRENT_STATE.REGS[fields[4]];
	int64_t result;
	if(nbits == 64){
		result = val;
		//mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
		//mem_write_32(start_addr+4, (uint32_t)((val >> 32) & 0xFFFFFFFF));
	}
	else if(nbits == 16){
		//TODO: check if memory is waiting to be written
		uint32_t mem = mem_read_32(start_addr);
		int mask_16 = 0xFFFF << 16;
		mem = mem & mask_16;

		uint32_t val2 = (uint32_t)val & 0xFFFF;
		mem = mem + val2;
		result = (int64_t)mem;
		//mem_write_32(start_addr, mem);
	}
	else{ // bits = 8
		//TODO: check if memory is waiting to be written
		uint32_t mem = mem_read_32(start_addr);
		int mask_24 = 0xFFFFFF << 8;
		mem = mem & mask_24;

		uint32_t val2 = (uint32_t)val & 0xFF;
		mem = mem + val2;
		result = (int64_t)mem;
		//mem_write_32(start_addr, mem);
	}

	EXtoMEM.mem_address = start_addr;
	EXtoMEM.result = result;
	EXtoMEM.nbits = nbits;

}

void LDURx(int nbits, int fields[]){

	EXtoMEM.result = 0;
	EXtoMEM.mem_read = true;
	EXtoMEM.mem_write = false;
	EXtoMEM.mem_to_reg = true;
	EXtoMEM.reg_write = true;
	
	//TODO: check whether register is waiting to be written
	uint64_t start_addr = CURRENT_STATE.REGS[fields[3]] + fields[1];
	//printf("load start addr: %08lx\n", start_addr);
	//printf("load start addr+32: %08lx\n", start_addr + 4);
	if(nbits == 64){
		//printf("calling ldur\n");
		//uint64_t lower32 = (uint64_t)mem_read_32(start_addr);
		//should this be not 32, but 4?)
		//uint64_t upper32 = (uint64_t)mem_read_32(start_addr + 4);
		//printf("upper 32: %16lx \n", upper32);
		//printf("lower 32: %16lx \n", lower32);
		//NEXT_STATE.REGS[fields[4]] = (upper32 << 32) + (lower32);
	}
	else if(nbits == 16){
		//printf("calling ldurh\n");
		//uint64_t val = (uint64_t)mem_read_32(start_addr);
			//uint64_t temp = val & 0xFFFF;
			//printf("set reg %ld", temp);
			//NEXT_STATE.REGS[fields[4]] = val & 0xFFFF; // gets bottom 16 bits
	}
	else { // nbits = 8
		//printf("calling ldurb\n");
		//uint64_t val = (uint64_t)mem_read_32(start_addr);
		//NEXT_STATE.REGS[fields[4]] = val & 0xFF; // gets bottom 8 bits
	}

	EXtoMEM.mem_address = start_addr;
	EXtoMEM.nbits = nbits;
	EXtoMEM.dest_register = fields[4];

}

void BRANCH_IMM(int fields[]){
	//printf("branching IMM\n");
	int64_t addr = (((((int64_t)fields[1]) << 38 ) >> 38) << 2);
	CURRENT_STATE.PC = CURRENT_STATE.PC - 4 + addr;
	//printf("bar addr: %016lx\n", NEXT_STATE.PC);
	
}

void CBNZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B

	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	//TODO: check if register is waiting to be written
	if(CURRENT_STATE.REGS[fields[2]] != CURRENT_STATE.REGS[31]) {
		EXtoMEM.branch = true;
		CURRENT_STATE.PC = CURRENT_STATE.PC - 4 + addr;
	}
	else {
		//NEXT_STATE.PC = CURRENT_STATE.PC + 4;
		EXtoMEM.branch = false;
	}

}

void CBZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B
	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	//TODO: check if register is waiting to be written
	if(CURRENT_STATE.REGS[fields[2]] == CURRENT_STATE.REGS[31]) {
		EXtoMEM.branch = true;
		CURRENT_STATE.PC = CURRENT_STATE.PC - 4 + addr;
	}
	else {
		EXtoMEM.branch = false;
		//NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

}

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
		CURRENT_STATE.PC = CURRENT_STATE.PC - 4 + addr;
	}
	else{
		//printf("not branching...\n");
		EXtoMEM.branch = false;
		//NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}
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
		EXtoMEM.mem_address = 0;
		EXtoMEM.nbits = 0;
		EXtoMEM.branch = false; //NOTE: will need to be reset in BR
		EXtoMEM.mem_read = false;
		EXtoMEM.mem_write = false;
		EXtoMEM.reg_write = true; //NOTE: will need to be reset in BR
		EXtoMEM.mem_to_reg = false;		
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
		EXtoMEM.mem_address = 0;
		EXtoMEM.nbits = 0;
		EXtoMEM.branch = false;
		EXtoMEM.mem_read = false;
		EXtoMEM.mem_write = false;
		EXtoMEM.reg_write = true;
		EXtoMEM.mem_to_reg = false;
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
		EXtoMEM.branch = false;
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
		EXtoMEM.result = 0;
		EXtoMEM.dest_register = 0;
		EXtoMEM.mem_address = 0;
		EXtoMEM.mem_read = false;
		EXtoMEM.mem_write = false;
		EXtoMEM.reg_write = false;
		EXtoMEM.mem_to_reg = false;
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
		EXtoMEM.branch = true;
		EXtoMEM.result = 0;
		EXtoMEM.dest_register = 0;
		EXtoMEM.mem_address = 0;
		EXtoMEM.mem_read = false;
		EXtoMEM.mem_write = false;
		EXtoMEM.reg_write = false;
		EXtoMEM.mem_to_reg = false;
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

	//TODO: STALLLLLLLLL

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
		IF_STALL = true;
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
		}
	}
	else{
		printf("[STALLED] WB result: %16lx\n", MEMtoWB.result);
		printf("[STALLED] WB dest reg: %d\n", MEMtoWB.dest_register);
	}
}

void pipe_stage_mem()
{
	if(!MEM_STALL){
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

		printf("WB unstalled\n");
		WB_STALL = false;
	}
	else {
		MEMtoWB.result = 0;
		MEMtoWB.dest_register = 0;
		MEMtoWB.reg_write = 0;
		MEMtoWB.mem_to_reg = 0;
		printf("WB stalled\n");
		WB_STALL = true;
	}

}

void pipe_stage_execute()
{
	if(!EX_STALL){
		EXtoMEM.HALT_FLAG = IDtoEX.HALT_FLAG;
		int fields[] = {IDtoEX.fields1, IDtoEX.fields2, IDtoEX.fields3, IDtoEX.fields4, IDtoEX.fields5};
  		execute(IDtoEX.instr_type, fields);
		printf("MEM unstalled\n");
		MEM_STALL = false;
	}
	else{
		EXtoMEM.result = 0;
		EXtoMEM.dest_register = 0;
		EXtoMEM.mem_address = 0;
		EXtoMEM.nbits = 0; // for STURx and LDURx
 		EXtoMEM.branch = false;
 		EXtoMEM.mem_read = false;
 		EXtoMEM.mem_write = false;
 		EXtoMEM.reg_write = false;
 		EXtoMEM.mem_to_reg = false;
		printf("MEM stalled\n");
		MEM_STALL = true;
	}
}

void pipe_stage_decode()
{
	if(!ID_STALL){
  		decode(IFtoID.instruction);
		printf("EX unstalled\n");
		EX_STALL = false;
	}
	else{
		IDtoEX.fields1 = 0;
		IDtoEX.fields2 = 0;
		IDtoEX.fields3 = 0;
		IDtoEX.fields4 = 0;
		IDtoEX.fields5 = 0;
		IDtoEX.instr_type = '0';
		printf("EX stalled\n");
		EX_STALL = true;
	}
}

void pipe_stage_fetch()
{
	if(!IF_STALL){
 		uint32_t temp = mem_read_32(CURRENT_STATE.PC);
		//printf("%08x \n", temp);
		IFtoID.instruction = temp;
 		CURRENT_STATE.PC += 4;
		printf("ID unstalled\n");
		ID_STALL = false;
	}
	else{
		IFtoID.instruction = 0;
		printf("ID stalled\n");
		ID_STALL = true;
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
