#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

#include "MakeAstTreeFromFile.h"
#include "backend.h"
#include "dsl.h"

#define PRINTF_ERROR printf("%s:%d: Error\n", __FUNCTION__, __LINE__);
#define LOCAL_VAR_INDEX table.vars->data.var[node->value.name.index].local_var_index
#define GLOBAL_VAR_NAME node->value.name.name
#define FUNC_NAME       table.funcs->data.func[node->value.name.index].name

const int INDEX_ARRAY_SIZE = 5;
const int NUM_OF_NAMES = 20;
const int NUM_OF_LABELS = 20;
const int NUM_OF_ACCESSES = 20;
const int LENGTH_OF_FUNC_NAME = 20;
const int BYTE_ARRAY_SIZE = 8000;


// Check if all the funcs are initialized and defined before translation

bool CheckFuncsFromTable (Nametables table)
{
    int num_of_funcs = table.funcs->size;

    for (int count = 0; count < num_of_funcs; count++)
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

// Prints info about all funcs and vars in the nametable

void PrintFuncsAndVars (Nametables table)
{
    int num_of_funcs = table.funcs->size;
    int num_of_vars  = table.vars->size;

    for (int count = 0; count < num_of_funcs; count++)
    printf("FUNC [%d] %s\n", count, table.funcs->data.func[count].name);

    for (int count = 0; count < num_of_vars; count++)
    {
        printf("VAR [%d] %s [%d]\n", count, table.vars->data.var[count].name, table.vars->data.var[count].index);
        if (table.vars->data.var[count].if_local)
            printf("        local! num: %d\n", table.vars->data.var[count].local_var_index);
    }
}

// Copies all the funcs and vars to asm struct from nametable

void CopyFuncsAndVarsToAsmStruct (Asm_t * asm_struct, Nametables table)
{
    assert(asm_struct);

    int num_of_funcs = table.funcs->size;
    int num_of_vars  = table.vars->size;

    for (int count = 0; count < num_of_funcs; count++)
    {
        asm_struct->names[count].name = table.funcs->data.func[count].name;
        asm_struct->names[count].type = FUNC;
        asm_struct->name_counter++;
    }

    for (int count = 0; count < num_of_vars; count++)
    {
        if (!table.vars->data.var[count].if_local)
            FillVar(asm_struct, table.vars->data.var[count].name, 8);
    }
}

// Adds all the standart funcs to asm_struct

void AddStandartFuncsAndVideomemory (Asm_t * asm_struct)
{
    assert(asm_struct);

    FillFuncCall(asm_struct, "Draw");
    FillFuncCall(asm_struct, "MyIn");
    FillFuncCall(asm_struct, "MyPrintf");

    FillVar(asm_struct, "VIDEOMEMORY", 100);
}


int TranslateAstToAsmCommands (const char * output_filename, Node * root, Nametables table)
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
    fprintf(asm_fp, "default rel\nsection .text\n\nglobal main\n\nmain:\n");

    TranslateString(file, root, &labels, table, NULL, &is_inside_func);

    fprintf(asm_fp, "mov eax, 60\nxor rdi, rdi\nsyscall\n\n\n");

    int res = 0;
    MOV_NUM_TO_REG(RAX, 60);
    XOR_REG_REG(RDI);
    SYSCALL;

    AddDrawFuncBytes(table.asm_struct);
    AddMyInFuncBytes(table.asm_struct);
    AddMyPrintfFuncBytes(table.asm_struct);

    table.asm_struct->start_of_data_segment = table.asm_struct->byte_counter;

    if ((res = AddVarsOffsets(table.asm_struct)) != 0)
        return -1;

    fclose(asm_func_fp);
    fclose(asm_standart_func_fp);

    CopyFuncsCode(asm_fp, "asm_func.txt", "asm_standart_func.txt");

    SetDataSection(asm_fp, table);

    fclose(asm_fp);

    return 0;
}


