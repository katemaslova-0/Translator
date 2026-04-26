#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "MakeAstTreeFromFile.h"
#include "backend.h"
#include "data.h"

#define PRINTF_ERROR printf("%s:%d: Error\n", __FUNCTION__, __LINE__);
#define LOCAL_VAR_INDEX table.vars->data.var[node->value.name.index].local_var_index // поменять!
#define GLOBAL_VAR_NAME node->value.name.name
#define FUNC_NAME       table.funcs->data.func[node->value.name.index].name

const int INDEX_ARRAY_SIZE = 5;


bool CheckFuncsFromTable (Nametables table)
{
    for (int count = 0 ; count < table.funcs->size; count++)
    {
        if (!table.funcs->data.func[count].if_initialized)
        {
            printf("Error: func %s has not been initialized\n", table.funcs->data.func[count].name);
            return false;
        }
        if (!table.funcs->data.func[count].if_defined)
        {
            printf("Error: func %s has not been defined\n", table.funcs->data.func[count].name);
            return false;
        }
    }

    return true;
}


void TranslateAstToAsmCommands (const char * output_filename, Node * root, Nametables table)
{
    assert(root);
    assert(output_filename);

    FILE * asm_fp = fopen(output_filename, "w");
    FILE * asm_func_fp = fopen("asm_func.txt", "w");
    FILE * asm_standart_func_fp = fopen("asm_standart_func.txt", "r");

    Files file = {asm_fp, asm_func_fp, asm_standart_func_fp};

    bool is_inside_func = false;

    int labels = 0; // first free label number

    printf("START TABLE.VARS->SIZE == %d\n", table.vars->size);
    // SetVideoMemory();

    fprintf(asm_fp, "default rel\nsection .text\n\nglobal main\n\nmain:\n");

    TranslateString(file, root, &labels, table, NULL, &is_inside_func);

    fprintf(asm_fp, "mov rax, 60\nxor rdi, rdi\nsyscall \n\n\n");
    fclose(asm_func_fp);
    fclose(asm_standart_func_fp);

    CopyFuncsCode(asm_fp, "asm_func.txt", "asm_standart_func.txt");

    SetDataSection(asm_fp, table);

    fclose(asm_fp);
}


void SetDataSection (FILE * asm_fp, Nametables table)
{
    assert(asm_fp);

    fprintf(asm_fp, "\nsection .data\n\nSymBuff times 10 db 0\n\nflag db 0\n");

    int num_of_vars = table.vars->size;

    for (int i = 0; i < num_of_vars; i++)
    {
        if (!table.vars->data.var[i].if_local)
            fprintf(asm_fp, "%s:\t\t\tdq\t0\n", table.vars->data.var[i].name);
    }
}


void CopyFuncsCode (FILE * asm_fp, const char * user_func, const char * st_func)
{
    assert(asm_fp);
    assert(user_func);
    assert(st_func);

    FILE * asm_func_fp = fopen(user_func, "r");
    FILE * asm_standart_func_fp = fopen(st_func, "r");
    assert(asm_func_fp);
    assert(asm_standart_func_fp);

    int ch = '\0';

    while ((ch = fgetc(asm_func_fp)) != EOF)
        fputc(ch, asm_fp);

    fprintf(asm_fp, "\n");

    while ((ch = fgetc(asm_standart_func_fp)) != EOF)
        fputc(ch, asm_fp);

    fclose(asm_func_fp);
    fclose(asm_standart_func_fp);
}


