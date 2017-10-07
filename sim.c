#include <stdio.h>
#include "shell.h"

#define ARM_REGS 32

unsigned int fetch(uint64_t PC)
{
	int temp = mem_read_32(PC);
	printf("%08x \n", temp);
	return temp;
}




int ADDx(char instr_type, int set_flag, int *fields){
	int64_t temp;	

	if(instr_type == 'R'){
		temp = CURRENT_STATE.REGS[fields[1]] + CURRENT_STATE.REGS[fields[3]];
		NEXT_STATE.REGS[fields[4]] = temp;
	}
	else{
		temp = fields[1] + CURRENT_STATE.REGS[fields[2]];
		NEXT_STATE.REGS[fields[3]] = temp;
	}

	if(set_flag){
		if(temp < CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_N = 1;
		else if(temp == CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_Z = 0;
	}
		
}

int SUBx(char instr_type, int set_flag, int *fields){
	int64_t temp;	

	if(instr_type == 'R'){
		temp = CURRENT_STATE.REGS[fields[3]] - CURRENT_STATE.REGS[fields[1]];
		NEXT_STATE.REGS[fields[4]] = temp;
	}
	else{
		temp = CURRENT_STATE.REGS[fields[2]] - fields[1];
		NEXT_STATE.REGS[fields[3]];
	}

	if(set_flag){
		if(temp < CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_N = 1;
		else if(temp == CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_Z = 0;
	}
}


void execute(char inst_type, int fields[])
{
	switch (inst_type){
		case 'R': {

			printf("case R\n");
			switch(fields[0]) {

            	case 0x00000458: { // ADD
            	   	ADDx('R', 0, fields);
				} break;
               	case 0x00000558: { // ADDS
					printf("ADDS ... \n");
					ADDx('R', 1, fields);
				} break;
               	case 0x00000450: { // AND
                
				} break;
               	case 0x00000750: { // ANDS
                } break;
               	case 0x00000650: { // EOR
                } break;
               	case 0x00000550: { // ORR
                } break;
               	case 0x00000658: { // SUB
                } break;
				case 0x00000758: { // SUBS
				} break;
               	case 0x000004D8: { // MUL
                } break;
        	}

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;
 
		} break;
		case 'I': {
			switch(fields[0]) {
		
				case 0x00000488: // ADDI 
				case 0x00000489: { // ADDI
				} break;
				case 0x000002c4: // ADDIS ... I'm confused on how to handle the range here...
				case 0x00000589: { // ADDIS ... 589 shifted all the way right becomes 2c4 as well?
					printf("ADDIS ... \n");
					ADDx('I', 1, fields);
				} break;
				case 0x00000688: // SUBI
				case 0x00000689: { // SUBI
				} break;
				case 0x00000788: // SUBIS
				case 0x00000789: { // SUBIS
				} break; 
			}

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;		
		} break;
		case 'D': {

		} break;
	
	}

	return;
}




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

	int fields[] = {opcode, Rm, shamt, Rn, Rd};
	
	int i;
	for(i = 0; i < 5; i++){
		printf("%08x \n", fields[i]);
	}
	execute('R', fields);

}

void I_decoder(int instruct_no)
{

	int five_bit_mask = 0x0000001F;

	int Rd = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int Rn = instruct_no & five_bit_mask;
	instruct_no >>= 5;
	int immediate = instruct_no & 0x00000FFF;
	instruct_no >>= 12;
	int opcode = instruct_no & 0x000003FF;

	int fields[] = {opcode, immediate, Rn, Rd};

	int i;	
	for(i = 0; i < 4; i++){
         printf("%08x \n", fields[i]);
     }	

	execute('I', fields);


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
	int address = instruct_no & 0x000001FF;
	instruct_no >>= 9;
	int opcode = instruct_no & 0x000007FF;

	int fields[] = {opcode, address, op2, Rn, Rt};

	execute('D', fields);

}


void decode(int instruct_no)
{

	if(instruct_no == 0xd4400000){
		printf("HLT command detected");
		RUN_BIT = 0;
		return;
	}


	unsigned int op0 = ((unsigned)(instruct_no << 3) >> 28);

	unsigned int first_3 = op0 >> 1;
	unsigned int last_3 = op0 & 0x00000007;

	printf("first_3: %03x | last_3: %03x \n", first_3, last_3);

	//Branches, Exception Generating and System Instructions
	if(first_3 == 5){
		//B_decoder(instruct_no);
		
	}
	//Data Processing -- Immediate
	else if(first_3 == 4){
		printf("I decoding...\n");
		I_decoder(instruct_no);
	}
	//Unallocated
	else if((first_3 >> 1) == 0){
	}
	//Data Processing -- Register
	else if(last_3 == 5){
		printf("R decoding...\n");
		R_decoder(instruct_no);
	}
	//Data Processing -- Scalar Floating-Point and Advanced SIMD
	else if(last_3 == 7){
	}
	//Loads and Stores
	else{
		//D_decoder(instruct_no);
	}
}

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    int instruct_no = fetch(CURRENT_STATE.PC);
    decode(instruct_no);
    //execute();
}