void SetDataSection (FILE * asm_fp, Nametables table)
{
    assert(asm_fp);

    fprintf(asm_fp, "\nsection .data\n\nSymBuff: db 0\nflag: db 0\n");

    int num_of_vars = table.vars->size;

    fprintf(asm_fp, "VIDEOMEMORY: times 100 db ' '\nnewline: db `\\n`\n");

    for (int i = 0; i < num_of_vars; i++)
    {
        if (!table.vars->data.var[i].if_local)
            fprintf(asm_fp, "%s: dq 0\n", table.vars->data.var[i].name);
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


int TranslateSetPixel (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    TranslateEq(file, table, node->left, is_inside_func); // res in rbx

    fprintf(fp, "imul rbx, rbx, 10d\n");
    IMUL_REG_REG_10(RBX);
    fprintf(fp, "push rbx\n");
    PUSH_REG(RBX);

    TranslateEq(file, table, node->right, is_inside_func);

    fprintf(fp, "pop rcx\n");
    POP_REG(RCX);
    fprintf(fp, "add rcx, rbx\n");
    ADD_REG_REG(RBX, RCX);
    fprintf(fp, "push rax\n");
    PUSH_REG(RAX);
    fprintf(fp, "lea rax, [VIDEOMEMORY]\n");
    LEA_RAX_VIDEOMEMORY;
    fprintf(fp, "mov byte [rax + rcx], 'o'\n");
    MOV_BYTE; // for now only for this specific construction!
    fprintf(fp, "pop rax\n");
    POP_REG(RAX);

    return 0;
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
        case kDraw:     TranslateDraw     (file, node, table, is_inside_func);                      break;
        case kReturn:   TranslateReturn   (file, node, table, is_inside_func);                      break;
        case kVarInit:
        case kElse:
        default:        PRINTF_ERROR;
    }
}


int TranslateReturn (Files file, Node * node, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    Node * curr_node = node->left;
    if (!curr_node) PRINTF_ERROR;

    int local_var_index = table.vars->data.var[curr_node->value.name.index].local_var_index;

    if (curr_node->type == kName)
    {
        if (*is_inside_func)
        {
            fprintf(fp, "mov rax, [rbp + %d * 8d]\n", local_var_index);
            MOV_RAX_FROM_MEM_RBP_WITH_DISP;
        }
        else
        {
            fprintf(fp, "mov rax, [%s]\n", curr_node->value.name.name);
            MOV_RAX_FROM_MEM_LABEL;
        }
    }
    else if (curr_node->type == kNum)
    {
        fprintf(fp, "mov eax, %d\n", curr_node->value.num);
        MOV_NUM_TO_REG(RAX, curr_node->value.num);
    }

    return 0;
}


int TranslateDraw (Files file, Node * node, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    fprintf(fp, "push rbp\ncall Draw\npop rbp\n");
    PUSH_REG(RBP);
    CALL_FUNC_NAME("Draw");
    POP_REG(RBP);

    return 0;
}


int TranslateFuncCall (Files file, Node * node, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    int num_of_args = table.funcs->data.func[node->value.name.index].num_of_args;

    node = node->left;
    const char * func_name = table.funcs->data.func[node->value.name.index].name;

    for (int count = 0; count < num_of_args; count++) // пока только 1 арг!
    {
        if (node->type == kName)
        {
            if (*is_inside_func)
            {
                fprintf(fp, "mov rdi, [rbp + %d * 8d]\n", LOCAL_VAR_INDEX); // только 1 арг в rdi
                MOV_RDI_FROM_MEM_RBP_WITH_DISP;
            }
            else
            {
                fprintf(fp, "mov rdi, [%s]\n", GLOBAL_VAR_NAME);
                MOV_RDI_LABEL_GLOBAL_VAR_NAME;
            }
        }
        else if (node->type == kNum)
        {
            fprintf(fp, "mov edi, %d\n", node->value.num);
            MOV_NUM_TO_REG(RDI, node->value.num);
        }
        else if (node->type == kOp)                            // added!
        {
            TranslateEq(file, table, node, is_inside_func);    // added!
            fprintf(fp, "mov rdi, rbx\n");
            MOV_REG_REG(RBX, RDI);
        }

        node = node->left;
    }

    fprintf(fp, "push rbp\n");
    PUSH_REG(RBP);
    fprintf(fp, "call %s\n", func_name);
    CALL_FUNC_NAME(func_name);
    fprintf(fp, "pop rbp\n");
    POP_REG(RBP);

    return 0;
}


int TranslateFuncInit (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);
    assert(is_inside_func);

    node = node->left; // move to func name

    if (!node->right)
        return 0;

    int res = 0;

    // MOV_NUM_TO_REG(RAX, 60);
    // XOR_REG_REG(RDI);
    // SYSCALL;

    fprintf(file.asm_func, "%s:\n\n", FUNC_NAME);
    if ((res = AddFunctionOffset(table.asm_struct, FUNC_NAME)) != 0) return -1;

    fprintf(file.asm_func, "sub rsp, 20 * 8d\n"); // FIXME: фиксированный размер фрейма! исправить, добавив подсчет количества локальных переменных
    SUB_REG_NUM(RSP, 20 * 8);

    fprintf(file.asm_func, "mov rbp, rsp\n");
    MOV_REG_REG(RSP, RBP);
    fprintf(file.asm_func, "mov [rbp], rdi\n");
    MOV_RDI_TO_MEM_RBP;

    *is_inside_func = true;

    TranslateString(file, node->right, labels, table, while_index, is_inside_func);

    *is_inside_func = false;

    fprintf(file.asm_func, "add rsp, 20 * 8d\n");
    ADD_REG_NUM(RSP, 20 * 8);
    fprintf(file.asm_func, "\nret\n");
    RET;

    return 0;
}


