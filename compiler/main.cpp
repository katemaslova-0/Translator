#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

#include "MakeAstTreeFromFile.h"
#include "backend.h"
#include "GraphDump.h"

const int NUM_OF_VARS = 20;
const int NUM_OF_FUNCS = 20;

int main (int argc, char * argv[])
{
    const char * input_file = "sqrt.txt";

    if (argc == 2)
        input_file = argv[1];
    else if (argc > 2)
    {
        printf("Invalid cmd line arguments\n");
        return -1;
    }

    const char * asm_file = "ASM.s";

    Buffer tokens = Tokenize(input_file);

    #ifdef NDEBUG
    PrintfTokens(&tokens);
    #endif

    Buffer var_names  = AllocateBuffer(kVarBuff, NUM_OF_VARS);
    Buffer func_names = AllocateBuffer(kFuncBuff, NUM_OF_FUNCS);

    bool is_in_func_now = false;
    bool if_inside_while = false;
    int local_vars_counter = 0;
    Nametables table = {&var_names, &func_names, &is_in_func_now, &local_vars_counter, &if_inside_while};

    Node * root = MakeTreeFromTokens(&tokens, table);

    #ifdef NDEBUG
    MakeGraphCodeFile(root);
    PrintFuncsAndVars(table);
    #endif

    if (!CheckFuncsFromTable(table)) return -1;

    #ifdef NDEBUG
    const char * tree_file = "TREE_FILE.txt";
    PutTreeToFile(tree_file, root);
    MakeGraphCodeFile(root);
    #endif

    Asm_t asm_struct = InitAsmStruct();
    CopyFuncsAndVarsToAsmStruct(&asm_struct, table);

    AddStandartFuncsAndVideomemory(&asm_struct); // добавление стандартных функций в asm_struct

    // заполнили имена функций и переменных. метки внесем по ходу компиляции

    table.asm_struct = &asm_struct;
    TranslateAstToAsmCommands(asm_file, root, table);

    int res = 0;
    if ((res = Link(&asm_struct)) != 0) // линковка
        printf("Error while linking\n");

    // сборка бинарника

    MakeBinFile(&asm_struct);

    DestroyAll(&asm_struct);

    NodeDtor(root);
    free(tokens.data.token);
    free(var_names.data.var);
    FreeNametable(table);
    free(func_names.data.func);

    return 0;
}
