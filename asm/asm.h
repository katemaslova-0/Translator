#ifndef ASM_H
#define ASM_H

const int FUNC_NAME_LENGTH = 20;

enum FuncOrVar_t
{
    FUNC,
    VAR
};

struct Name_t
{
    char * name;
    FuncOrVar_t type;
    int offset;
    int * byte_pointers;
    int size_of_byte_pointers_array = 0;
};

struct Label_t
{
    char number;
    int offset;
    int * byte_pointers;
    int size_of_byte_pointers_array = 0;
};

struct Asm_t
{
    int num_of_lines;
    int size_of_buffer;
    char * buffer;
    Name_t * names;
    Label_t * labels;
    unsigned char * bytes;
    int byte_counter = 0;
    int name_counter = 0;
    int label_counter = 0;
    char ** lines;
    int start_of_data_segment = 0;
};

enum MathOp_t
{
    ADD,
    SUB,
    MUL,
    DIV
};

enum Jmp_t
{
    JMP,
    JL,
    JLE,
    JG,
    JGE,
    JE,
    JNE,
    JS,
    JNS,
    JZ,
    JNZ,
    DEFAULT_JMP
};

struct HashElem_t
{
    int hash;
    const char * string;
    int (* func) (Asm_t *, char *);
};

enum Cvt_t
{
    CVTSI2SD,
    CVTSD2SI
};

enum Op_t
{
    RAX,
    RBX,
    RCX,
    RDX,
    RDI,
    RSI,
    RBP,
    RSP,
    XMM0,
    NUM_MOV,
    NUM_ADD,
    NUM_SUB,
    LABEL,
    DEFAULT
};

enum Order_t
{
    SrcFirst,
    DstFirst
};


Asm_t InitAsmStruct         (void);
void DestroyAll             (Asm_t * asm_struct);
int FillHashtable           (void);
int CopyAsmFileToBuffer     (Asm_t * asm_struct, const char * input_file);
unsigned int Crc32          (const char * word);
int FillPointerBuff         (Asm_t * asm_struct);
int CountLines              (char * buffer);
int GetFileSize             (const char * filename);
void ReplaceSymbolsInBuffer (char * buffer, char sym_to_find, char sym_to_put_instead, int size_of_buffer);


int FillNametable (Asm_t * asm_struct);
void FillVars     (Asm_t * asm_struct, int count, char ** ptr);
void FillFuncCall (Asm_t * asm_struct, char * ptr);
void FillLabel    (Asm_t * asm_struct, char * ptr);

int FirstCompilation  (Asm_t * asm_struct);
int SecondCompilation (Asm_t * asm_struct);

void WriteOffsetToByteArray (Asm_t * asm_struct, int offset, int byte_pointer);
int AddVarsOffsets         (Asm_t * asm_struct);
int AddFunctionOffset      (Asm_t * asm_struct, char * ptr);
int AddLabelOffset         (Asm_t * asm_struct, char * ptr);

int TranslateRet (Asm_t * asm_struct, char * ptr);
int TranslateSqr (Asm_t * asm_struct, char * ptr);
int TranslateCvt (Asm_t * asm_struct, char * ptr);
int TranslateCvtTypeSpecified (Cvt_t type, Asm_t * asm_struct, char * ptr);
int TranslateJmp (Asm_t * asm_struct, char * ptr);
int TranslateMath (Asm_t * asm_struct, char * ptr);
int TranslateCmp (Asm_t * asm_struct, char * ptr);
int TranslateXcng (Asm_t * asm_struct, char * ptr);
void TranslateMulOrDiv (MathOp_t type, Asm_t * asm_struct, char * ptr);
void TranslateAddOrSub (MathOp_t type, Asm_t * asm_struct, char * ptr);
void TranslateAddOrSubRegAndNum (MathOp_t type, Asm_t * asm_struct, char * ptr);
void TranslateAddOrSubTwoRegs (MathOp_t type, Asm_t * asm_struct, char * ptr);
int TranslateSyscall (Asm_t * asm_struct, char * ptr);
int TranslateXor (Asm_t * asm_struct, char * ptr);
int TranslatePop (Asm_t * asm_struct, char * ptr);
int TranslatePush (Asm_t * asm_struct, char * ptr);
int TranslateCall (Asm_t * asm_struct, char * ptr);
int TranslateLea (Asm_t * asm_struct, char * ptr);
int TranslateLeaFromMemLabel (Asm_t * asm_struct, char * ptr);
void TranslateLeaFromMemRegWithDisp (Asm_t * asm_struct, char * ptr);
void TranslateLeaFromMemRegNoDisp (Asm_t * asm_struct, char * ptr);
int TranslateMov (Asm_t * asm_struct, char * ptr);
int TranslateMovMemLabelToReg (Asm_t * asm_struct, char * ptr);
int TranslateMovMemRegWithDispToReg (Asm_t * asm_struct, char * ptr);
int TranslateMovMemRegNoDispToReg (Asm_t * asm_struct, char * ptr);
int TranslateMovRegToMemRegWithDisp (Asm_t * asm_struct, char * ptr);
int TranslateMovRegToMemLabel (Asm_t * asm_struct, char * ptr);
int TranlateMovRegToMemRegNoDisp (Asm_t * asm_struct, char * ptr);
int TranslateMovNumToReg (Asm_t * asm_struct, char * ptr);
int TranslateMovRegToReg (Asm_t * asm_struct, char * ptr);

int TranslateImul (Asm_t * asm_struct, char * ptr); // FIXME
int TranslateMovByte (Asm_t * asm_struct);

int AddNamePointer (Asm_t * asm_struct, char * ptr, int var_offset);
void AddLabelPointer (Asm_t * asm_struct, char * ptr, int label_offset);
Jmp_t ParseJmp (char * ptr);
Op_t ParseRegName (char * ptr);
unsigned char CalculateByteForPushPopInstr (char * ptr);
unsigned char MakeModeRm (unsigned char mod, Op_t src, Op_t dst, Order_t order);

void SetCodeSize (int code_size);
void SetDataSizeAndOffset (int data_size, int code_size);
void FwriteAll (FILE * fp_out, Asm_t * asm_struct);

int CheckCollisions (Asm_t * asm_struct);


#endif // ASM_H