int TranslateOut (Files file, Nametables table, Node * node, bool * is_inside_func) // FIXME
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    int local_var_index = table.vars->data.var[node->left->value.name.index].local_var_index;

    if (node->left->type == kName)
    {
        if (*is_inside_func)
        {
            fprintf(fp, "mov rdi, rbp\nadd rdi, %d * 8d\n", local_var_index); // changed from macro
            MOV_REG_REG(RBP, RDI);
            ADD_REG_NUM(RDI, 8 * local_var_index);
        }
        else
        {
            fprintf(fp, "mov rdi, [%s]\n", node->left->value.name.name); // changed from GLOBAL_VAR_NAME
            MOV_RDI_MEM_LABEL;
        }
    }
    else if (node->left->type == kOp || node->left->type == kNum)
    {
        TranslateEq(file, table, node->left, is_inside_func);
        fprintf(fp, "mov rdi, rbx\n");
        MOV_REG_REG(RBX, RDI);
    }
    else
        PRINTF_ERROR;

    fprintf(fp, "call MyPrintf\n");
    CALL_FUNC_NAME("MyPrintf");

    return 0;
}


int TranslateIn (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    node = node->left;

    int res = 0;

    if (*is_inside_func)
        fp = file.asm_func;

    if (*is_inside_func)
    {
        fprintf(fp, "lea rdi, [rbp + %d * 8d]\ncall MyIn\nmov [rbp + %d * 8d], rax\n\n", LOCAL_VAR_INDEX, LOCAL_VAR_INDEX);
        LEA_RDI_RBP_WITH_DISP;
        CALL_FUNC_NAME("MyIn");
        MOV_RAX_TO_MEM_RBP_WITH_DISP;
    }
    else
    {
        fprintf(fp, "lea rdi, [%s]\ncall MyIn\nmov [%s], rax\n\n", GLOBAL_VAR_NAME, GLOBAL_VAR_NAME);
        LEA_RDI_GLOBAL_VAR_NAME;
        CALL_FUNC_NAME("MyIn");
        MOV_RAX_TO_MEM_LABEL;
    }

    return 0;
}

