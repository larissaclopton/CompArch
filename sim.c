#include <stdio.h>
#include "shell.h"

#define ARM_REGS 32


// NOTE: Team Members
// Larissa Clopton cloptla
// Nathan Chortek	nchortek


uint32_t fetch(uint64_t PC)
{
	uint32_t temp = mem_read_32(PC);
	printf("%08x \n", temp);
	return temp;
}

// Function implementations

void ADDx(char instr_type, int set_flag, int fields[]){
	//printf("executing add...\n");
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
	printf("executing SUBx \n");
	int64_t temp;

	if(instr_type == 'R'){
		//printf("%ld - %ld", CURRENT_STATE.REGS[fields[3]], CURRENT_STATE.REGS[fields[1]]);
		temp = CURRENT_STATE.REGS[fields[3]] - CURRENT_STATE.REGS[fields[1]];
		NEXT_STATE.REGS[fields[4]] = temp;
	}
	else{
		//printf("%ld - % ld", (int64_t)CURRENT_STATE.REGS[fields[2]], (int64_t)fields[1]);
		temp = (int64_t)CURRENT_STATE.REGS[fields[2]] - (int64_t)fields[1];
		NEXT_STATE.REGS[fields[3]] = temp;
	}

	//printf("temp %ld", temp);

	if(set_flag){
		printf("setting flag after subx...\n");
		if(temp < CURRENT_STATE.REGS[31])
    	NEXT_STATE.FLAG_N = 1;
    else
      NEXT_STATE.FLAG_N = 0;

    if(temp == CURRENT_STATE.REGS[31])
      NEXT_STATE.FLAG_Z = 1;
    else
    	NEXT_STATE.FLAG_Z = 0;

			//set 0-register to 0
			NEXT_STATE.REGS[31] = 0;
	}

	printf("Subx done\n");
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
	//printf("temp %ld", temp);
	NEXT_STATE.REGS[fields[4]] = temp;


}

void LDURx(int nbits, int fields[]){

	uint64_t start_addr = CURRENT_STATE.REGS[fields[3]] + fields[1];
	//printf("load start addr: %08lx\n", start_addr);
	//printf("load start addr+32: %08lx\n", start_addr + 4);
	if(nbits == 64){
		//printf("calling ldur\n");
		uint64_t lower32 = (uint64_t)mem_read_32(start_addr);
		//should this be not 32, but 4?)
		uint64_t upper32 = (uint64_t)mem_read_32(start_addr + 4);
		//printf("upper 32: %16lx \n", upper32);
		//printf("lower 32: %16lx \n", lower32);
		NEXT_STATE.REGS[fields[4]] = (upper32 << 32) + (lower32);
	}
	else if(nbits == 16){
		//printf("calling ldurh\n");
		uint64_t val = (uint64_t)mem_read_32(start_addr);
			//uint64_t temp = val & 0xFFFF;
			//printf("set reg %ld", temp);
			NEXT_STATE.REGS[fields[4]] = val & 0xFFFF; // gets bottom 16 bits
	}
	else { // nbits = 8
		//printf("calling ldurb\n");
		uint64_t val = (uint64_t)mem_read_32(start_addr);
		NEXT_STATE.REGS[fields[4]] = val & 0xFF; // gets bottom 8 bits
	}

}

void STURx(int nbits, int fields[]){
	uint64_t start_addr = CURRENT_STATE.REGS[fields[3]] + fields[1];
	//printf("store start addr: %08lx\n", start_addr);
	//printf("store start addr+32: %08lx\n", start_addr+32);
	int64_t val = CURRENT_STATE.REGS[fields[4]];
	if(nbits == 64){
		//printf("calling stur\n");
		//uint32_t temp = (uint32_t)(val & 0x0000FFFF);
		//printf("store %d", temp);
		mem_write_32(start_addr, (uint32_t)(val & 0xFFFFFFFF));
		mem_write_32(start_addr+4, (uint32_t)((val >> 32) & 0xFFFFFFFF));
	}
	else if(nbits == 16){
		//printf("calling sturh\n");
		uint32_t mem = mem_read_32(start_addr);
		int mask_16 = 0xFFFF << 16;
		mem = mem & mask_16;

		uint32_t val2 = (uint32_t)val & 0xFFFF;
		mem = mem + val2;

		mem_write_32(start_addr, mem);
	}
	else{ // bits = 8
		//printf("calling sturb\n");
		uint32_t mem = mem_read_32(start_addr);
		int mask_24 = 0xFFFFFF << 8;
		mem = mem & mask_24;

		uint32_t val2 = (uint32_t)val & 0xFF;
		mem = mem + val2;

		mem_write_32(start_addr, mem);
	}
}

