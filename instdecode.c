/*
 * instdecode.c
 *
 * Created: 1/3/2017 2:54:46 PM
 *  Author: Neil Klingensmith
 */ 

#include "instdecode.h"

void instDecode(struct inst *instruction, uint16_t *location){
	uint16_t encoding = *location; // Get the opcode
	
	instruction->nbytes = 2;
	
	// Decode the instruction type. Start with the longest opcode masks.
	if((encoding & THUMB_MASK_BRANCH) == THUMB_OPCODE_BRANCH){
		// Branch instruction
		instruction->type = THUMB_TYPE_BRANCH;
	}else if ((encoding & THUMB_MASK_UNDEF) == THUMB_OPCODE_UNDEF){
		// Undefined instruction
		instruction->type = THUMB_TYPE_BRANCH;
	}else if ((encoding & THUMB_MASK_SYSCALL) == THUMB_OPCODE_SYSCALL){
		// Syscall instruction
		instruction->type = THUMB_TYPE_SYSCALL;
	}else if ((encoding & THUMB_MASK_ADDSUBREG) == THUMB_OPCODE_ADDSUBREG){
		// Add/subtract register
		instruction->type = THUMB_TYPE_ADDSUBREG;
	}else if ((encoding & THUMB_MASK_ADDSUBIMM) == THUMB_OPCODE_ADDSUBIMM){
		// Add/subtract immediate
		instruction->type = THUMB_TYPE_ADDSUBIMM;
	}else if ((encoding & THUMB_MASK_DATAPROCREG) == THUMB_OPCODE_DATAPROCREG){
		// Data processing register
		instruction->type = THUMB_TYPE_DATAPROCREG;
	}else if ((encoding & THUMB_MASK_SPECIALDATAPROC) == THUMB_OPCODE_SPECIALDATAPROC){
		// Special data processing instructions
		instruction->type = THUMB_TYPE_SPECIALDATAPROC;
	}else if ((encoding & THUMB_MASK_LOADLITERAL) == THUMB_OPCODE_LOADLITERAL){
		// Load from literal pool
		instruction->type = THUMB_TYPE_LOADLITERAL;
	}else if ((encoding & THUMB_MASK_BRUNCOND) == THUMB_OPCODE_BRUNCOND){
		// Unconditional branch
		instruction->type = THUMB_TYPE_BRUNCOND;
	}else if ((encoding & THUMB_MASK_32BINSTA) == THUMB_OPCODE_32BINSTA){
		// 32-bit instructions
		instruction->type = THUMB_TYPE_32BINSTA;
		instruction->nbytes = 4;
	}else if ((encoding & THUMB_MASK_LDSTREG) == THUMB_OPCODE_LDSTREG){
		// Load/store register offset instructions
		instruction->type = THUMB_TYPE_LDSTREG;
	}else if ((encoding & THUMB_MASK_LDSTHALFWORD) == THUMB_OPCODE_LDSTHALFWORD){
		// Load/store halfword immediate offset
		instruction->type = THUMB_TYPE_LDSTHALFWORD;
	}else if ((encoding & THUMB_MASK_LDSTSTACK) == THUMB_OPCODE_LDSTSTACK){
		// Load/store from/to stack
		instruction->type = THUMB_TYPE_LDSTSTACK;
	}else if ((encoding & THUMB_MASK_ADDSPPC) == THUMB_OPCODE_ADDSPPC){
		// Add to SP or PC
		instruction->type = THUMB_TYPE_ADDSPPC;
	}else if ((encoding & THUMB_MASK_MISC) == THUMB_OPCODE_MISC){
		// Misc instructions
		instruction->type = THUMB_TYPE_MISC;
	}else if ((encoding & THUMB_MASK_LDSTM) == THUMB_OPCODE_LDSTM){
		// Load/store multiple
		instruction->type = THUMB_TYPE_LDSTM;
	}else if ((encoding & THUMB_MASK_BRCOND) == THUMB_OPCODE_BRCOND){
		// Conditional branch
		instruction->type = THUMB_TYPE_BRCOND;
	}else if ((encoding & THUMB_MASK_32BINSTB) == THUMB_OPCODE_32BINSTB){
		// 32-bit instructions
		instruction->type = THUMB_TYPE_32BINSTB;
		instruction->nbytes = 4;
	}else if ((encoding & THUMB_MASK_SHIFTIMM) == THUMB_OPCODE_SHIFTIMM){
		// Shift immediate
		instruction->type = THUMB_TYPE_SHIFTIMM;
	}else if ((encoding & THUMB_MASK_ADDSUBCMPIMM) == THUMB_OPCODE_ADDSUBCMPIMM){
		// Add/subtract/compare immediate
		instruction->type = THUMB_TYPE_ADDSUBCMPIMM;
	}else if ((encoding & THUMB_MASK_LDSTWORDBYTE) == THUMB_OPCODE_LDSTWORDBYTE){
		// Load/store word/byte immediate offset
		instruction->type = THUMB_TYPE_LDSTWORDBYTE;
	}
}