Node * TranslateString (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    printf("TRANSLATE STRING HAS BEEN CALLED. table.vars->size = %d\n", table.vars->size);

    if (node->type == kKeyWord && node->value.keyword == kElse)
        return node;
    if (node->type != kDivider)
        PRINTF_ERROR;

    switch (node->left->type)
    {
        case kName:
                    {
                        if (node->left->value.name.type == kVarName)
                            TranslateVarEq(file, table, node->left, is_inside_func);
                        else if (node->left->value.name.type == kFuncName)
                            TranslateFuncCall(file, node->left, table, is_inside_func);
                        else
                            PRINTF_ERROR;
                        break;
                    }

        case kKeyWord: TranslateKeyWord(file, node->left, labels, table, while_index, is_inside_func); break;
        case kNum:
        case kOp:
                    {
                        if (node->left->value.op == kSetPixel)
                            TranslateSetPixel(file, table, node->left, is_inside_func);
                        else
                            PRINTF_ERROR;
                        break;
                    }
        case kDivider:
        case kComma:
        default:       PRINTF_ERROR;
    }

    if (node->right)
        return TranslateString(file, node->right, labels, table, while_index, is_inside_func);

    return NULL;
}


void TranslateSetPixel (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    fprintf(fp, "PUSH 111\n");
    TranslateEq(file, table, node->left, is_inside_func);
    fprintf(fp, "PUSH 10\n");
    fprintf(fp, "MUL\n");
    TranslateEq(file, table, node->right, is_inside_func);
    fprintf(fp, "ADD\n");
    fprintf(fp, "POPREG DX\n");
    fprintf(fp, "POPM [DX]\n");
}


void TranslateKeyWord (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);

    printf("TRANSLATE KEYWORD HAS BEEN CALLED\n");

    switch(node->value.keyword)
    {
        case kIf:       TranslateIf       (file, node, labels, table, while_index, is_inside_func); break;
        case kWhile:    TranslateWhile    (file, node, labels, table, is_inside_func);              break;
        case kContinue: TranslateContinue (file, node, while_index, is_inside_func);                break;
        case kBreak:    TranslateBreak    (file, node, labels, while_index, is_inside_func);        break;
        case kIn:       TranslateIn       (file, table, node, is_inside_func);                      break;
        case kOut:      TranslateOut      (file, table, node, is_inside_func);                      break;
        case kFunc:     TranslateFuncInit (file, node, labels, table, while_index, is_inside_func); break;
        case kDraw:     TranslateDraw     (file, node, is_inside_func);                             break;
        case kReturn:   TranslateReturn   (file, node, table, is_inside_func);                      break;
        case kVarInit:
        case kElse:
        default:        PRINTF_ERROR;
    }
}


void TranslateReturn (Files file, Node * node, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    Node * curr_node = node->left;
    if (!curr_node) PRINTF_ERROR;

    int local_var_index = table.vars->data.var[curr_node->value.name.index].local_var_index;

    if (curr_node->type == kName)
    {
        if (*is_inside_func)
            fprintf(fp, "mov rax, [rbp + %d * 8d]\n", local_var_index); // changed from macro
        else
            fprintf(fp, "mov rax, [%s]\n", curr_node->value.name.name);
    }
    else if (curr_node->type == kNum)
            fprintf(fp, "mov rax, %d\n", curr_node->value.num);
}


void TranslateDraw (Files file, Node * node, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    fprintf(fp, "DRAW\n");
}


void TranslateFuncCall (Files file, Node * node, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int num_of_args = table.funcs->data.func[node->value.name.index].num_of_args;

    node = node->left;
    const char * func_name = table.funcs->data.func[node->value.name.index].name;

    for (int count = 0; count < num_of_args; count++) // пока только 1 арг!
    {
        if (node->type == kName)
        {
            if (*is_inside_func)
                fprintf(fp, "mov rdi, [rbp + %d * 8d]\n", LOCAL_VAR_INDEX); // только 1 арг в rdi
            else
                fprintf(fp, "mov rdi, [%s]\n", GLOBAL_VAR_NAME);
        }
        else if (node->type == kNum)
            fprintf(fp, "mov rdi, %d\n", node->value.num);
        else if (node->type == kOp)                            // added!
        {
            TranslateEq(file, table, node, is_inside_func);    // added!
            fprintf(fp, "mov rdi, rbx\n");
        }

        node = node->left;
    }

    fprintf(fp, "push rbp\n");
    fprintf(fp, "call %s\n", func_name);
    fprintf(fp, "pop rbp\n");
}