void CBNZ(int fields[]){

	// need to shift address to be 64 bits
	// left extend by 43, bottom 2 bits 0
	// could also call B

	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	if(CURRENT_STATE.REGS[fields[2]] != CURRENT_STATE.REGS[31]) {
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
	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	if(CURRENT_STATE.REGS[fields[2]] == CURRENT_STATE.REGS[31]) {
		NEXT_STATE.PC = CURRENT_STATE.PC + addr;
	}
	else {
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

}


void BR(int fields[]){
	//printf("executing BR\n");
	//printf("bar addr: %016lx\n", CURRENT_STATE.REGS[fields[3]]);
	NEXT_STATE.PC = CURRENT_STATE.REGS[fields[3]];

}

void BRANCH_IMM(int fields[]){
	printf("branching IMM\n");
	int64_t addr = (((((int64_t)fields[1]) << 38 ) >> 38) << 2);
	NEXT_STATE.PC = CURRENT_STATE.PC + addr;
	//printf("bar addr: %016lx\n", NEXT_STATE.PC);
}

// Branching functions

void B_COND(int fields[]){

	int64_t addr = (((((int64_t)fields[1]) << 45 ) >> 45) << 2);

	int cond = fields[2] & 0x0000000F;
	int branch = 0;

	switch (cond) {
		case 0x00000000: { //equal
			//printf("executing beq...\n");
			//if Z = 1,
			if(CURRENT_STATE.FLAG_Z == 1)
				branch = 1;
		} break;
		case 0x00000001: { // not equal
			//printf("executing bne...\n");
			if( CURRENT_STATE.FLAG_Z == 0)
				branch = 1;
		} break;
		case 0x0000000A: { // greater than or equal
			//printf("executing bge...\n");
			if(CURRENT_STATE.FLAG_N == 0)
				branch = 1;
		} break;
		case 0x0000000B: { // less than
			//printf("executing bl...\n");
			if(CURRENT_STATE.FLAG_N == 1)
				branch = 1;
		} break;
		case 0x0000000C: {//greater than
			//printf("executing bg...\n");
			if(CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0)
				branch = 1;
		} break;
		case 0x0000000D: {//less than or equal to
			//printf("executing ble...\n");
			if(!(CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0))
				branch = 1;
		} break;
	}

	if(branch){
		//printf("branching...\n");
		NEXT_STATE.PC = CURRENT_STATE.PC + addr;
	}
	else{
		//printf("not branching...\n");
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}
}

void LSx(int fields[]){
	//printf("executing lsx...\n");
	//int i;
	//for(i = 0; i < 4; i++){
	//     	printf("fields[%d]: %08x \n", i, fields[i]);
	 //	}

	int shamt = fields[1] & 0x03F;
	int right_shift = (fields[1] >> 6) & 0x03F;
	int mask = (-1) << 6;
	int left_shift = right_shift | mask;
	left_shift = (~left_shift) + 1;


	//printf("executing LSx\n");
	uint64_t val = (uint64_t)CURRENT_STATE.REGS[fields[2]];

	if(shamt == 0x3F){
		//printf("executing lsr\n");
		NEXT_STATE.REGS[fields[3]] = val >> right_shift;
	}
	else {
		//printf("executing lsl\n");
		NEXT_STATE.REGS[fields[3]] = val << left_shift;
	}

}

void MOVZ(int fields[]){
	//printf("executing movz...\n");
	uint64_t val = (uint64_t)fields[2];

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

	//printf("MOVZ val %ld", val);
	NEXT_STATE.REGS[fields[3]] = (int64_t)val;
}

void EOR(int fields[]){
	int64_t temp;
	temp = CURRENT_STATE.REGS[fields[1]] ^ CURRENT_STATE.REGS[fields[3]];
	NEXT_STATE.REGS[fields[4]] = temp;
}

void ORR(int fields[]){
	int64_t temp;
	temp = CURRENT_STATE.REGS[fields[1]] | CURRENT_STATE.REGS[fields[3]];
	NEXT_STATE.REGS[fields[4]] = temp;
}


// Execution implementation
void execute(char inst_type, int fields[])
{
	int branch_reg = 0;
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
									printf("SUBS ... \n");
									SUBx('R', 1, fields);
								} break;
               	case 0x4D8: { // MUL
									MUL(fields);
                } break;
								case 0x6B0: { // BR
									BR(fields);
									branch_reg = 1;
								} break;
			}

			//don't touch PC if branching has occurred
			if(!branch_reg)
				NEXT_STATE.PC = CURRENT_STATE.PC + 4;

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

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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

			NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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

	int fields[] = {opcode, Rm, shamt, Rn, Rd};

	//int i;
	//for(i = 0; i < 5; i++){
	//	printf("%08x \n", fields[i]);
	//}
	execute('R', fields);

}

