#ifndef DATA_H
#define DATA_H

enum Type_t
{
    kNum     = 0,
    kOp      = 1,
    kKeyWord = 2,
    kName    = 3,
    kDivider = 4,
    kComma   = 5
};

enum IfInitialized_t
{
    kNotInit = 0,
    kInit = 1
};

enum Op_t
{
    kAdd                = 0,
    kSub                = 1,
    kMul                = 2,
    kDiv                = 3,
    kGreaterThan        = 4,
    kLessThan           = 5,
    kGreaterThanOrEqual = 6,
    kLessThanOrEqual    = 7,
    kEqual              = 8,
    kNotEqual           = 9,
    kPow                = 10,
    kAsn                = 11,
    kSin                = 12,
    kCos                = 13,
    kTan                = 14,
    kCtg                = 15,
    kLn                 = 16,
    kDif                = 17,
    kSqrt               = 18,
    kSetPixel           = 19,
    kOpenBracket        = 20,
    kCloseBracket       = 21,
    kOpenCurlyBracket   = 22,
    kCloseCurlyBracket  = 23
};

enum KeyWord_t
{
    kIf                 = 0,
    kWhile              = 1,
    kContinue           = 2,
    kBreak              = 3,
    kReturn             = 4,
    kIn                 = 5,
    kOut                = 6,
    kFunc               = 7,
    kVarInit            = 8,
    kElse               = 9,
    kDraw               = 10
};

enum FuncOrVar_t
{
    kFuncName = 1,
    kVarName  = 2
};

struct NameNode
{
    char * name;
    FuncOrVar_t type;
    int index;
};

union Value_t
{
    int num;
    NameNode name;
    Op_t op;
    KeyWord_t keyword;
};

struct Node
{
    Type_t type;
    Value_t value;
    Node * parent;
    Node * left;
    Node * right;
};

struct Asm_data
{
    int * labels;
    int * ram_count;
};

struct Function
{
    char * name;
    int index; // at nametable
    int num_of_args;
    bool if_initialized = false;
    bool if_defined = false;
    Node ** array;
};

struct Variable
{
    char * name;
    int index; // at nametable
    bool if_local;
    int local_var_index; // to find in stack;
};

struct KeyWord
{
    const char * name;
    KeyWord_t number;
};

struct Token
{
    Type_t type;
    Value_t value;
};

union BuffData
{
    Node ** node;
    Function * func;
    Variable * var;
    char * sym;
    Token * token;
};

struct Buffer
{
    BuffData data;
    int size;
};

struct Nametables
{
    Buffer * vars;
    Buffer * funcs;
    bool * is_in_func_now;
    int * local_vars_counter;
};

enum TypeOfBuffer
{
    kNodeBuff  = 0,
    kCharBuff  = 1,
    kTokenBuff = 2,
    kFuncBuff  = 3,
    kVarBuff   = 4
};

#endif // DATA_H
