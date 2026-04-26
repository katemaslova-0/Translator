#ifndef BACKEND_H
#define BACKEND_H

#include "data.h"

struct Files
{
    FILE * asm_code;
    FILE * asm_func;
    FILE * asm_standart_func;
};

bool CheckFuncsFromTable       (Nametables table);
void TranslateAstToAsmCommands (const char * output_filename, Node * root, Nametables table);
Node * TranslateString         (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateKeyWord          (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateFuncCall         (Files file, Node * node, Nametables table, bool * is_inside_func);
void TranslateFuncInit         (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateOut              (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateIn               (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateBreak            (Files file, Node * node, int * labels, int * while_index, bool * is_inside_func);
void TranslateContinue         (Files file, Node * node, int * while_index, bool * is_inside_func);
void TranslateWhile            (Files file, Node * node, int * labels, Nametables table, bool * is_inside_func);
void TranslateIf               (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateElse             (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func);
void TranslateVarEq            (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateEq               (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateOp               (Files file, Node * node, bool * is_inside_func);
void TranslateReturn           (Files file, Node * node, Nametables table, bool * is_inside_func);
void TranslateSetPixel         (Files file, Nametables table, Node * node, bool * is_inside_func);
void TranslateDraw             (Files file, Node * node, bool * is_inside_func);

void FreeNametable (Nametables table);
void CopyFuncsCode (FILE * asm_fp, const char * user_func, const char * st_func);

void SetDataSection (FILE * asm_fp, Nametables table);


#endif // BACKEND_H
