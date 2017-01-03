/*
 * instdecode.h
 *
 * Created: 1/3/2017 2:55:29 PM
 *  Author: Eric
 */ 


#ifndef INSTDECODE_H_
#define INSTDECODE_H_

#include <stdint.h>



// Bitmasks
#define THUMB_MASK_SHIFTIMM        0xE000 // 3 bit mask
#define THUMB_MASK_ADDSUBREG       0xFC00 // 6 bit mask
#define THUMB_MASK_ADDSUBIMM       0xFC00 // 6 bit mask
#define THUMB_MASK_ADDSUBCMPIMM    0xE000 // 3 bit mask
#define THUMB_MASK_DATAPROCREG     0xFC00 // 6 bit mask
#define THUMB_MASK_SPECIALDATAPROC 0xFC00 // 6 bit mask
#define THUMB_MASK_BRANCH          0xFF00 // 8 bit mask
#define THUMB_MASK_LOADLITERAL     0xF800 // 5 bit mask
#define THUMB_MASK_LDSTREG         0xF000 // 4 bit mask
#define THUMB_MASK_LDSTWORDBYTE    0xE000 // 3 bit mask
#define THUMB_MASK_LDSTHALFWORD    0xF000 // 4 bit mask
#define THUMB_MASK_LDSTSTACK       0xF000 // 4 bit mask
#define THUMB_MASK_ADDSPPC         0xF000 // 4 bit mask
#define THUMB_MASK_MISC            0xF000 // 4 bit mask
#define THUMB_MASK_LDSTM           0xF000 // 4 bit mask
#define THUMB_MASK_BRCOND          0xF000 // 4 bit mask
#define THUMB_MASK_UNDEF           0xFF00 // 8 bit mask
#define THUMB_MASK_SYSCALL         0xFF00 // 8 bit mask
#define THUMB_MASK_BRUNCOND        0xF800 // 5 bit mask
#define THUMB_MASK_32BINSTA        0xF800 // 5 bit mask
#define THUMB_MASK_32BINSTB        0xF000 // 4 bit mask

// Opcodes

#define THUMB_OPCODE_SHIFTIMM        0x0000
#define THUMB_OPCODE_ADDSUBREG       0x1800
#define THUMB_OPCODE_ADDSUBIMM       0x1A00
#define THUMB_OPCODE_ADDSUBCMPIMM    0x2000
#define THUMB_OPCODE_DATAPROCREG     0x4000
#define THUMB_OPCODE_SPECIALDATAPROC 0x4400
#define THUMB_OPCODE_BRANCH          0x4700
#define THUMB_OPCODE_LOADLITERAL     0x4800
#define THUMB_OPCODE_LDSTREG         0x5000
#define THUMB_OPCODE_LDSTWORDBYTE    0x6000
#define THUMB_OPCODE_LDSTHALFWORD    0x8000
#define THUMB_OPCODE_LDSTSTACK       0x9000
#define THUMB_OPCODE_ADDSPPC         0xA000
#define THUMB_OPCODE_MISC            0xB000
#define THUMB_OPCODE_LDSTM           0xC000
#define THUMB_OPCODE_BRCOND          0xD000
#define THUMB_OPCODE_UNDEF           0xDE00
#define THUMB_OPCODE_SYSCALL         0xE800
#define THUMB_OPCODE_BRUNCOND        0xF000
#define THUMB_OPCODE_32BINSTA        0xF800
#define THUMB_OPCODE_32BINSTB        0xF000


struct inst {
	uint16_t imm;
	uint8_t Rd;
	uint8_t Rn;
	uint8_t Rm;
	uint8_t type;
	char mnemonic[4];
};

#endif /* INSTDECODE_H_ */
