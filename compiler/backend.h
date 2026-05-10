#ifndef BACKEND_H
#define BACKEND_H

#include "data.h"

struct Files
{
    FILE * asm_code;
    FILE * asm_func;
    FILE * asm_standart_func;
};

bool CheckFuncsFromTable            (Nametables table);
Nametables InitNametable            (void);
void PrintFuncsAndVars              (Nametables table);
void AddStandartFuncsAndVideomemory (Asm_t * asm_struct);

void CopyFuncsAndVarsToAsmStruct (Asm_t * asm_struct, Nametables table);
int TranslateAstToAsmCommands (const char * output_filename, Node * root, Nametables table);
Node * TranslateString         (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateKeyWord          (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
int TranslateFuncCall         (Files file, Node * node, Nametables table, bool * is_inside_func);
int TranslateFuncInit         (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
int TranslateOut              (Files file, Nametables table, Node * node, bool * is_inside_func);
int TranslateIn               (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateBreak            (Files file, Node * node, int * labels, int * while_index, bool * is_inside_func);
void TranslateContinue         (Files file, Node * node, int * while_index, bool * is_inside_func);
int TranslateWhile            (Files file, Node * node, int * labels, Nametables table, bool * is_inside_func);
int TranslateIf               (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateElse             (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
int TranslateVarEq            (Files file, Nametables table, Node * node, bool * is_inside_func);
int TranslateEq               (Files file, Nametables table, Node * node, bool * is_inside_func);
int TranslateOp               (Files file, Node * node, bool * is_inside_func, Nametables table);
int TranslateReturn           (Files file, Node * node, Nametables table, bool * is_inside_func);
int TranslateSetPixel         (Files file, Nametables table, Node * node, bool * is_inside_func);
int TranslateDraw             (Files file, Node * node, Nametables table, bool * is_inside_func);

void FreeNametable (Nametables table);
void CopyFuncsCode (FILE * asm_fp, const char * user_func, const char * st_func);

void SetDataSection (FILE * asm_fp, Nametables table);

// ASM
Asm_t InitAsmStruct (void);
void FillFuncCall (Asm_t * asm_struct, const char * func_name);
void DestroyAll   (Asm_t * asm_struct);
void FillVar      (Asm_t * asm_struct, char * name, int size);

void MakeBinFile (Asm_t * asm_struct);


#endif // BACKEND_H
