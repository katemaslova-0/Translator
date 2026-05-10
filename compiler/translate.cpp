#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "translate.h"

// elf.h
unsigned char elf_header[] =          {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x02, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00,
                                       0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char program_header_text[] = {0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // 0x05 -> 0x07
                                       0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char program_header_data[] = {0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // 0x06 -> 0x07
                                       0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xC9, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xC9, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


void MakeStdFuncArrays (Asm_t * asm_struct)
{
    assert(asm_struct);

    system("objcopy -O binary --only-section=.text asm_standart_func.o output.o");

    FILE * fp = fopen("output.o", "rb");

    struct stat st;
    stat("output.o", &st);
    int filesize = (int)st.st_size;

    unsigned char * buffer = (unsigned char *) calloc ((size_t) filesize, sizeof(unsigned char));
    assert(buffer);

    fread(buffer, (size_t)filesize, sizeof(unsigned char), fp);

    for (int count = 0; count < SIZE_OF_DRAW; count++)
        asm_struct->draw[count] = buffer[count];

    for (int count = 0; count < SIZE_OF_MY_IN; count++)
        asm_struct->my_in[count] = buffer[count + SIZE_OF_DRAW];

    for (int count = 0; count < SIZE_OF_MY_PRINTF; count++)
        asm_struct->my_printf[count] = buffer[count + SIZE_OF_DRAW + SIZE_OF_MY_IN];

    fclose(fp);
}


void AddDrawFuncBytes (Asm_t * asm_struct)
{
    assert(asm_struct);

    int byte_counter = asm_struct->byte_counter;
    int size = SIZE_OF_DRAW;

    for (int i = 0; i < asm_struct->name_counter; i++)
    {
        if (strcmp("Draw", asm_struct->names[i].name) == 0)
            asm_struct->names[i].offset = byte_counter;
    }

    for (int count = 0; count < size; count++)
        asm_struct->bytes[byte_counter + count] = asm_struct->draw[count];

    for (int i = 0; i < asm_struct->name_counter; i++)
    {
        if (strcmp("VIDEOMEMORY", asm_struct->names[i].name) == 0)
        {
            asm_struct->names[i].byte_pointers[asm_struct->names[i].size_of_byte_pointers_array] = byte_counter + 20; // FIXME
            asm_struct->names[i].size_of_byte_pointers_array++;
            break;
        }
    }

    asm_struct->byte_counter += size;
}


void AddMyInFuncBytes (Asm_t * asm_struct)
{
    assert(asm_struct);

    int byte_counter = asm_struct->byte_counter;
    int size = SIZE_OF_MY_IN;

    for (int i = 0; i < asm_struct->name_counter; i++)
    {
        if (strcmp("MyIn", asm_struct->names[i].name) == 0)
        {
            asm_struct->names[i].offset = byte_counter;
            break;
        }
    }

    for (int count = 0; count < size; count++)
        asm_struct->bytes[byte_counter + count] = asm_struct->my_in[count];

    asm_struct->byte_counter += size;
}


void AddMyPrintfFuncBytes (Asm_t * asm_struct)
{
    assert(asm_struct);

    int byte_counter = asm_struct->byte_counter;
    int size = SIZE_OF_MY_PRINTF;

    int i = 0;
    for (; i < asm_struct->name_counter; i++)
    {
        if (strcmp("MyPrintf", asm_struct->names[i].name) == 0)
        {
            asm_struct->names[i].offset = byte_counter;
            break;
        }
    }

    if (i == asm_struct->name_counter)
        printf("ERROR! PRINTF NOR FOUND\n\n\n");

    for (int count = 0; count < size; count++)
        asm_struct->bytes[byte_counter + count] = asm_struct->my_printf[count];

    asm_struct->byte_counter += size;
}


// TRANSLATE FUNCS: WILL BE REWRITED WITHOUT THE STRING PARSING

// MOVS:

int TranslateMovRegToReg (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x89; // opcode: mov from reg to reg/mem

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in regs

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateMovNumToReg (Asm_t * asm_struct, int value, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xC7; // opcode: mov num to reg/mem

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in regs

    Operand_t src = NUM_MOV;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 1] = mod_rm;

    // записываем 4-байтное значение в little-endian
    asm_struct->bytes[count + 2] = (unsigned char) value & 0xFF;
    asm_struct->bytes[count + 3] = (unsigned char) (value >> 8) & 0xFF;
    asm_struct->bytes[count + 4] = (unsigned char) (value >> 16) & 0xFF;
    asm_struct->bytes[count + 5] = (unsigned char) (value >> 24) & 0xFF;

    asm_struct->byte_counter += 6;

    return 0;
}


int TranslateMovRegToMemRegWithDisp (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst) // max 1 byte disp!
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x89; // opcode: mov from reg to reg/mem

    unsigned char mod = 0;
    mod |= (1 << 6); // mod == '01' <-> 8 bit disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->bytes[count + 3] = (unsigned char) disp & 0xFF; // disp = 8 bit

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateMovRegToMemLabel (Asm_t * asm_struct, Operand_t src, const char * name_ptr)
{
    assert(asm_struct);
    assert(name_ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x89; // opcode: mov from reg to reg/mem

    unsigned char mod = 0; // mod == '00' <-> no disp

    int res = 0;
    if ((res = AddNamePointer(asm_struct, name_ptr, asm_struct->byte_counter + 3)) != 0)
        return -1;

    Operand_t dst = LABEL;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now
    return 0;
}


int TranslateMovRegToMemRegNoDisp (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x89; // opcode: mov from reg to reg/mem

    unsigned char mod = 0; // mod == '00' <-> no disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateMovMemRegWithDispToReg (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8B; // opcode: mov from reg/mem to reg

    unsigned char mod = 0;
    mod |= (1 << 6); // mod == '01' <-> 8 bit disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->bytes[count + 3] = (unsigned char) disp & 0xFF; // disp = 8 bit

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateMovMemRegNoDispToReg (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8B; // opcode: mov from reg/mem to reg

    unsigned char mod = 0; // mod == '00' <-> no disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateMovMemLabelToReg (Asm_t * asm_struct, Operand_t dst, const char * name_ptr)
{
    assert(asm_struct);
    assert(name_ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8B; // opcode: mov from reg/mem to reg

    unsigned char mod = 0; // mod == '00' <-> no disp

    Operand_t src = LABEL;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, name_ptr, asm_struct->byte_counter + 3)) != 0)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now

    return 0;
}


int TranslateMovByte (Asm_t * asm_struct) // only for specific standart func! check
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0xC6;
    asm_struct->bytes[count + 1] = 0x04;
    asm_struct->bytes[count + 2] = 0x08;
    asm_struct->bytes[count + 3] = 0x6F;

    asm_struct->byte_counter += 4;

    return 0;
}


// LEA

int TranslateLeaFromMemRegNoDisp (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8D; // opcode: lea

    unsigned char mod = 0; // mod == '00' <-> no disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateLeaFromMemRegWithDisp (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst) // only for 8 bit disp
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8D; // opcode: lea

    unsigned char mod = 0;
    mod |= (1 << 6); // mod == '01' <-> 8 bit disp

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->bytes[count + 3] = (unsigned char) disp & 0xFF; // disp = 8 bit

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateLeaFromMemLabel (Asm_t * asm_struct, Operand_t dst, const char * name_ptr)
{
    assert(asm_struct);
    assert(name_ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x8D; // opcode: lea

    unsigned char mod = 0; // mod == '00' <-> no disp

    Operand_t src = LABEL;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, name_ptr, asm_struct->byte_counter + 3)) != 0)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now

    return 0;
}

// CALL

int TranslateCall (Asm_t * asm_struct, const char * name_ptr)
{
    assert(asm_struct);
    assert(name_ptr);

    printf("\n\nTRANSLATE CALL CALLED: %s\n\n\n", name_ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xE8; // opcode: call disp32

    int res = 0;
    if ((res = AddNamePointer(asm_struct, name_ptr, asm_struct->byte_counter + 1)) != 0)
        return -1;

    asm_struct->byte_counter += 5; // 4 bytes for offset are null for now

    return 0;
}


// PUSH

int TranslatePush (Asm_t * asm_struct, Operand_t reg)
{
    assert(asm_struct);

    unsigned char byte = 0x50 + CalculateByteForPushPopInstr(reg); // opcode: push reg

    asm_struct->bytes[asm_struct->byte_counter] = byte;
    asm_struct->byte_counter++;

    return 0;
}


unsigned char CalculateByteForPushPopInstr (Operand_t reg)
{
    unsigned char byte = 0;

    switch(reg)
    {
        case RAX:            break;
        case RCX: byte++;    break;
        case RDX: byte += 2; break;
        case RBX: byte += 3; break;
        case RSP: byte += 4; break;
        case RBP: byte += 5; break;
        case RSI: byte += 6; break;
        case RDI: byte += 7; break;
        case XMM0:
        case NUM_MOV:
        case NUM_ADD:
        case NUM_SUB:
        case LABEL:
        case DEFAULT:
        default: printf("Error in CalculateByteForPushPopInstr\n");
    }

    return byte;
}

// POP

int TranslatePop (Asm_t * asm_struct, Operand_t reg)
{
    assert(asm_struct);

    unsigned char byte = 0x58 + CalculateByteForPushPopInstr(reg); // opcode: pop reg

    asm_struct->bytes[asm_struct->byte_counter] = byte;
    asm_struct->byte_counter++;

    return 0;
}

// XOR

int TranslateXor (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x31; // opcode: xor reg/mem, reg

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}

// SYSCALL

int TranslateSyscall (Asm_t * asm_struct)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x0F;
    asm_struct->bytes[count + 1] = 0x05; // 0x0F 0x05 --> syscall

    asm_struct->byte_counter += 2;

    return 0;
}

// ADD OR SUB

int TranslateAddOrSubTwoRegs (MathOp_t type, Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48; // for 64-bit operands

    if (type == ADD)
        asm_struct->bytes[count + 1] = 0x01; // opcode: add reg/mem, reg
    else
        asm_struct->bytes[count + 1] = 0x29; // opcode: sub reg/mem, reg

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in regs

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateAddOrSubRegAndNum (MathOp_t type, Asm_t * asm_struct, int value, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;     // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x81; // == operation is coded inside mod_rm (as reg)

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in regs; <op> reg/mem, num

    Operand_t src = DEFAULT;

    if (type == ADD)
        src = NUM_ADD;
    else
        src = NUM_SUB;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    // записываем 4-байтное значение в little-endian
    asm_struct->bytes[count + 3] = (unsigned char) value & 0xFF;
    asm_struct->bytes[count + 4] = (unsigned char) (value >> 8) & 0xFF;
    asm_struct->bytes[count + 5] = (unsigned char) (value >> 16) & 0xFF;
    asm_struct->bytes[count + 6] = (unsigned char) (value >> 24) & 0xFF;

    asm_struct->byte_counter += 7;

    return 0;
}

// MUL OR DIV

int TranslateMulOrDiv (MathOp_t type, Asm_t * asm_struct, Operand_t reg)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0xF7; //== operation is coded inside mod_rm (as reg)

    unsigned char mod_rm = 0;

    if (type == MUL)
        mod_rm = 0xE0; // the operation
    else if (type == DIV)
        mod_rm = 0xF0;
    else
        printf("Error in TranslateMulOrDiv(invalid math op)\n");

    switch(reg)
    {
        case RAX:              break;
        case RCX: mod_rm++;    break;
        case RDX: mod_rm += 2; break;
        case RBX: mod_rm += 3; break;
        case RSP: mod_rm += 4; break;
        case RBP: mod_rm += 5; break;
        case RSI: mod_rm += 6; break;
        case RDI: mod_rm += 7; break;
        case XMM0:
        case NUM_MOV:
        case NUM_ADD:
        case NUM_SUB:
        case LABEL:
        case DEFAULT:
        default: printf("Error in TranslateMulOrDiv\n");
    }

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


// XCHG

int TranslateXcng (Asm_t * asm_struct, Operand_t reg1, Operand_t reg2)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x87; // opcode: xchg reg/mem, reg

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in registers

    unsigned char mod_rm = MakeModeRm(mod, reg1, reg2, SrcFirst); // the last arg doesn't matter
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}

// CMP

int TranslateCmp (Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x39; // opcode: cmp reg/mem, reg

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in registers

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}

// JUMPS

// Разделяем трансляцию джампа и метки после него

int TranslateJmp (Asm_t * asm_struct, Jmp_t jump)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    if (jump == JMP)
    {
        asm_struct->bytes[count] = 0xE9; // opcode: jmp disp32
        asm_struct->byte_counter++;
    }
    else
    {
        asm_struct->bytes[count] = 0x0F; // escape prefix: opcode is next

        switch(jump) // jump opcode
        {
            case JL:  asm_struct->bytes[count + 1] = 0x8C; break;
            case JLE: asm_struct->bytes[count + 1] = 0x8E; break;
            case JG:  asm_struct->bytes[count + 1] = 0x8F; break;
            case JGE: asm_struct->bytes[count + 1] = 0x8D; break;
            case JZ:
            case JE:  asm_struct->bytes[count + 1] = 0x84; break;
            case JNZ:
            case JNE: asm_struct->bytes[count + 1] = 0x85; break;
            case JS:  asm_struct->bytes[count + 1] = 0x88; break;
            case JNS: asm_struct->bytes[count + 1] = 0x89; break;
            case JMP:
            case DEFAULT_JMP:
            default: {printf("Error in TranslateJmp\n"); return -1;}
        }

        asm_struct->byte_counter += 2;
    }

    return 0;
}


int TranslateLabelAfterJump (Asm_t * asm_struct, int label_number)
{
    assert(asm_struct);

    int res = 0;

    if ((res = AddLabelPointer(asm_struct, label_number, asm_struct->byte_counter)) != 0)
        return -1;

    asm_struct->byte_counter += 4; // 4 bytes for offset are null for now

    return 0;
}

// CONVERT

int TranslateCvtTypeSpecified (Cvt_t type, Asm_t * asm_struct, Operand_t src, Operand_t dst)
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;
    asm_struct->bytes[count] = 0xF2; // rep prefix
    asm_struct->bytes[count + 1] = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 2] = 0x0F; // escape prefix

    if (type == CVTSI2SD)
        asm_struct->bytes[count + 3] = 0x2A; // opcode
    else if (type == CVTSD2SI)
        asm_struct->bytes[count + 3] = 0x2D; // opcode
    else
    {
        printf("Error in TranslateCvtTypeSpecified\n");
        return -1;
    }

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6); // mod = '11' <-> both operands are in registers

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 4] = mod_rm;

    asm_struct->byte_counter += 5;

    return 0;
}