void TranslateFuncInit (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    node = node->left; // move to func name

    if (!node->right)
        return;

    fprintf(file.asm_func, "%s:\n\n", FUNC_NAME);
    fprintf(file.asm_func, "sub rsp, 20 * 8d\n"); // FIXME: фиксированный размер фрейма! исправить, добавив подсчет количества локальных переменных
    fprintf(file.asm_func, "mov rbp, rsp\n");
    fprintf(file.asm_func, "mov [rbp], rdi\n");

    *is_inside_func = true;

    TranslateString(file, node->right, labels, table, while_index, is_inside_func);

    *is_inside_func = false;

    fprintf(file.asm_func, "add rsp, 20 * 8d\n");
    fprintf(file.asm_func, "\nret\n");
}


void TranslateOut (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int local_var_index = table.vars->data.var[node->left->value.name.index].local_var_index;

    if (node->left->type == kName)
    {
        if (*is_inside_func)
            fprintf(fp, "mov rdi, rbp + %d * 8d\n", local_var_index); // changed from macro
        else
            fprintf(fp, "mov rdi, [%s]\n", node->left->value.name.name); // changed from GLOBAL_VAR_NAME
    }
    else if (node->left->type == kOp || node->left->type == kNum)
    {
        TranslateEq(file, table, node->left, is_inside_func);
        fprintf(fp, "mov rdi, rbx\n");
    }
    else
        PRINTF_ERROR;

    fprintf(fp, "call MyPrintf\n");
}


void TranslateIn (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    node = node->left;

    if (*is_inside_func)
        fp = file.asm_func;

    if (*is_inside_func)
        fprintf(fp, "lea rdi, [rbp + %d * 8d]\ncall MyIn\nmov [rbp + %d * 8d], rax\n\n", LOCAL_VAR_INDEX, LOCAL_VAR_INDEX);
    else
        fprintf(fp, "lea rdi, [%s]\ncall MyIn\nmov [%s], rax\n\n", GLOBAL_VAR_NAME, GLOBAL_VAR_NAME);
}


void TranslateBreak (Files file, Node * node, int * labels, int * while_index, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    if (!while_index)
        PRINTF_ERROR;

    fprintf(fp, "jmp .L%d\n", *labels);

    *while_index = *labels;

    (*labels)++;
}


void TranslateContinue (Files file, Node * node, int * while_index, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    if (!while_index)
        PRINTF_ERROR;

    fprintf(fp, "JMP :%d\n", *while_index);
}


void TranslateWhile (Files file, Node * node, int * labels, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(labels);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int while_start = *labels;
    (*labels)++;

    int le_label = *labels;
    (*labels)++;

    int break_index = while_start;

    fprintf(fp, ".L%d:\n", while_start);

    TranslateEq(file, table, node->left, is_inside_func);
    fprintf(fp, ".L%d\n", le_label);

    TranslateString(file, node->right, labels, table, &while_start, is_inside_func);

    fprintf(fp, "jmp .L%d\n", while_start);

    fprintf(fp, ".L%d:\n", le_label);
    if (break_index != while_start)
        fprintf(fp, ".L%d:\n", while_start);
}


void TranslateIf (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    TranslateEq(file, table, node->left, is_inside_func);

    int label1 = *labels;
    int label2 = *labels + 1;
    int label3 = *labels + 2;

    *(labels) += 3;

    fprintf(fp, ".L%d   \t\t\t\t; переход, если if выполнен\n", label1); // добавить комментарии к меткам (добавлять имя файла и номер стрки исходного текста - таблица в конце файла либо дерево)
    fprintf(fp, "jmp .L%d   \t\t\t\t;  иначе - пропуск тела if\n", label2);
    fprintf(fp, ".L%d:      \t\t\t\t; тело if\n", label1);

    Node * ret_node = TranslateString(file, node->right, labels, table, while_index, is_inside_func);

    if (ret_node)
        fprintf(fp, "jmp .L%d     ; пропуск тела else, если выполнен if\n", label3);

    fprintf(fp, ".L%d:                    ; пропуск тела if\n", label2);

    if (ret_node)
    {
        TranslateElse(file, ret_node, labels, table, while_index, is_inside_func);

        fprintf(fp, ".L%d:  ;  пропущен else, выполнен if\n", label3);
    }
}