// FIXME: not available!
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

// FIXME: not available!
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


int TranslateWhile (Files file, Node * node, int * labels, Nametables table, bool * is_inside_func)
{
    assert(node);
    assert(labels);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    int while_start = *labels;
    (*labels)++;

    int le_label = *labels;
    (*labels)++;

    if ((res = AddLabel(table.asm_struct, while_start)) != 0) return -1;
    if ((res = AddLabel(table.asm_struct, le_label)) != 0)    return -1;

    int break_index = while_start;

    fprintf(fp, ".L%d:\n", while_start); // fix!!!
    AddLabelOffset(table.asm_struct, while_start);
    *(table.if_inside_while) = true;
    TranslateEq(file, table, node->left, is_inside_func);
    fprintf(fp, ".L%d\n", le_label);
    LABEL_AFTER_JUMP(le_label);

    *(table.if_inside_while) = false;

    TranslateString(file, node->right, labels, table, &while_start, is_inside_func);

    fprintf(fp, "jmp .L%d\n", while_start);
    JUMP(JMP);
    LABEL_AFTER_JUMP(while_start);

    fprintf(fp, ".L%d:\n", le_label);
    AddLabelOffset(table.asm_struct, le_label);

    if (break_index != while_start)
    {
        printf("ERROR: BREAK FOUND(unavailable now!!)\n");
        fprintf(fp, ".L%d:\n", while_start);
    }

    return 0;
}


int TranslateIf (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    TranslateEq(file, table, node->left, is_inside_func);

    int label1 = *labels;
    int label2 = *labels + 1;
    int label3 = *labels + 2;

    *(labels) += 3;

    if ((res = AddLabel(table.asm_struct, label1)) != 0) return -1;
    if ((res = AddLabel(table.asm_struct, label2)) != 0) return -1;
    if ((res = AddLabel(table.asm_struct, label3)) != 0) return -1;

    fprintf(fp, ".L%d   \t\t\t\t; переход, если if выполнен\n", label1);
    LABEL_AFTER_JUMP(label1);
    fprintf(fp, "jmp .L%d   \t\t\t\t;  иначе - пропуск тела if\n", label2);
    JUMP(JMP);
    LABEL_AFTER_JUMP(label2);
    fprintf(fp, ".L%d:      \t\t\t\t; тело if\n", label1);
    AddLabelOffset(table.asm_struct, label1);

    Node * ret_node = TranslateString(file, node->right, labels, table, while_index, is_inside_func);

    if (ret_node)
    {
        fprintf(fp, "jmp .L%d     ; пропуск тела else, если выполнен if\n", label3);
        JUMP(JMP);
        LABEL_AFTER_JUMP(label3);
    }

    fprintf(fp, ".L%d:                    ; пропуск тела if\n", label2);
    AddLabelOffset(table.asm_struct, label2);

    if (ret_node)
    {
        TranslateElse(file, ret_node, labels, table, while_index, is_inside_func);

        fprintf(fp, ".L%d:  ;  пропущен else, выполнен if\n", label3);
        AddLabelOffset(table.asm_struct, label3);
    }

    return 0;
}


void TranslateElse (Files file, Node * node, int * labels, Nametables table, int * while_index, bool * is_inside_func)
{
    assert(node);

    TranslateString(file, node->right, labels, table, while_index, is_inside_func);
}


int TranslateVarEq (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    if (node->left->left->type == kName)
    {
        TranslateFuncCall(file, node->left->left, table, is_inside_func);

        if (*is_inside_func)
        {
            fprintf(fp, "mov [rbp + %d * 8d], rax\n", LOCAL_VAR_INDEX);
            MOV_RAX_TO_MEM_RBP_WITH_DISP;
        }
        else
        {
            fprintf(fp, "mov [%s], rax\n", GLOBAL_VAR_NAME);
            MOV_RAX_TO_MEM_GLOBAL_VAR_NAME;
        }
    }
    else
    {
        TranslateEq(file, table, node->left->left, is_inside_func);

        if (*is_inside_func)
        {
            fprintf(fp, "mov [rbp + %d * 8d], rbx\n", LOCAL_VAR_INDEX);
            MOV_RBX_TO_MEM_RBP_WITH_DISP;
        }
        else
        {
            fprintf(fp, "mov [%s], rbx\n", GLOBAL_VAR_NAME);
            MOV_RBX_TO_MEM_LABEL;
        }
    }

    return 0;
}