// SQRT

int TranslateSqr (Asm_t * asm_struct) // for now only for sqrtsd xmm0, xmm0!
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xF2; // rep prefix
    asm_struct->bytes[count + 1] = 0x0F; // escape prefix
    asm_struct->bytes[count + 2] = 0x51;
    asm_struct->bytes[count + 3] = 0xC0;

    asm_struct->byte_counter += 4;

    return 0;
}

// RET

int TranslateRet (Asm_t * asm_struct)
{
    assert(asm_struct);

    asm_struct->bytes[asm_struct->byte_counter] = 0xC3; // opcode
    asm_struct->byte_counter++;

    return 0;
}

// IMUL

int TranslateImul (Asm_t * asm_struct, Operand_t src, Operand_t dst) // только для imul reg1, reg2, 10!!
{
    assert(asm_struct);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48; // for 64-bit operands
    asm_struct->bytes[count + 1] = 0x6B; // opcode of imul

    unsigned char mod = 0;
    mod |= (1 << 6);
    mod |= (1 << 7); // mod = '11' <-> both operands are in registers

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);

    asm_struct->bytes[count + 2] = mod_rm;
    asm_struct->bytes[count + 3] = 0x0A; // = 10d

    asm_struct->byte_counter += 4;

    return 0;
}


unsigned char MakeModeRm (unsigned char mod, Operand_t src, Operand_t dst, Order_t order)
{
    unsigned char mod_rm = mod;

    if (order == DstFirst)
    {
        Operand_t val = src;
        src = dst;
        dst = val;
    }

    switch(src)
    {
        case RAX: break;
        case RBX: {mod_rm |= (1 << 3); mod_rm |= (1 << 4);} break;
        case RCX: {mod_rm |= (1 << 3);} break;
        case RDX: {mod_rm |= (1 << 4);} break;
        case RDI: {mod_rm |= (1 << 3); mod_rm |= (1 << 4); mod_rm |= (1 << 5);} break;
        case RSI: {mod_rm |= (1 << 4); mod_rm |= (1 << 5);} break;
        case RBP: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case RSP: {mod_rm |= (1 << 5);} break;
        case NUM_MOV: break;
        case NUM_ADD: break;
        case NUM_SUB: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case XMM0: break;
        case LABEL: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case DEFAULT:
        default: printf("Error in MakeModRm(src)\n");
    }

    switch (dst)
    {
        case RAX: break;
        case RBX: {mod_rm |= (1 << 0); mod_rm |= (1 << 1);} break;
        case RCX: {mod_rm |= (1 << 0);} break;
        case RDX: {mod_rm |= (1 << 1);} break;
        case RDI: {mod_rm |= (1 << 0); mod_rm |= (1 << 1); mod_rm |= (1 << 2);} break;
        case RSI: {mod_rm |= (1 << 1); mod_rm |= (1 << 2);} break;
        case RBP: {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case RSP: {mod_rm |= (1 << 2);} break;
        case NUM_MOV: break;
        case NUM_ADD: break;
        case NUM_SUB: {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case XMM0: break;
        case LABEL:   {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case DEFAULT:
        default: printf("Error in MakeModRm(dst)\n");
    }

    return mod_rm;
}

// ADD POINTERS FUNCS

int AddNamePointer (Asm_t * asm_struct, const char * ptr, int var_offset)
{
    assert(asm_struct);
    assert(ptr);

    int name_counter = asm_struct->name_counter;

    char name[20] = {}; // FIXME

    for(int count = 0; *ptr != ']' && *ptr != '\0'; count++, ptr++)
        name[count] = *ptr;

    printf("AddNamePointer: Name came: %s\n", name);

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(name, asm_struct->names[count].name) == 0)
        {
            asm_struct->names[count].byte_pointers[asm_struct->names[count].size_of_byte_pointers_array] = var_offset;
            asm_struct->names[count].size_of_byte_pointers_array++;
            return 0;
        }
    }

    printf("Error: var has not been found\n");
    return -1;
}


int AddLabelPointer (Asm_t * asm_struct, int label_number, int label_offset)
{
    assert(asm_struct);

    int label_counter = asm_struct->label_counter;

    for (int count = 0; count < label_counter; count++)
    {
        if (label_number == asm_struct->labels[count].number)
        {
            asm_struct->labels[count].byte_pointers[asm_struct->labels[count].size_of_byte_pointers_array] = label_offset;
            asm_struct->labels[count].size_of_byte_pointers_array++;
            return 0;
        }
    }

    printf("Error: label not found\n");
    return -1;
}


// ADD OFFSETS FUNCS

int AddFunctionOffset (Asm_t * asm_struct, const char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int name_counter = asm_struct->name_counter;

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(ptr, asm_struct->names[count].name) == 0)
        {
            asm_struct->names[count].offset = asm_struct->byte_counter;
            return 0;
        }
    }

    printf("Error: func name's not found\n");
    return -1;
}


