/*
 * instdecode.c
 *
 * Created: 1/3/2017 2:54:46 PM
 *  Author: Neil Klingensmith
 */ 

#include "instdecode.h"
#include <string.h>



int instDecode(struct inst *instruction, uint16_t *location){
	uint16_t encoding = *location; // Get the opcode
	
	instruction->nbytes = 2;
	instruction->Rd = -1;
	instruction->Rm = -1;
	instruction->Rn = -1;
	
	// Decode the instruction type. Start with the longest opcode masks.
	if((encoding & THUMB_MASK_BRANCH) == THUMB_OPCODE_BRANCH){
		// Branch instruction
		instruction->type = THUMB_TYPE_BRANCH;
		instruction->Rm = (encoding>>3) & 0xf;
		if(((encoding>>7) & 1) == 1) {
			strcpy(instruction->mnemonic, "BLX");
		} else {
			strcpy(instruction->mnemonic, "BL");
		}
	}else if ((encoding & THUMB_MASK_UNDEF) == THUMB_OPCODE_UNDEF){
		// Undefined instruction
		instruction->type = THUMB_TYPE_BRANCH;
	}else if ((encoding & THUMB_MASK_SYSCALL) == THUMB_OPCODE_SYSCALL){
		// Syscall instruction
		instruction->type = THUMB_TYPE_SYSCALL;
	}else if ((encoding & THUMB_MASK_ADDSUBREG) == THUMB_OPCODE_ADDSUBREG){
		// Add/subtract register
		instruction->type = THUMB_TYPE_ADDSUBREG;
		instruction->Rm = (encoding>>6) & 7;
		instruction->Rn = (encoding>>3) & 7;
		instruction->Rd = encoding & 7;
		switch((encoding>>9) & 1)
		{
		case 0:
			strcpy(instruction->mnemonic, "ADD");
			break;
		case 1:
			strcpy(instruction->mnemonic, "SUB");
			break;
		}
	}else if ((encoding & THUMB_MASK_ADDSUBIMM) == THUMB_OPCODE_ADDSUBIMM){
		// Add/subtract immediate
		instruction->type = THUMB_TYPE_ADDSUBIMM;
		instruction->imm = (encoding>>6) & 7;
		instruction->Rn = (encoding>>3) & 7;
		instruction->Rd = encoding & 7;
		switch((encoding>>9) & 1)
		{
		case 0:
			strcpy(instruction->mnemonic, "ADD");
			break;
		case 1:
			strcpy(instruction->mnemonic, "SUB");
			break;
		}
	}else if ((encoding & THUMB_MASK_DATAPROCREG) == THUMB_OPCODE_DATAPROCREG){
		// Data processing register
		instruction->type = THUMB_TYPE_DATAPROCREG;
		instruction->Rm = (encoding>>3) & 7;
		instruction->Rd = encoding & 7;
		switch((encoding>>6) & 0xf)
		{
		case 0: // AND
			strcpy(instruction->mnemonic, "AND");
			break;
		case 1: // EOR
			strcpy(instruction->mnemonic, "EOR");
			break;
		case 2: // LSL
			strcpy(instruction->mnemonic, "LSL");
			break;
		case 3: // LSR
			strcpy(instruction->mnemonic, "LSR");
			break;
		case 4: // ASR
			strcpy(instruction->mnemonic, "ASR");
			break;
		case 5: // ADC
			strcpy(instruction->mnemonic, "ASC");
			break;
		case 6: // SBC
			strcpy(instruction->mnemonic, "SBC");
			break;
		case 7: // ROR
			strcpy(instruction->mnemonic, "ROR");
			break;
		case 8: // TST
			strcpy(instruction->mnemonic, "TST");
			break;
		case 9: // RSB
			strcpy(instruction->mnemonic, "RSB");
			break;
		case 10: // CMP
			strcpy(instruction->mnemonic, "CMP");
			break;
		case 11: // CMN
			strcpy(instruction->mnemonic, "CMN");
			break;
		case 12: // ORR
			strcpy(instruction->mnemonic, "ORR");
			break;
		case 13: // MUL
			strcpy(instruction->mnemonic, "MUL");
			break;
		case 14: // BIC
			strcpy(instruction->mnemonic, "BIC");
			break;
		case 15: // MVN
			strcpy(instruction->mnemonic, "MVN");
			break;
		}
	}else if ((encoding & THUMB_MASK_SPECIALDATAPROC) == THUMB_OPCODE_SPECIALDATAPROC){
		// Special data processing instructions
		instruction->type = THUMB_TYPE_SPECIALDATAPROC;
		instruction->Rd = encoding & 7;
		instruction->Rm = (encoding>>3) & 0xf;

		// Determine instruction variant
		switch((encoding >> 8) & 3)
		{
		case 0: // ADD
			strcpy(instruction->mnemonic, "ADD");
			break;
		case 1: // CMP
			strcpy(instruction->mnemonic, "CMP");
			break;
		case 2: // MOV
			strcpy(instruction->mnemonic, "MOV");
			break;
		default:
			return -1;
		}
	}else if ((encoding & THUMB_MASK_LOADLITERAL) == THUMB_OPCODE_LOADLITERAL){
		// Load from literal pool
		instruction->type = THUMB_TYPE_LOADLITERAL;
	}else if ((encoding & THUMB_MASK_BRUNCOND) == THUMB_OPCODE_BRUNCOND){
		// Unconditional branch
		instruction->type = THUMB_TYPE_BRUNCOND;
	}else if ((encoding & THUMB_MASK_32BINSTA) == THUMB_OPCODE_32BINSTA){
		// 32-bit instructions
		instruction->nbytes = 4;
		uint32_t encoding32 = *((uint32_t*)location);
		if((encoding32 & THUMB_MASK32_LDSTDOUBLE) == THUMB_OPCODE32_LDSTDOUBLE) {
			instruction->type = THUMB_TYPE_LDSTDOUBLE;
		} else if ((encoding32 & THUMB_MASK32_LDSTM) == THUMB_OPCODE32_LDSTM) {
			instruction->type = THUMB_TYPE_LDSTM32;
		}
	}else if ((encoding & THUMB_MASK_LDSTREG) == THUMB_OPCODE_LDSTREG){
		// Load/store register offset instructions
		instruction->type = THUMB_TYPE_LDSTREG;
		instruction->Rm = (encoding>>6) & 7;
		instruction->Rm = (encoding>>3) & 7;
		instruction->Rd = encoding & 7;
		switch((encoding>>9) & 7)
		{
		case 0: // STR
			strcpy(instruction->mnemonic, "STR");
			break;
		case 1: // STRH
			strcpy(instruction->mnemonic, "STRH");
			break;
		case 2: // STRB
			strcpy(instruction->mnemonic, "STRB");
			break;
		case 3: // LDRSB
			strcpy(instruction->mnemonic, "LDRSB");
			break;
		case 4: // LDR
			strcpy(instruction->mnemonic, "LDR");
			break;
		case 5: // LDRH
			strcpy(instruction->mnemonic, "LDRH");
			break;
		case 6: // LDRB
			strcpy(instruction->mnemonic, "LDRB");
			break;
		case 7: // LDRSH
			strcpy(instruction->mnemonic, "LDRSH");
			break;
		}
	}else if ((encoding & THUMB_MASK_LDSTHALFWORD) == THUMB_OPCODE_LDSTHALFWORD){
		// Load/store halfword immediate offset
		instruction->type = THUMB_TYPE_LDSTHALFWORD;
		instruction->Rd = encoding & 7;
		instruction->Rn = (encoding>>3) & 7;
		instruction->imm = (encoding>>6) & 0x1f;
		if(((encoding>>11)& 1) == 0){
			strcpy(instruction->mnemonic, "STRH");
		} else {
			strcpy(instruction->mnemonic, "LDRH");
		}
	}else if ((encoding & THUMB_MASK_LDSTSTACK) == THUMB_OPCODE_LDSTSTACK){
		// Load/store from/to stack
		instruction->type = THUMB_TYPE_LDSTSTACK;
		instruction->Rd = (encoding>>8) & 0x7;
		instruction->imm = encoding & 0xff;
		if(((encoding>>11) & 1) == 0) {
			strcpy(instruction->mnemonic, "STR");
		} else {
			strcpy(instruction->mnemonic, "LDR");
		}
	}else if ((encoding & THUMB_MASK_ADDSPPC) == THUMB_OPCODE_ADDSPPC){
		// Add to SP or PC
		instruction->type = THUMB_TYPE_ADDSPPC;
		instruction->Rd = (encoding>>8) & 7;
		instruction->imm = encoding & 0xff;
		if(((encoding>>11)& 1) == 0){
			strcpy(instruction->mnemonic, "ADR");
		} else {
			strcpy(instruction->mnemonic, "ADD");
		}
	}else if ((encoding & THUMB_MASK_MISC) == THUMB_OPCODE_MISC){
		// Misc instructions
		instruction->type = THUMB_TYPE_MISC;
	}else if ((encoding & THUMB_MASK_LDSTM) == THUMB_OPCODE_LDSTM){
		// Load/store multiple
		instruction->type = THUMB_TYPE_LDSTM;
		instruction->Rn = (encoding>>8) & 7;
		instruction->imm = encoding & 0xff; // Represents register list
		if(((encoding>>11)& 1) == 0){ // STMIA/STMEA
			strcpy(instruction->mnemonic, "STMIA");
		} else { // LDMIA/LDMFD
			strcpy(instruction->mnemonic, "LDMIA");
		}
	}else if ((encoding & THUMB_MASK_BRCOND) == THUMB_OPCODE_BRCOND){
		// Conditional branch
		instruction->type = THUMB_TYPE_BRCOND;
	}else if ((encoding & THUMB_MASK_32BINSTB) == THUMB_OPCODE_32BINSTB){
		// 32-bit instructions
		// TODO: Decode 32-bit instructions here
		uint32_t encoding32 = (((uint32_t)*location)<<16) | *(location+1);
		instruction->nbytes = 4;
		
		if((encoding32 & THUMB_MASK32_LDSTSINGLE) == THUMB_OPCODE32_LDSTSINGLE){
			instruction->type = THUMB_TYPE_LDSTSINGLE;
			instruction->Rt = (encoding32 >> 12) & 0xf;
			instruction->Rn = (encoding32 >> 16) & 0xf;
			if((instruction->Rn == 15) || (((encoding32 >> 23) & 1) == 1)){ // 12 bit immediate
				if(encoding32>>24 & 1){ // Sign extend?
					instruction->imm = (encoding32 & 0xfff) | ((encoding32 & 0x800) ? 0xfffff000 : 0); // Sign extend immediate
				} else {
					instruction->imm = (encoding32 & 0xfff); // zero extend immediate if S bit clear
				}
			} else { // 8 bit immediate
				if(encoding32>>24 & 1){ // Sign extend?
					instruction->imm = (encoding32 & 0xff) | ((encoding32 & 0x80) ? 0xffffff00 : 0); // Sign extend immediate
				} else {
					instruction->imm = (encoding32 & 0xff); // zero extend immediate if S bit clear
				}
			}
		} else if((encoding32 & THUMB_MASK32_LDSTDOUBLE) == THUMB_OPCODE32_LDSTDOUBLE){
			instruction->type = THUMB_TYPE_LDSTDOUBLE;
			instruction->Rt = (encoding32 >> 12) & 0xf;
			instruction->Rn = (encoding32 >> 16) & 0xf;
			if(((encoding32>>23) & 3) == 0) {
				instruction->Rd = (encoding32 >> 8) & 0xf;
			} else {
				instruction->Rm = encoding32 & 0xf;
				// LOAD AND STORE EXCLUSIVE BYTE, HALFWORD,DOUBLEWORD, AND TABLE BRANCH NOT IMPLEMENTED!!!
			}
			instruction->imm = encoding32 & 0xff;
		}

	}else if ((encoding & THUMB_MASK_SHIFTIMM) == THUMB_OPCODE_SHIFTIMM){
		// Shift immediate
		instruction->type = THUMB_TYPE_LDSTDOUBLE;
		instruction->Rd = encoding & 7;
		instruction->Rm = (encoding>>3) & 7;
		instruction->imm = (encoding>>6) & 0x1f;
		
		// Determine instruction variant
		switch((encoding >> 11) & 3)
		{
		case 0: // MOV/LSL
			if(instruction->imm == 0){
				strcpy(instruction->mnemonic, "MOV");
			} else {
				strcpy(instruction->mnemonic, "MOV");
			}
			break;
		case 1: // LSR
			strcpy(instruction->mnemonic, "LSR");
			break;
		case 2: // ASR
			strcpy(instruction->mnemonic, "ASR");
			break;
		default:
			return -1;
		
		}
	}else if ((encoding & THUMB_MASK_ADDSUBCMPIMM) == THUMB_OPCODE_ADDSUBCMPIMM){
		// Add/subtract/compare immediate
		instruction->type = THUMB_TYPE_ADDSUBCMPIMM;
		instruction->Rd = (encoding>>8) & 7;
		instruction->imm = (encoding>>6) & 0xff;
		
		// Determine instruction variant
		switch((encoding >> 11) & 3)
		{
		case 0: // MOV
			strcpy(instruction->mnemonic, "MOV");
			break;
		case 1: // CMP
			strcpy(instruction->mnemonic, "CMP");
			break;
		case 2: // ADD
			strcpy(instruction->mnemonic, "ADD");
			break;
		case 3: // SUB
			strcpy(instruction->mnemonic, "SUB");
			break;
		default:
			return -1;
		
		}
	}else if ((encoding & THUMB_MASK_LDSTWORDBYTE) == THUMB_OPCODE_LDSTWORDBYTE){
		// Load/store word/byte immediate offset
		instruction->type = THUMB_TYPE_LDSTWORDBYTE;
		instruction->Rd = encoding & 7;
		instruction->Rn = (encoding>>3) & 7;
		instruction->imm = (encoding>>6) & 0x1f;
		
		switch((encoding>>11) & 3)
		{
		case 0b00: // STR
			strcpy(instruction->mnemonic, "STR");
			break;
		case 0b01: // LDR
			strcpy(instruction->mnemonic, "LDR");
			break;
		case 0b10: // STRB
			strcpy(instruction->mnemonic, "STRB");
			break;
		case 0b11: // LDRB
			strcpy(instruction->mnemonic, "LDRB");
			break;
		}
	}
	
	return 0;
}