int TranslateEq (Files file, Nametables table, Node * node, bool * is_inside_func)
{
    assert(node);

    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    switch (node->type)
    {
        case kNum:  {fprintf(fp, "mov ebx, %d\n", node->value.num); MOV_NUM_TO_REG(RBX, node->value.num);}break; // changed
        case kName: {
                        if (*is_inside_func)
                        {
                            fprintf(fp, "mov rbx, [rbp + %d * 8d]\n", LOCAL_VAR_INDEX);
                            MOV_MEM_RBP_WITH_DISP_TO_RBX;
                        }
                        else
                        {
                            fprintf(fp, "mov rbx, [%s]\n", GLOBAL_VAR_NAME);
                            MOV_MEM_LABEL_TO_RBX;
                        }
                        break;
                    }
        case kOp:   {
                        TranslateEq(file, table, node->left, is_inside_func);
                        fprintf(fp, "push rbx\n");
                        PUSH_REG(RBX);

                        if (node->value.op != kSqrt && node->value.op != kPow)
                        {
                            TranslateEq(file, table, node->right, is_inside_func);
                        }

                        fprintf(fp, "pop rcx\nxchg rcx, rbx\n");
                        POP_REG(RCX);
                        XCHG_REG_REG(RCX, RBX);
                        TranslateOp(file, node, is_inside_func, table);
                        break;
                    }
        case kKeyWord:
        case kDivider:
        case kComma:
        default:        PRINTF_ERROR;
    }

    return 0;
}


