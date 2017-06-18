/*
 * instdecode.h
 *
 * Created: 1/3/2017 2:55:29 PM
 *  Author: Neil Klingensmith
 */ 


#ifndef INSTDECODE_H_
#define INSTDECODE_H_

#include <stdint.h>
#include "hermes.h"


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
#define THUMB_MASK32_DATAPROCIMM   0xF0008000
#define THUMB_MASK32_DATAPROCNOIMM 0xEE000000
#define THUMB_MASK32_LDSTSINGLE    0xFE000000
#define THUMB_MASK32_LDSTDOUBLE    0xFE400000
#define THUMB_MASK32_LDSTM         0xFE400000
#define THUMB_MASK32_BRANCH        0xF8008000
#define THUMB_MASK32_MRS           0xFFE0D000
#define THUMB_MASK32_MSR           0xFFE0D000
#define THUMB_MASK_MISC_CPS        0xFFE0


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
#define THUMB_OPCODE_BRUNCOND        0xE000
#define THUMB_OPCODE_32BINSTA        0xE800
#define THUMB_OPCODE_32BINSTB        0xF000
#define THUMB_OPCODE32_DATAPROCIMM   0xF0000000 // A
#define THUMB_OPCODE32_DATAPROCNOIMM 0xEA000000 // A?
#define THUMB_OPCODE32_LDSTSINGLE    0xF8000000 // B
#define THUMB_OPCODE32_LDSTDOUBLE    0xE8400000 // A
#define THUMB_OPCODE32_LDSTM         0xE8000000 // A
#define THUMB_OPCODE32_BRANCH        0xF0008000 // B
#define THUMB_OPCODE32_MRS           0xF3E08000 // B
#define THUMB_OPCODE32_MSR           0xF3808000 // B
#define THUMB_OPCODE_MISC_CPS        0xB660

// Instruction type definitions
#define THUMB_TYPE_SHIFTIMM        1
#define THUMB_TYPE_ADDSUBREG       2
#define THUMB_TYPE_ADDSUBIMM       3
#define THUMB_TYPE_ADDSUBCMPIMM    4
#define THUMB_TYPE_DATAPROCREG     5
#define THUMB_TYPE_SPECIALDATAPROC 6
#define THUMB_TYPE_BRANCH          7
#define THUMB_TYPE_LOADLITERAL     8
#define THUMB_TYPE_LDSTREG         9
#define THUMB_TYPE_LDSTWORDBYTE    10
#define THUMB_TYPE_LDSTHALFWORD    11
#define THUMB_TYPE_LDSTSTACK       12
#define THUMB_TYPE_ADDSPPC         13
#define THUMB_TYPE_MISC            14
#define THUMB_TYPE_LDSTM           15
#define THUMB_TYPE_BRCOND          16
#define THUMB_TYPE_UNDEF           17
#define THUMB_TYPE_SYSCALL         18
#define THUMB_TYPE_BRUNCOND        19
#define THUMB_TYPE_LDSTDOUBLE      20
#define THUMB_TYPE_LDSTM32         21
#define THUMB_TYPE_LDSTSINGLE      22
#define THUMB_TYPE_MRS             23
#define THUMB_TYPE_MSR             24

#define THUMB_TYPE_MISC_CPS        25

// Register definitions for the MRS instruction
#define MRS_REGISTER_MSP           8
#define MRS_REGISTER_PSP           9
#define MRS_REGISTER_BASEPRI       17

struct inst {
	uint16_t imm;
	uint8_t Rd;
	uint8_t Rn;
	uint8_t Rm;
	uint8_t Rt;
	uint8_t nbytes;
	uint8_t type;
	char mnemonic[6];
};


int instDecode(struct inst *instruction, uint16_t *location);
void *effectiveAddress(struct inst *instruction, struct vm *guest);


#endif /* INSTDECODE_H_ */