int AddLabel (Asm_t * asm_struct, int label_number)
{
    assert(asm_struct);

    asm_struct->labels[asm_struct->label_counter].number = label_number;
    asm_struct->label_counter++;

    return 0;
}


int AddLabelOffset (Asm_t * asm_struct, int label_number)
{
    assert(asm_struct);

    if (label_number < 0 || label_number > asm_struct->label_counter)
    {
        printf("Error in AddLabelOffset\n");
        return -1;
    }

    asm_struct->labels[label_number].offset = asm_struct->byte_counter - 4;

    return 0;
}


// LINKING

int Link (Asm_t * asm_struct)
{
    assert(asm_struct);

    int label_counter = asm_struct->label_counter;
    int name_counter  = asm_struct->name_counter;
    printf("NAME COUNTER = %d\n", name_counter);

    for (int count = 0; count < label_counter; count++)
    {
        int byte_ptr_array_size = asm_struct->labels[count].size_of_byte_pointers_array;
        Label_t curr_label = asm_struct->labels[count];

        printf("\nLabel: number: %d offset: %d ", curr_label.number, curr_label.offset);
        for (int one_label_cnt = 0; one_label_cnt < byte_ptr_array_size; one_label_cnt++)
        {
            int offset = curr_label.offset - curr_label.byte_pointers[one_label_cnt];
            printf(" RESULT OFFSET: %d ", offset);
            WriteOffsetToByteArray(asm_struct, offset, curr_label.byte_pointers[one_label_cnt]);

            printf("byteptr: %d ", curr_label.byte_pointers[one_label_cnt]);
        }
    }

    for (int count = 0; count < name_counter; count++)
    {
        int byte_ptr_array_size = asm_struct->names[count].size_of_byte_pointers_array;
        Name_t curr_name = asm_struct->names[count];

        printf("\nName: count: %d, name: %s offset: %d ", count, curr_name.name, curr_name.offset);
        for (int one_name_cnt = 0; one_name_cnt < byte_ptr_array_size; one_name_cnt++)
        {
            int offset = 0;

            if (curr_name.type == VAR)
                offset = curr_name.offset - 4 - curr_name.byte_pointers[one_name_cnt];
            else
                offset = curr_name.offset - 4 - curr_name.byte_pointers[one_name_cnt];

            printf(" RESULT OFFSET: %d ", offset);
            WriteOffsetToByteArray(asm_struct, offset, curr_name.byte_pointers[one_name_cnt]);

            printf("byteptr: %d ", curr_name.byte_pointers[one_name_cnt]);
        }
    }

    return 0;
}


