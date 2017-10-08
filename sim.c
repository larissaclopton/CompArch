#include <stdio.h>
#include "shell.h"

#define ARM_REGS 32

uint32_t fetch(uint64_t PC)
{
	uint32_t temp = mem_read_32(PC);
	printf("%08x \n", temp);
	return temp;
}

// Function implementations

void ADDx(char instr_type, int set_flag, int fields[]){
	
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
                else
                        NEXT_STATE.FLAG_N = 0;

                if(temp == CURRENT_STATE.REGS[31])
                        NEXT_STATE.FLAG_Z = 1;
                else
                        NEXT_STATE.FLAG_Z = 0;
	}
		
}

void SUBx(char instr_type, int set_flag, int fields[]){

	int64_t temp;	

	if(instr_type == 'R'){
		printf("%ld - %ld", CURRENT_STATE.REGS[fields[3]], CURRENT_STATE.REGS[fields[1]]);
		temp = (int64_t)CURRENT_STATE.REGS[fields[3]] - (int64_t)CURRENT_STATE.REGS[fields[1]];
		NEXT_STATE.REGS[fields[4]] = temp;
	}
	else{
		printf("%ld - % ld", (int64_t)CURRENT_STATE.REGS[fields[2]], (int64_t)fields[1]);
		temp = (int64_t)CURRENT_STATE.REGS[fields[2]] - (int64_t)fields[1];
		NEXT_STATE.REGS[fields[3]] = temp;
	}

	printf("temp %ld", temp);

	if(set_flag){
		if(temp < CURRENT_STATE.REGS[31])
                        NEXT_STATE.FLAG_N = 1;
                else
                        NEXT_STATE.FLAG_N = 0;

                if(temp == CURRENT_STATE.REGS[31])
                        NEXT_STATE.FLAG_Z = 1;
                else
                        NEXT_STATE.FLAG_Z = 0;
	}

}

void ANDx(int set_flag, int fields[]) {

	int64_t temp = CURRENT_STATE.REGS[fields[3]] & CURRENT_STATE.REGS[fields[1]];
	NEXT_STATE.REGS[fields[4]] = temp;

	if(set_flag){
		if(temp < CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_N = 1;
		else
			NEXT_STATE.FLAG_N = 0;

		if(temp == CURRENT_STATE.REGS[31])
			NEXT_STATE.FLAG_Z = 1;
		else
			NEXT_STATE.FLAG_Z = 0;
	}			
}

void MUL(int fields[]){

	int64_t temp;
	temp = (CURRENT_STATE.REGS[fields[3]] * CURRENT_STATE.REGS[fields[1]]);
	printf("temp %ld", temp);
	NEXT_STATE.REGS[fields[4]] = temp;

}

void LDURx(int nbits, int fields[]){

	uint64_t start_addr = fields[3] + uint64_t(fields[1]);
	if(nbits == 64){
		uint64_t lower32 = (uint64_t)mem_read_32(start_addr);
		uint64_t upper32 = (uint64_t)mem_read_32(start_addr + 32);
		NEXT_STATE.REGS[fields[4]] = (upper32 << 32) + lower32;
	}
	else if(nbits == 16){
		uint64_t val = (uint64_t)mem_read_32(start_addr);
      NEXT_STATE.REGS[fields[4]] = val & 0xFFFF; // gets bottom 16 bits
	}
	else { // nbits = 8
		uint64_t val = (uint64_t)mem_read_32(start_addr);
		NEXT_STATE.REGS[fields[4]] = val & 0xFF; // gets bottom 8 bits	
	}

}

void CBNZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B

	if(CURRENT_STATE.REGS[fields[2]] != CURRENT_STATES.REGS[31]) {
		NEXT_STATE.PC = CURRENT_STATE.PC + addr;
	}
	else {
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

}

void CBZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B

	if(CURRENT_STATE.REGS[fields[2]] == CURRENT_STATE.REGS[31]) {
		NEXT_STATE.PC = CURRENT_STATE.PC + addr;
	}
	else {
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

}

void LSx(int fields[]){

	val = uint64_t(CURRENT_STATE.REGS[fields[3]]);

	if(fields[2] == 0x3F){
		NEXT_STATE.REGS[fields[4]] = val << fields[2];
	}
	else {
		NEXT_STATE.REGS[fields[4]] = val >> fields[2];
	}

}

void MOVZ(int fields[]){

	uint64_t val = (uint64_t) fields[2];

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

	printf("MOVZ val %ld", val);
	NEXT_STATE.REGS[fields[3]] = val;

}


// Branching functions

void BR(int fields[]){

	NEXT_STATE.PC = CURRENT_STATE.REGS[fields[3]];

}



// Execution implementation

void execute(char inst_type, int fields[])
{
	switch (inst_type){
		case 'R': {

			printf("case R\n");
			switch(fields[0]) {

            	case 0x458: { // ADD
            	   	ADDx('R', 0, fields);
				} break;
               	case 0x558: { // ADDS
					printf("ADDS ... \n");
					ADDx('R', 1, fields);
				} break;
               	case 0x450: { // AND
                	ANDx(0, fields);	
				} break;
               	case 0x750: { // ANDS
			ANDx(1, fields);
                } break;
               	case 0x650: { // EOR
                } break;
               	case 0x550: { // ORR
                } break;
               	case 0x658: { // SUB
			SUBx('R', 0, fields);
                } break;
		case 0x758: { // SUBS
			printf("SUBS ... \n");
			SUBx('R', 1, fields);
		} break;
               	case 0x4D8: { // MUL
			MUL(fields);
                } break;
		case 0x6b0: { // BR
			BR(fields);
		} break;
        	}

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;
 
		} break;
		case 'I': {
			printf("case I\n");
			switch(fields[0]) {
		
				case 0x244: // ADDI 
				case 0x489: { // ADDI
					ADDx('I', 0, fields);
				} break;
				case 0x2c4: // ADDIS ... I'm confused on how to handle the range here...
				case 0x589: { // ADDIS ... 589 shifted all the way right becomes 2c4 as well?
					printf("ADDIS ... \n");
					ADDx('I', 1, fields);
				} break;
				case 0x3c4: // SUBI
				case 0x689: { // SUBI
					SUBx('I', 0, fields);
				} break;
				case 0x788: // SUBIS
				case 0x789: { // SUBIS
					SUBx('I', 1, fields);
				} break; 
			}

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;		
		} break;
		case 'D': {
			printf("case D\n");
			switch(fields[0]) {
			
				case 0x7C0: { // STUR
					//STURx(64, fields);
				} break;
				case 0x3C0: { // STURH
					//STURx(16, fields);
				} break;
				case 0x1C0: { // STURB
					//STURx(8, fields);
				case 0x7C2: { // LDUR
					LDURx(64, fields);
				} break;
				case 0x3C2: { // LDURH
					LDURx(16, fields);
				} break;
				case 0x1C2: // LDURB
					LDURx(8, fields);
				} break;
				
			} 
		
			NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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
	int64_t immediate = instruct_no & 0x00000FFF;
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
	int offset = instruct_no & 0x000001FF;
	instruct_no >>= 9;
	int opcode = instruct_no & 0x000007FF;

	int fields[] = {opcode, offset, op2, Rn, Rt};

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
		D_decoder(instruct_no);
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
