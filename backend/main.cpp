#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
    const char * tree_file = "TREE_FILE.txt";

    Buffer tokens = Tokenize(input_file);

    PrintfTokens(&tokens);

    Buffer var_names  = AllocateBuffer(kVarBuff, NUM_OF_VARS);
    Buffer func_names = AllocateBuffer(kFuncBuff, NUM_OF_FUNCS);

    bool is_in_func_now = false;
    int local_vars_counter = 0;
    Nametables table = {&var_names, &func_names, &is_in_func_now, &local_vars_counter};

    Node * root = MakeTreeFromTokens(&tokens, table);

    MakeGraphCodeFile(root);

    for (int count = 0; count < table.funcs->size; count++)
        printf("FUNC [%d] %s\n", count, table.funcs->data.func[count].name);

    for (int count = 0; count < table.vars->size; count++)
    {
        printf("VAR [%d] %s [%d]\n", count, table.vars->data.var[count].name, table.vars->data.var[count].index);
        if (table.vars->data.var[count].if_local)
            printf("        local! num: %d\n", table.vars->data.var[count].local_var_index);
    }

    if (!CheckFuncsFromTable(table)) return -1;

    PutTreeToFile(tree_file, root);

    MakeGraphCodeFile(root);

    TranslateAstToAsmCommands(asm_file, root, table);

    NodeDtor(root);
    free(tokens.data.token);
    free(var_names.data.var);
    FreeNametable(table);
    free(func_names.data.func);

    return 0;
}