void WriteOffsetToByteArray (Asm_t * asm_struct, int offset, int byte_pointer)
{
    assert(asm_struct);

    asm_struct->bytes[byte_pointer] =     (unsigned char)  offset & 0xFF;
    asm_struct->bytes[byte_pointer + 1] = (unsigned char) (offset >> 8)  & 0xFF;
    asm_struct->bytes[byte_pointer + 2] = (unsigned char) (offset >> 16) & 0xFF;
    asm_struct->bytes[byte_pointer + 3] = (unsigned char) (offset >> 24) & 0xFF;
}


int AddVarsOffsets (Asm_t * asm_struct)
{
    assert(asm_struct);

    int num_of_names = asm_struct->name_counter;

    for (int count = 0; count < num_of_names; count++)
    {
        if (asm_struct->names[count].type == VAR)
        {
            asm_struct->names[count].offset = asm_struct->byte_counter;

            int byte_counter = asm_struct->byte_counter;
            int size = asm_struct->names[count].size;

            if (strcmp(asm_struct->names[count].name, "VIDEOMEMORY") == 0)
            {
                for (int i = 0; i < size; i++)
                    asm_struct->bytes[byte_counter + i] = 0x20;
            }
            else
            {
                for (int i = 0; i < size; i++)
                    asm_struct->bytes[byte_counter + i] = 0x00;
            }

            asm_struct->byte_counter += size;
        }
    }

    return 0;
}