int TranslateOp (Files file, Node * node, bool * is_inside_func, Nametables table)
{
    FILE * fp = file.asm_code;

    if (*is_inside_func)
        fp = file.asm_func;

    int res = 0;

    if (*(table.if_inside_while))
    {
        fprintf(fp, "xchg rbx, rcx\n");
        XCHG_REG_REG(RBX, RCX);
    }

    switch(node->value.op)
    {
        case kAdd:                  {fprintf(fp, "add rbx, rcx\n"); ADD_REG_REG(RCX, RBX);}    break;
        case kSub:                  {fprintf(fp, "sub rbx, rcx\n"); SUB_REG_REG(RCX, RBX);}    break;
        case kMul:                  {fprintf(fp, "mov rax, rbx\nmul rcx\nmov rbx, rax\n"); MOV_REG_REG(RBX, RAX); MUL_REG(RCX); MOV_REG_REG(RAX, RBX);}     break;
        case kDiv:                  {fprintf(fp, "mov rax, rbx\nxor rdx, rdx\ndiv rcx\nmov rbx, rax\n"); MOV_REG_REG(RBX, RAX); XOR_REG_REG(RDX); DIV_REG(RCX); MOV_REG_REG(RAX, RBX);}break;
        case kGreaterThan:          {fprintf(fp, "cmp rbx, rcx\njg ");  CMP_REG_REG(RCX, RBX); JUMP(JG);}  break;
        case kLessThan:             {fprintf(fp, "cmp rbx, rcx\njl ");  CMP_REG_REG(RCX, RBX); JUMP(JL);}  break;
        case kGreaterThanOrEqual:   {fprintf(fp, "cmp rbx, rcx\njge "); CMP_REG_REG(RCX, RBX); JUMP(JGE);} break;
        case kLessThanOrEqual:      {fprintf(fp, "cmp rbx, rcx\njle "); CMP_REG_REG(RCX, RBX); JUMP(JLE);} break;
        case kEqual:                {fprintf(fp, "cmp rbx, rcx\nje ");  CMP_REG_REG(RCX, RBX); JUMP(JE);}  break;
        case kNotEqual:             {fprintf(fp, "cmp rbx, rcx\njne "); CMP_REG_REG(RCX, RBX); JUMP(JNE);} break;
        case kSqrt:     {
                            fprintf(fp, "cvtsi2sd xmm0, rbx\nsqrtsd xmm0, xmm0\ncvtsd2si rbx, xmm0\n\n");
                            CVTSI2SD_XMM0_RBX;
                            SQRTSD_XMM0_XMM0;
                            CVTSD2SI_RBX_XMM0;
                            break;
                        }
        case kSetPixel:
        case kPow: //
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

    return 0;
}


void FreeNametable (Nametables table)
{
    for (int count = 0; count < table.funcs->size; count++)
        free(table.funcs->data.func[count].array);
}

// ASM FUNCS

Asm_t InitAsmStruct (void)
{
    Asm_t asm_struct = {};

    asm_struct.bytes = (unsigned char *) calloc ((size_t) BYTE_ARRAY_SIZE, sizeof(unsigned char));
    asm_struct.names  = (Name_t *)  calloc ((size_t) NUM_OF_NAMES,  sizeof(Name_t));
    asm_struct.labels = (Label_t *) calloc ((size_t) NUM_OF_LABELS, sizeof(Label_t));

    for (int count = 0; count < NUM_OF_NAMES; count++)
        asm_struct.names[count].byte_pointers = (int *) calloc ((size_t)NUM_OF_ACCESSES, sizeof(int));

    for (int count = 0; count < NUM_OF_LABELS; count++)
        asm_struct.labels[count].byte_pointers = (int *) calloc ((size_t)NUM_OF_ACCESSES, sizeof(int));

    return asm_struct;
}


void FillFuncCall (Asm_t * asm_struct, const char * func_name)
{
    assert(asm_struct);
    assert(func_name);

    char * name = (char *) calloc ((size_t) LENGTH_OF_FUNC_NAME, sizeof(char));
    assert(name);

    sscanf(func_name, "%s", name);
    printf("SCANFED IN FILL FUNC CALL: %s\n", name);

    int name_counter = asm_struct->name_counter;

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(asm_struct->names[count].name, func_name) == 0)
        {
            free(name);
            return;
        }
    }

    asm_struct->names[name_counter].name = name;
    asm_struct->names[name_counter].type = FUNC;
    asm_struct->name_counter++;
}


void FillVar (Asm_t * asm_struct, char * name, int size)
{
    assert(asm_struct);
    assert(name);

    asm_struct->names[asm_struct->name_counter].name = name;
    asm_struct->names[asm_struct->name_counter].type = VAR;
    asm_struct->names[asm_struct->name_counter].size = size;
    asm_struct->name_counter++;
}


void MakeBinFile (Asm_t * asm_struct)
{
    assert(asm_struct);

    FILE * fp_out = fopen("asm_output", "w+b");
    assert(fp_out);

    int code_size = asm_struct->start_of_data_segment;
    int data_size = asm_struct->byte_counter - code_size;

    SetCodeSize(code_size);
    SetDataSizeAndOffset(data_size, code_size);

    FwriteAll(fp_out, asm_struct);

    fclose(fp_out);
    chmod("asm_output", 0755);
}


void DestroyAll (Asm_t * asm_struct)
{
    for (int count = 0; count < NUM_OF_NAMES; count++)
    {
        free(asm_struct->names[count].byte_pointers);
        //free(asm_struct->names[count].name);
    }

    for (int count = 0; count < NUM_OF_LABELS; count++)
        free(asm_struct->labels[count].byte_pointers);

    free(asm_struct->names);
    free(asm_struct->labels);

    free(asm_struct->buffer);
    free(asm_struct->bytes);
    free(asm_struct->lines);
}
