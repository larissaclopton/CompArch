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
  int fields[5];
  char instr_type;
  // bool ALU_op1;
  // bool ALU_op0;
  // bool ALU_src;
  // bool branch;
  // bool mem_read;
  // bool mem_write;
  // bool reg_write;
  // bool mem_to_reg;
} Pipe_Reg_IDtoEX;

static Pipe_Reg_IDtoEX IDtoEX;

typedef struct Pipe_Reg_EXtoMEM{
  int64_t result;
  bool branch;
  bool mem_read;
  bool mem_write;
  bool reg_write;
  bool mem_to_reg;
} Pipe_EXtoMEM;

static Pipe_Reg_EXtoMEM EXtoMEM;



// Execution implementation
void execute(char inst_type, int fields[])
{
  //int branch_reg = 0;
  switch (inst_type){
    case 'R': {
      //printf("case R\n");
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
                  //branch_reg = 1;
                } break;
      }

      //don't touch PC if branching has occurred
      // if(!branch_reg)
      // 	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    } break;
    case 'I': {
      //printf("case I\n");
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
      //printf("case D\n");
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

	IDtoEX.fields[0] = opcode;
  IDtoEX.fields[1] = Rm;
  IDtoEX.fields[2] = shamt;
  IDtoEX.fields[3] = Rn;
  IDtoEX.fields[4] = Rd;

	//int i;
	//for(i = 0; i < 5; i++){
	//	printf("%08x \n", fields[i]);
	//}
  IDtoEX.instr_type = 'R';
}

void I_decoder(int instruct_no)
{

	int five_bit_mask = 0x0000001F;
	int fields[5];

	//special case for MOVZ
	if((unsigned)instruct_no >> 23 == 0x000001A5){
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int imm = instruct_no & 0x0000FFFF;
		instruct_no >>= 16;
		int op2 = instruct_no & 0x00000003;
		instruct_no >>= 2;
		int opcode = instruct_no & 0x000001FF;

		IDtoEX.fields[0] = opcode;
		IDtoEX.fields[1] = op2;
		IDtoEX.fields[2] = imm;
		IDtoEX.fields[3] = Rd;
    IDtoEX.fields[4] = 0;
	}
	else{
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int Rn = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int immediate = instruct_no & 0x00000FFF;
		instruct_no >>= 12;
		int opcode = instruct_no & 0x000003FF;

		IDtoEX.fields[0] = opcode;
		IDtoEX.fields[1] = immediate;
		IDtoEX.fields[2] = Rn;
		IDtoEX.fields[3] = Rd;
    IDtoEX.fields[4] = 0;

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

	IDtoEX.fields[0] = opcode;
  IDtoEX.fields[1] = offset;
  IDtoEX.fields[2] = op2;
  IDtoEX.fields[3] = Rn;
  IDtoEX.fields[4] = Rt;

  IDtoEX.instr_type = 'D';
}

void B_decoder(int instruct_no){

	//nonconditional immediate branching if 6 bit opcode is 0x05
	if ((((unsigned) instruct_no) >> 26) == 0x00000005) {
		int br_addr = instruct_no & 0x03FFFFFF;
		int opcode = ((unsigned)instruct_no) >> 26;

		IDtoEX.fields[0] = opcode;
    IDtoEX.fields[1] = br_addr;
    IDtoEX.fields[2] = 0;
    IDtoEX.fields[3] = 0;
    IDtoEX.fields[4] = 0;

    IDtoEX.instr_type = 'B';
	}
	//nonconditional register branching if 7 bit opcode 0x6b
	else if ((((unsigned)instruct_no) >> 25) == 0x0000006b) {
    //NOTE: instr_type and fields will be set in R_decoder
		R_decoder(instruct_no);
    return;
	}
	//otherwise it's conditional branching, with 8-bit op codes
	else {
		int Rt = instruct_no & 0x0000001F;
		instruct_no >>= 5;
		int br_addr = instruct_no & 0x0007FFFF;
		instruct_no >>= 19;
		int opcode = instruct_no & 0x000000FF;

		IDtoEX.fields[0] = opcode;
    IDtoEX.fields[1] = br_addr;
    IDtoEX.fields[2] = Rt;
    IDtoEX.fields[3] = 0;
    IDtoEX.fields[4] = 0;

    IDtoEX.instr_type = 'C';
	}
}


void decode(int instruct_no)
{
	printf("initial decoding\n");
	//unique commands
	if(instruct_no == 0xd4400000){ // HLT
		//printf("HLT command detected");
		RUN_BIT = 0;
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
}

void pipe_stage_mem()
{
}

void pipe_stage_execute()
{
  execute(IDtoEX.inst_type, IDtoEX.fields);
}

void pipe_stage_decode()
{
  decode(IFtoID.instruction);
}

void pipe_stage_fetch()
{
  uint32_t temp = mem_read_32(CURRENT_STATE.PC);
	printf("%08x \n", temp);
	IFtoID.instruction = temp;
  CURRENT_STATE.PC += 4;
}

void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
}