// BUILDING ELF

void SetCodeSize (int code_size)
{
    program_header_text[32] = (unsigned char)  code_size & 0xFF;
    program_header_text[33] = (unsigned char) (code_size >> 8) & 0xFF;
    program_header_text[34] = (unsigned char) (code_size >> 16) & 0xFF;
    program_header_text[35] = (unsigned char) (code_size >> 24) & 0xFF;

    program_header_text[40] = (unsigned char)  code_size & 0xFF;
    program_header_text[41] = (unsigned char) (code_size >> 8)  & 0xFF;
    program_header_text[42] = (unsigned char) (code_size >> 16) & 0xFF;
    program_header_text[43] = (unsigned char) (code_size >> 24) & 0xFF;
}


void SetDataSizeAndOffset (int data_size, int code_size)
{
    // size
    program_header_data[32] = (unsigned char) data_size & 0xFF;
    program_header_data[33] = (unsigned char) (data_size >> 8) & 0xFF;
    program_header_data[34] = (unsigned char) (data_size >> 16) & 0xFF;
    program_header_data[35] = (unsigned char) (data_size >> 24) & 0xFF;

    program_header_data[40] = (unsigned char) data_size & 0xFF;
    program_header_data[41] = (unsigned char) (data_size >> 8) & 0xFF;
    program_header_data[42] = (unsigned char) (data_size >> 16) & 0xFF;
    program_header_data[43] = (unsigned char) (data_size >> 24) & 0xFF;

    //offset
    int offset = 0x400000 + (int)sizeof(elf_header) + (int)sizeof(program_header_text) + (int)sizeof(program_header_data) + code_size;

    program_header_data[16] = (unsigned char) offset & 0xFF;
    program_header_data[17] = (unsigned char) (offset >> 8)  & 0xFF;
    program_header_data[18] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[19] = (unsigned char) (offset >> 24) & 0xFF;

    program_header_data[24] = (unsigned char) offset & 0xFF;
    program_header_data[25] = (unsigned char) (offset >> 8)  & 0xFF;
    program_header_data[26] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[27] = (unsigned char) (offset >> 24) & 0xFF;

    offset -= 0x400000;

    program_header_data[8] = (unsigned char) offset & 0xFF;
    program_header_data[9] = (unsigned char)  (offset >> 8)  & 0xFF;
    program_header_data[10] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[11] = (unsigned char) (offset >> 24) & 0xFF;
}


void FwriteAll (FILE * fp_out, Asm_t * asm_struct)
{
    assert(fp_out);
    assert(asm_struct);

    fwrite(&elf_header,          1, sizeof(elf_header),          fp_out);
    fwrite(&program_header_text, 1, sizeof(program_header_text), fp_out);
    fwrite(&program_header_data, 1, sizeof(program_header_data), fp_out);
    fwrite(asm_struct->bytes,    1, (size_t) asm_struct->byte_counter,    fp_out);
}