void I_decoder(int instruct_no)
{

	int five_bit_mask = 0x0000001F;
	int fields[4];

	//special case for MOVZ
	if((unsigned)instruct_no >> 23 == 0x000001A5){
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int imm = instruct_no & 0x0000FFFF;
		instruct_no >>= 16;
		int op2 = instruct_no & 0x00000003;
		instruct_no >>= 2;
		int opcode = instruct_no & 0x000001FF;

		fields[0] = opcode;
		fields[1] = op2;
		fields[2] = imm;
		fields[3] = Rd;
	}
	else{
		int Rd = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int Rn = instruct_no & five_bit_mask;
		instruct_no >>= 5;
		int immediate = instruct_no & 0x00000FFF;
		instruct_no >>= 12;
		int opcode = instruct_no & 0x000003FF;

		fields[0] = opcode;
		fields[1] = immediate;
		fields[2] = Rn;
		fields[3] = Rd;

		//int i;
		//for(i = 0; i < 4; i++){
    //     	printf("%08x \n", fields[i]);
     //	}
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

void B_decoder(int instruct_no){
	//nonconditional immediate branching if 6 bit opcode is 0x05
	if ((((unsigned) instruct_no) >> 26) == 0x00000005) {
		int br_addr = instruct_no & 0x03FFFFFF;
		int opcode = ((unsigned)instruct_no) >> 26;

		int fields[] = {opcode, br_addr};

		execute('B', fields);
	}
	//nonconditional register branching if 7 bit opcode 0x6b
	else if ((((unsigned)instruct_no) >> 25) == 0x0000006b) {
		R_decoder(instruct_no);
	}
	//otherwise it's conditional branching, with 8-bit op codes
	else {
		int Rt = instruct_no & 0x0000001F;
		instruct_no >>= 5;
		int br_addr = instruct_no & 0x0007FFFF;
		instruct_no >>= 19;
		int opcode = instruct_no & 0x000000FF;

		int fields[] = {opcode, br_addr, Rt};

		execute('C', fields);
	}
}

void decode(int instruct_no)
{
	printf("initial decoding\n");
	//unique commands
	if(instruct_no == 0xd4400000){ // HLT
		//printf("HLT command detected");
		RUN_BIT = 0;
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
		return;
	}
	//else if((((unsigned)instruct_no) >> 22) == 0x34D ){ // LSx
	//	R_decoder(instruct_no);
	//	return;
	//}


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

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
		 printf("about to fetch\n");
    int instruct_no = fetch(CURRENT_STATE.PC);
		printf("about to decode\n");
    decode(instruct_no);
    //execute();
}
