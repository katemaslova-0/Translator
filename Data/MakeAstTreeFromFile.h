#ifndef MAKETREE_H
#define MAKETREE_H

#include "data.h"

Buffer Tokenize (const char * input_file);

void ReadWord       (char ** pos, Buffer * tokens);
void ReadKeyWord    (char ** pos, Buffer * tokens);
void ReadOp         (char ** pos, Buffer * tokens);
void ReadDivider    (char ** pos, Buffer * tokens);
void ReadNum        (char ** pos, Buffer * tokens);
void ReadVarOrFunc  (char ** pos, Buffer * tokens, char * name);

Buffer CopyTreeToBuff (const char * input_file);

char * ReadName (char ** pos);

bool IsMathOp            (char pos);
bool IsAsn               (Buffer * tokens, int current);
bool IsDivider           (Buffer * tokens, int current);
bool IsCloseCurlyBracket (Buffer * tokens, int current);
bool IsOpenCurlyBracket  (Buffer * tokens, int current);
bool IsCloseBracket      (Buffer * tokens, int current);
bool IsOpenBracket       (Buffer * tokens, int current);

int     GetFileSize         (const char * filename);
Buffer  AllocateBuffer      (TypeOfBuffer type, int size);
Node ** AllocateNodeBuff    (int size);
char *  AllocateCharBuff    (int size);
Token * AllocateTokenBuff   (int size);
Function * AllocateFuncBuff (int size);
Variable * AllocateVarBuff  (int size);


Node * MakeTreeFromTokens (Buffer * tokens, Nametables table);

Node * GetG              (Buffer * tokens, Nametables table);
Node * GetString         (Buffer * tokens, int * current, Nametables table);
Node * GetVarOrFunc      (Buffer * tokens, int * current, Nametables table);
Node * GetVar            (Buffer * tokens, int * current, Nametables table);
Node * GetFuncDefinition (Buffer * tokens, int * current, Nametables table);
Node * GetVarEq          (Buffer * tokens, int * current, Nametables table);
Node * GetE              (Buffer * tokens, int * current, Nametables table);
Node * GetP              (Buffer * tokens, int * current, Nametables table);
Node * GetMathFunc       (Buffer * tokens, int * current, Nametables table);
Node * GetFunc           (Buffer * tokens, int * current, bool if_initialized, Nametables table);
Node * GetKeyword        (Buffer * tokens, int * current, Nametables table);
Node * GetIn             (Buffer * tokens, int * current, Nametables table);
Node * GetOut            (Buffer * tokens, int * current, Nametables table);
Node * GetReturn         (Buffer * tokens, int * current, Nametables table);
Node * GetIfOrWhile      (Buffer * tokens, int * current, Nametables table);
Node * GetSetPixel       (Buffer * tokens, int * current, Nametables table);

Node * GetVarNumOrFuncArgs (Buffer * tokens, int * current, int * num_of_args, Nametables table);
Node * GetVarArgs          (Buffer * tokens, int * current, int * num_of_args, Nametables table);

void PutTreeToFile  (const char * filename, Node * root);
void PrintNodeValue (FILE * fp, Node * node);
void PrintNode      (FILE * fp, Node * node);
Node * MakeNewNode (Type_t type, Value_t value, Node * left, Node * right);
Node * AllocateNodeMemory (void);

void SkipBrace (Buffer * tokens, int * current);

void PrintfTokens (Buffer * tokens);
void PrintTokenValue (Token token);

int FindVarAtNametable  (char * name, Buffer * vars);
int FindAtFuncNametable (char * name, Buffer * funcs);
bool IsFuncInitialized  (int index,   Buffer * funcs);
bool AreArgsCorrect     (int index,   int num_of_args, Buffer * funcs);

void NodeDtor (Node * node);

#endif // MAKETREE_H
