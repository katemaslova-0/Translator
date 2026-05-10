const int FUNC_NAME_LENGTH = 20;

enum FuncOrVarAsm_t
{
    FUNC,
    VAR
};

struct Name_t
{
    char * name;
    FuncOrVarAsm_t type;
    int offset;
    int * byte_pointers;
    int size_of_byte_pointers_array = 0;
    int size; // size of var; only used for vars
};

struct Label_t
{
    int number;
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

enum Operand_t
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


// TRANSLATE FUNCS

int TranslateMovRegToReg            (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateMovNumToReg            (Asm_t * asm_struct, int value, Operand_t dst);
int TranslateMovRegToMemRegWithDisp (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst);
int TranslateMovRegToMemLabel       (Asm_t * asm_struct, Operand_t src, const char * name_ptr);
int TranslateMovRegToMemRegNoDisp   (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateMovMemRegWithDispToReg (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst);
int TranslateMovMemRegNoDispToReg   (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateMovMemLabelToReg       (Asm_t * asm_struct, Operand_t dst, const char * name_ptr);
int TranslateMovByte                (Asm_t * asm_struct);
int TranslateLeaFromMemRegNoDisp    (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateLeaFromMemRegWithDisp  (Asm_t * asm_struct, int disp, Operand_t src, Operand_t dst);
int TranslateLeaFromMemLabel        (Asm_t * asm_struct, Operand_t dst, const char * name_ptr);
int TranslateCall                   (Asm_t * asm_struct, const char * name_ptr);
int TranslatePush                   (Asm_t * asm_struct, Operand_t reg);
int TranslatePop                    (Asm_t * asm_struct, Operand_t reg);
int TranslateXor                    (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateSyscall                (Asm_t * asm_struct);
int TranslateAddOrSubTwoRegs        (MathOp_t type, Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateAddOrSubRegAndNum      (MathOp_t type, Asm_t * asm_struct, int value, Operand_t dst);
int TranslateMulOrDiv               (MathOp_t type, Asm_t * asm_struct, Operand_t reg);
int TranslateXcng                   (Asm_t * asm_struct, Operand_t reg1, Operand_t reg2);
int TranslateCmp                    (Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateJmp                    (Asm_t * asm_struct, Jmp_t jump);
int TranslateLabelAfterJump         (Asm_t * asm_struct, int label_number);
int TranslateCvtTypeSpecified       (Cvt_t type, Asm_t * asm_struct, Operand_t src, Operand_t dst);
int TranslateSqr                    (Asm_t * asm_struct);
int TranslateRet                    (Asm_t * asm_struct);
int TranslateImul                   (Asm_t * asm_struct, Operand_t src, Operand_t dst);

unsigned char CalculateByteForPushPopInstr (Operand_t reg);
unsigned char MakeModeRm (unsigned char mod, Operand_t src, Operand_t dst, Order_t order);

int AddNamePointer  (Asm_t * asm_struct, const char * ptr, int var_offset);
int AddLabelPointer (Asm_t * asm_struct, int label_number, int label_offset);

int AddFunctionOffset (Asm_t * asm_struct, const char * ptr);
int AddLabel          (Asm_t * asm_struct, int label_number);
int AddLabelOffset    (Asm_t * asm_struct, int label_number);


int Link              (Asm_t * asm_struct);
void WriteOffsetToByteArray (Asm_t * asm_struct, int offset, int byte_pointer);
int AddVarsOffsets (Asm_t * asm_struct);


void SetCodeSize (int code_size);
void SetDataSizeAndOffset (int data_size, int code_size);
void FwriteAll (FILE * fp_out, Asm_t * asm_struct);

void AddDrawFuncBytes (Asm_t * asm_struct);
void AddMyInFuncBytes (Asm_t * asm_struct);
void AddMyPrintfFuncBytes (Asm_t * asm_struct);


void MakeStdFuncArrays (void);