void TranslateElse (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);

    TranslateString(file, node->right, labels, table, while_index, is_inside_func);
}


void TranslateVarEq (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    if (node->left->left->type == kName)
    {
        TranslateFuncCall(file, node->left->left, table, is_inside_func);

        if (*is_inside_func)
            fprintf(fp, "mov [rbp + %d * 8d], rax\n", LOCAL_VAR_INDEX);
        else
            fprintf(fp, "mov [%s], rax\n", GLOBAL_VAR_NAME);
    }
    else
    {
        TranslateEq(file, table, node->left->left, is_inside_func);

        if (*is_inside_func)
            fprintf(fp, "mov [rbp + %d * 8d], rbx\n", LOCAL_VAR_INDEX);
        else
        fprintf(fp, "mov [%s], rbx\n", GLOBAL_VAR_NAME);
    }
}


void TranslateEq (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    switch (node->type)
    {
        case kNum:  fprintf(fp, "mov rbx, %d\n", node->value.num); break;
        case kName: {
                        if (*is_inside_func)
                            fprintf(fp, "mov rbx, [rbp + %d * 8d]\n", LOCAL_VAR_INDEX);
                        else
                            fprintf(fp, "mov rbx, [%s]\n", GLOBAL_VAR_NAME);
                        break;
                    }
        case kOp:   {
                        TranslateEq(file, table, node->left, is_inside_func);
                        fprintf(fp, "push rbx\n");

                        if (node->value.op != kSqrt && node->value.op != kPow)
                        {
                            TranslateEq(file, table, node->right, is_inside_func);
                        }

                        fprintf(fp, "pop rcx\nxchg rcx, rbx\n");
                        TranslateOp(file, node, is_inside_func);
                        break;
                    }
        case kKeyWord:
        case kDivider:
        case kComma:
        default:        PRINTF_ERROR;
    }
}


void TranslateOp (Files file, Node * node, bool * is_inside_func)
{
    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    switch(node->value.op)
    {
        case kAdd:                  fprintf(fp, "add rbx, rcx\n");     break;
        case kSub:                  fprintf(fp, "sub rbx, rcx\n");     break;
        case kMul:                  fprintf(fp, "mov rax, rbx\nmul rcx\nmov rbx, rax\n");     break;
        case kDiv:                  fprintf(fp, "mov rax, rbx\nxor rdx, rdx\ndiv rcx\nmov rbx, rax\n");     break;
        case kGreaterThan:          fprintf(fp, "cmp rbx, rcx\njg ");  break;
        case kLessThan:             fprintf(fp, "cmp rbx, rcx\njl ");  break;
        case kGreaterThanOrEqual:   fprintf(fp, "cmp rbx, rcx\njge "); break;
        case kLessThanOrEqual:      fprintf(fp, "cmp rbx, rcx\njle "); break;
        case kEqual:                fprintf(fp, "cmp rbx, rcx\nje ");  break;
        case kNotEqual:             fprintf(fp, "cmp rbx, rcx\njne "); break;
        case kSqrt:     {
                            fprintf(fp, "cvtsi2sd xmm0, rbx\nsqrtsd xmm0, xmm0\ncvttsd2si rbx, xmm0\n\n");
                            break;
                        }
        case kSetPixel:
        case kPow: // доделать mov rax, rbx\n
        case kSin: //
        case kCos: //
        case kTan: //
        case kCtg: //
        case kLn:
        case kDif:
        case kOpenBracket:
        case kCloseBracket:
        case kOpenCurlyBracket:
        case kCloseCurlyBracket:
        case kAsn:
        default:    PRINTF_ERROR;
    }
}


void FreeNametable (Nametables table)
{
    for (int count = 0; count < table.funcs->size; count++)
        free(table.funcs->data.func[count].array);
}
