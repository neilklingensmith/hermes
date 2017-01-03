/*
 * instdecode.c
 *
 * Created: 1/3/2017 2:54:46 PM
 *  Author: Neil Klingensmith
 */ 

#include "instdecode.h"

void instDecode(struct inst *instruction, uint16_t *location){
	uint16_t encoding = *location; // Get the opcode
	
	// Decode the instruction type. Start with the longest opcode masks.
	if((encoding & THUMB_MASK_BRANCH) == THUMB_OPCODE_BRANCH){
		// Branch instruction
	}else if ((encoding & THUMB_MASK_UNDEF) == THUMB_OPCODE_UNDEF){
		// Undefined instruction
	}else if ((encoding & THUMB_MASK_SYSCALL) == THUMB_OPCODE_SYSCALL){
		// Syscall instruction
	}else if ((encoding & THUMB_MASK_ADDSUBREG) == THUMB_OPCODE_ADDSUBREG){
		// Add/subtract register
	}else if ((encoding & THUMB_MASK_ADDSUBIMM) == THUMB_OPCODE_ADDSUBIMM){
		// Add/subtract immediate
	}else if ((encoding & THUMB_MASK_DATAPROCREG) == THUMB_OPCODE_DATAPROCREG){
		// Data processing register
	}else if ((encoding & THUMB_MASK_SPECIALDATAPROC) == THUMB_OPCODE_SPECIALDATAPROC){
		// Special data processing instructionss
	}else if ((encoding & THUMB_MASK_LOADLITERAL) == THUMB_OPCODE_LOADLITERAL){
		// Load from literal pool
	}else if ((encoding & THUMB_MASK_BRUNCOND) == THUMB_OPCODE_BRUNCOND){
		// Unconditional branch
	}else if ((encoding & THUMB_MASK_32BINSTA) == THUMB_OPCODE_32BINSTA){
		// 32-bit instructions
	}else if ((encoding & THUMB_MASK_LDSTREG) == THUMB_OPCODE_LDSTREG){
		// Load/store register offset instructions
	}else if ((encoding & THUMB_MASK_LDSTHALFWORD) == THUMB_OPCODE_LDSTHALFWORD){
		// Load/store halfword immediate offset
	}else if ((encoding & THUMB_MASK_LDSTSTACK) == THUMB_OPCODE_LDSTSTACK){
		// Load/store from/to stack
	}else if ((encoding & THUMB_MASK_ADDSPPC) == THUMB_OPCODE_ADDSPPC){
		// Add to SP or PC
	}else if ((encoding & THUMB_MASK_MISC) == THUMB_OPCODE_MISC){
		// Misc instructions
	}else if ((encoding & THUMB_MASK_LDSTM) == THUMB_OPCODE_LDSTM){
		// Load/store multiple
	}else if ((encoding & THUMB_MASK_BRCOND) == THUMB_OPCODE_BRCOND){
		// Conditional branch
	}else if ((encoding & THUMB_MASK_32BINSTB) == THUMB_OPCODE_32BINSTB){
		// 32-bit instructions
	}else if ((encoding & THUMB_MASK_SHIFTIMM) == THUMB_OPCODE_SHIFTIMM){
		// Shift immediate
	}else if ((encoding & THUMB_MASK_ADDSUBCMPIMM) == THUMB_OPCODE_ADDSUBCMPIMM){
		// Add/subtract/compare immediate
	}else if ((encoding & THUMB_MASK_LDSTWORDBYTE) == THUMB_OPCODE_LDSTWORDBYTE){
		// Load/store word/byte immediate offset
	}
}

