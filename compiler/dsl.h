#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "backend.h"

// ASM_TO_BINARY DSL

// Syscall
#define SYSCALL                  if ((res = TranslateSyscall(table.asm_struct)) != 0) return -1;

// Mov num to reg
#define MOV_NUM_TO_REG(reg, num) if ((res = TranslateMovNumToReg(table.asm_struct, num, reg)) != 0) return -1;

// Xor reg, reg
#define XOR_REG_REG(reg)         if ((res = TranslateXor(table.asm_struct, reg, reg)) != 0) return -1;

// Imul reg, reg, 10
#define IMUL_REG_REG_10(reg)     if ((res = TranslateImul(table.asm_struct, reg, reg)) != 0) return -1;

// Push reg
#define PUSH_REG(reg)            if ((res = TranslatePush(table.asm_struct, reg)) != 0) return -1;

// Pop reg
#define POP_REG(reg)             if ((res = TranslatePop(table.asm_struct, reg)) != 0) return -1;

// Add reg, reg
#define ADD_REG_REG(src, dst)    if ((res = TranslateAddOrSubTwoRegs(ADD, table.asm_struct, src, dst)) != 0) return -1;

// Ret
#define RET                      if ((res = TranslateRet(table.asm_struct)) != 0) return -1;

// Xchg reg, reg
#define XCHG_REG_REG(reg1, reg2) if ((res = TranslateXcng(table.asm_struct, reg1, reg2)) != 0) return -1;

// Call
#define CALL_FUNC_NAME(name)     if ((res = TranslateCall(table.asm_struct, name)) != 0) return -1;

// Mov reg to reg
#define MOV_REG_REG(src, dst)    if ((res = TranslateMovRegToReg(table.asm_struct, src, dst)) != 0) return -1;

// Add reg, num
#define ADD_REG_NUM(reg, num)    if ((res = TranslateAddOrSubRegAndNum(ADD, table.asm_struct, num, reg)) != 0) return -1;

// Jumps
#define JUMP(jump)               if ((res = TranslateJmp(table.asm_struct, jump)) != 0) return -1;

// MUL
#define MUL_REG(reg)             if ((res = TranslateMulOrDiv(MUL, table.asm_struct, reg)) != 0) return -1;

// DIV
#define DIV_REG(reg)             if ((res = TranslateMulOrDiv(DIV, table.asm_struct, reg)) != 0) return -1;

// Sub reg, reg
#define SUB_REG_REG(src, dst)    if ((res = TranslateAddOrSubTwoRegs(SUB, table.asm_struct, src, dst)) != 0) return -1;

// Sub reg, num
#define SUB_REG_NUM(reg, num)    if ((res = TranslateAddOrSubRegAndNum(SUB, table.asm_struct, num, reg)) != 0) return -1;

// Label
#define LABEL_AFTER_JUMP(label)  if ((res = TranslateLabelAfterJump(table.asm_struct, label)) != 0) return -1;

// Cmp reg, reg
#define CMP_REG_REG(src, dst)    if ((res = TranslateCmp(table.asm_struct, src, dst)) != 0) return -1;

// Lea
#define LEA_RAX_VIDEOMEMORY            if ((res = TranslateLeaFromMemLabel(table.asm_struct, RAX, "VIDEOMEMORY")) != 0) return -1; // FIXME check
#define LEA_RDI_RBP_WITH_DISP          if ((res = TranslateLeaFromMemRegWithDisp(table.asm_struct, LOCAL_VAR_INDEX * 8, RBP, RDI)) != 0) return -1;
#define LEA_RDI_GLOBAL_VAR_NAME        if ((res = TranslateLeaFromMemLabel(table.asm_struct, RDI, GLOBAL_VAR_NAME)) != 0) return -1;

// Convert
#define CVTSI2SD_XMM0_RBX              if ((res = TranslateCvtTypeSpecified(CVTSI2SD, table.asm_struct, RBX, XMM0)) != 0) return -1;
#define CVTSD2SI_RBX_XMM0              if ((res = TranslateCvtTypeSpecified(CVTSD2SI, table.asm_struct, XMM0, RBX)) != 0) return -1;

// Sqrt
#define SQRTSD_XMM0_XMM0               if ((res = TranslateSqr(table.asm_struct)) != 0) return -1;

// Movs
#define MOV_BYTE                       if ((res = TranslateMovByte(table.asm_struct)) != 0) return -1;
#define MOV_RAX_FROM_MEM_RBP_WITH_DISP if ((res = TranslateMovMemRegWithDispToReg(table.asm_struct, local_var_index * 8, RBP, RAX)) != 0) return -1;
#define MOV_RAX_FROM_MEM_LABEL         if ((res = TranslateMovMemLabelToReg(table.asm_struct, RAX, curr_node->value.name.name)) != 0) return -1;
#define MOV_RDI_FROM_MEM_RBP_WITH_DISP if ((res = TranslateMovMemRegWithDispToReg(table.asm_struct, LOCAL_VAR_INDEX * 8, RBP, RDI)) != 0) return -1;
#define MOV_RDI_LABEL_GLOBAL_VAR_NAME  if ((res = TranslateMovMemLabelToReg(table.asm_struct, RDI, GLOBAL_VAR_NAME)) != 0) return -1;
#define MOV_RDI_TO_MEM_RBP             if ((res = TranslateMovRegToMemRegWithDisp(table.asm_struct, 0, RDI, RBP)) != 0) return -1;
#define MOV_RDI_MEM_LABEL              if ((res = TranslateMovMemLabelToReg(table.asm_struct, RDI, node->left->value.name.name)) != 0) return -1;
#define MOV_RAX_TO_MEM_RBP_WITH_DISP   if ((res = TranslateMovRegToMemRegWithDisp(table.asm_struct, LOCAL_VAR_INDEX * 8, RAX, RBP)) != 0) return -1;
#define MOV_RAX_TO_MEM_GLOBAL_VAR_NAME if ((res = TranslateMovRegToMemLabel(table.asm_struct, RAX, GLOBAL_VAR_NAME)) != 0) return -1;
#define MOV_RBX_TO_MEM_RBP_WITH_DISP   if ((res = TranslateMovRegToMemRegWithDisp(table.asm_struct, LOCAL_VAR_INDEX * 8, RBX, RBP)) != 0) return -1;
#define MOV_RBX_TO_MEM_LABEL           if ((res = TranslateMovRegToMemLabel(table.asm_struct, RBX, GLOBAL_VAR_NAME)) != 0) return -1;
#define MOV_MEM_RBP_WITH_DISP_TO_RBX   if ((res = TranslateMovMemRegWithDispToReg(table.asm_struct, LOCAL_VAR_INDEX * 8, RBP, RBX)) != 0){ printf("\n\n\nNOT TRANSLATED[]!!!!!!!!!!!\n\n\n");return -1;}
#define MOV_MEM_LABEL_TO_RBX           if ((res = TranslateMovMemLabelToReg(table.asm_struct, RBX, GLOBAL_VAR_NAME)) != 0) return -1;
#define MOV_RAX_TO_MEM_LABEL           if ((res = TranslateMovRegToMemLabel(table.asm_struct, RAX, GLOBAL_VAR_NAME)) != 0) return -1;
