#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "data.h"
#include "MakeAstTreeFromFile.h"
#include "GraphDump.h"

const int NUM_OF_TOKENS = 1000;
const int NUM_OF_KEY_WORDS = 10;
const int LENTH_OF_WORD = 100;
const int ALPH_LENGTH = 33;

const KeyWord key_words[NUM_OF_KEY_WORDS] = {{"if", kIf},
                                             {"while", kWhile},
                                             {"continue", kContinue},
                                             {"break", kBreak},
                                             {"return", kReturn},
                                             {"in", kIn},
                                             {"out", kOut},
                                             {"func", kFunc},
                                             {"else", kElse},
                                             {"draw", kDraw}};

#define PRINTF_ERROR printf("%s:%d: Error\n", __FUNCTION__, __LINE__);

#define MAKE_NODE_FROM_CURR_TOKEN MakeNewNode(tokens->data.token[*current].type, tokens->data.token[*current].value, NULL, NULL)


Buffer Tokenize (const char * input_file)
{
    assert(input_file);

    Buffer tree_buff = CopyTreeToBuff(input_file);
    Buffer tokens =    AllocateBuffer(kTokenBuff, NUM_OF_TOKENS);

    char * pos = tree_buff.data.sym;

    while (*pos != '\0')
    {
        if (isspace(*pos))
            pos++;
        else if (isdigit(*pos))
            ReadNum(&pos, &tokens);
        else if (*pos == ';')
            ReadDivider(&pos, &tokens);
        else if (IsMathOp(*pos))
            ReadOp(&pos, &tokens);
        else
            ReadWord(&pos, &tokens);
    }

    free(tree_buff.data.sym);

    return tokens;
}


Buffer CopyTreeToBuff (const char * input_file)
{
    assert(input_file);

    int size = GetFileSize(input_file);

    Buffer tree_buff = AllocateBuffer(kCharBuff, size + 1);
    tree_buff.size = size;

    FILE * fp = fopen(input_file, "r");
    assert(fp); // FIXME

    fread(tree_buff.data.sym, sizeof(char), (size_t)tree_buff.size, fp);
    *(tree_buff.data.sym + tree_buff.size) = '\0';

    fclose(fp);

    return tree_buff;
}


int GetFileSize (const char * filename)
{
    assert(filename);

    struct stat st;
    stat(filename, &st);

    return (int)st.st_size;
}


Node * MakeTreeFromTokens (Buffer * tokens, Nametables table)
{
    assert(tokens);

    Node * root = GetG(tokens, table);

    return root;
}


void SkipBrace (Buffer * tokens, int * current)
{
    assert(tokens);
    assert(current);

    Token curr_token = tokens->data.token[*current];

    if (curr_token.type == kOp &&
    (curr_token.value.op == kOpenBracket ||
     curr_token.value.op == kCloseBracket ||
     curr_token.value.op == kOpenCurlyBracket ||
     curr_token.value.op == kCloseCurlyBracket))
        (*current)++;
    else
        {PRINTF_ERROR; printf("CURRENT: %d\n", *current);}
}


Node * GetG (Buffer * tokens, Nametables table)
{
    assert(tokens);

    int current = 0;

    SkipBrace(tokens, &current);

    Node * root = GetString(tokens, &current, table);

    return root;
}


Node * GetString (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = NULL;

    if (IsDivider(tokens, *current))
    {
        node = MAKE_NODE_FROM_CURR_TOKEN;
        (*current)++;
    }
    else if (tokens->data.token[*current].type == kKeyWord && tokens->data.token[*current].value.keyword == kElse)
    {
        node = MAKE_NODE_FROM_CURR_TOKEN;
        (*current)++;

        SkipBrace(tokens, current);
        node->right = GetString(tokens, current, table);
        SkipBrace(tokens, current);

        return node;
    }
    else
        {printf("No divider!\n"); return NULL;}

    SkipBrace(tokens, current);

    switch (tokens->data.token[*current].type)
    {
        case kName:    node->left = GetVarOrFunc(tokens, current, table); break;
        case kKeyWord: node->left = GetKeyword(tokens, current, table);   break;
        case kNum:
        case kOp:      node->left = GetSetPixel(tokens, current, table);  break;
        case kDivider:
        case kComma:
        default:       PRINTF_ERROR;
    }

    SkipBrace(tokens, current);

    if (IsOpenBracket(tokens, *current))
    {
        SkipBrace(tokens, current);
        node->right = GetString(tokens, current, table);
        SkipBrace(tokens, current);
    }

    return node;
}


Node * GetSetPixel (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;
    (*current)++;

    if (node->value.op == kSetPixel)
    {
        SkipBrace(tokens, current);
        node->left = GetP(tokens, current, table);
        SkipBrace(tokens, current);

        SkipBrace(tokens, current);
        node->right = GetP(tokens, current, table);
        SkipBrace(tokens, current);
    }
    else
    {
        PRINTF_ERROR;
        return NULL;
    }

    return node;
}


Node * GetVarOrFunc (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    if (tokens->data.token[*current + 2].type == kOp &&
        tokens->data.token[*current + 2].value.op == kAsn)
        return GetVarEq(tokens, current, table);
    else
        return GetFunc(tokens, current, kNotInit, table);
}


Node * GetVarEq (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    printf("GET VAR EQ CALLED\n");

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;
    node->value.name.type = kVarName;
    int index = FindVarAtNametable(node->value.name.name, table.vars);

    if (index < 0)
    {
        table.vars->data.var[table.vars->size] = {node->value.name.name, table.vars->size};
        node->value.name.index = table.vars->size;
        table.vars->size++;

        if (*(table.is_in_func_now)) // весь if перенесен внутрь if'a
        {
            table.vars->data.var[node->value.name.index].if_local = true;
            table.vars->data.var[node->value.name.index].local_var_index = *(table.local_vars_counter);
            (*(table.local_vars_counter))++;
        }
    }
    else
        node->value.name.index = index;


    (*current)++;

    SkipBrace(tokens, current);
    printf("BRACE SKIPPED curr = %d\n", *current);

    if (IsAsn(tokens, *current))
        node->left = MAKE_NODE_FROM_CURR_TOKEN;
    else
        return NULL;

    (*current)++;

    SkipBrace(tokens, current);
    printf("BRACE SKIPPED curr = %d\n", *current);

    if (tokens->data.token[*current].type == kName)                   // added
        node->left->left = GetFunc(tokens, current, kNotInit, table); // added
    else
        node->left->left = GetE(tokens, current, table);

    SkipBrace(tokens, current);
    printf("BRACE SKIPPED curr = %d\n", *current);
    SkipBrace(tokens, current);
    printf("BRACE SKIPPED curr = %d\n", *current);

    return node;
}


int FindVarAtNametable (char * name, Buffer * vars)
{
    assert(name);
    assert(vars);

    printf("NAME [%s]\n", name);

    for (int count = 0; count < vars->size; count++)
    {
        if (strcmp(name, vars->data.var[count].name) == 0)
            return count;
    }

    return -1;
}


Node * GetE (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    printf("GET E CALLED curr = %d\n", *current);

    while (tokens->data.token[*current].type == kOp &&
           tokens->data.token[*current].value.op < kAsn &&
           tokens->data.token[*current].value.op >= kAdd) // FIXME
    {
        Node * op = MAKE_NODE_FROM_CURR_TOKEN;

        (*current)++;

        SkipBrace(tokens, current);
        op->left = GetE(tokens, current, table);
        SkipBrace(tokens, current);

        SkipBrace(tokens, current);
        op->right = GetE(tokens, current, table);
        SkipBrace(tokens, current);

        return op;
    }

    Node * node = GetP(tokens, current, table);

    return node;
}


Node * GetP (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    printf("GET P CALLED curr = %d\n", *current);

    Node * node = NULL;

    if (tokens->data.token[*current].type == kNum)
    {
        node = MAKE_NODE_FROM_CURR_TOKEN;
        (*current)++;
    }
    else if (tokens->data.token[*current].type == kName &&
             tokens->data.token[*current + 1].type == kOp &&
             tokens->data.token[*current + 1].value.op == kCloseBracket)
    {
        node = MAKE_NODE_FROM_CURR_TOKEN;
        node->value.name.type = kVarName;
        int index = FindVarAtNametable(node->value.name.name, table.vars);

        if (index < 0)
        {
            PRINTF_ERROR;
        }
        else
            node->value.name.index = index;

        (*current)++;
    }
    else if (tokens->data.token[*current].type == kOp &&
             tokens->data.token[*current].value.op > kAsn &&
             tokens->data.token[*current].value.op < kOpenBracket) // FIXME
        node = GetMathFunc(tokens, current, table);
    else
        {PRINTF_ERROR; printf("CURR = %d\n", *current);}

    return node;
}


Node * GetMathFunc (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    if(node->value.op != kSin && node->value.op != kCos &&
       node->value.op != kTan && node->value.op != kCtg &&
       node->value.op != kSqrt && node->value.op != kDif)
        return NULL;

    (*current)++;

    SkipBrace(tokens, current);
    node->left = GetE(tokens, current, table);
    SkipBrace(tokens, current);

    if (node->value.op == kDif)
    {
        SkipBrace(tokens, current);
        node->right = GetP(tokens, current, table);
        SkipBrace(tokens, current);
    }

    return node;
}


bool IsAsn (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kOp
     || tokens->data.token[current].value.op == kAsn)
        return true;

    return false;
}


Node * GetFunc (Buffer * tokens, int * current, bool if_initialization, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;
    node->value.name.type = kFuncName;

    printf("Entered GetFunc. Token enum type is %d\n", tokens->data.token[*current].type);

    int index = FindAtFuncNametable(node->value.name.name, table.funcs);

    printf("index = %d\n", index);

    bool is_initialized = IsFuncInitialized(index, table.funcs);

    if(is_initialized) printf("Init!\n");

    Node * curr_node = node;

    (*current)++;

    SkipBrace(tokens, current);

    int num_of_args = 0;

    if (if_initialization)
    {
        curr_node->left = GetVarArgs(tokens, current, &num_of_args, table);

        curr_node = curr_node->left;

        Node ** arg_nodes = (Node **) calloc ((size_t)num_of_args, sizeof(Node *));

        for (int count = 0; count < num_of_args; count++)
        {
            arg_nodes[count] = curr_node;
            curr_node = curr_node->left;
        }

        if (index < 0)
        {
            index = table.funcs->size;
            table.funcs->data.func[index] = {node->value.name.name, table.funcs->size, num_of_args, true, false, arg_nodes};
            node->value.name.index = index;

            table.funcs->size++;
        }
        else if (index >= 0 && !is_initialized)
        {
            table.funcs->data.func[index] = {node->value.name.name, table.funcs->size, num_of_args, true, false, arg_nodes};
            node->value.name.index = index;

            //table.funcs->size++;
        }
        else
            PRINTF_ERROR;
    }
    else
    {
        curr_node->left = GetVarNumOrFuncArgs(tokens, current, &num_of_args, table);

        if (index >= 0)
        {
            if (!AreArgsCorrect(index, num_of_args, table.funcs))
                PRINTF_ERROR;
        }
        else
        {
            index = table.funcs->size;
            table.funcs->data.func[index] = {node->value.name.name, table.funcs->size, num_of_args, false, false};
            node->value.name.index = index;

            table.funcs->size++;
        }
    }

    SkipBrace(tokens, current);

    if (IsOpenBracket(tokens, *current))
    {
        table.funcs->data.func[index].if_defined = true;

        SkipBrace(tokens, current);

        *(table.is_in_func_now) = true;

        node->right = GetString(tokens, current, table);

        *(table.is_in_func_now) = false;

        SkipBrace(tokens, current);
    }

    return node;
}


int FindAtFuncNametable (char * name, Buffer * funcs)
{
    assert(name);
    assert(funcs);

    for (int count = 0; count < funcs->size; count++)
    {
        if (strcmp(name, funcs->data.func[count].name) == 0)
            return count;
    }

    return -1;
}


bool IsFuncInitialized (int index, Buffer * funcs)
{
    assert(funcs);

    if (index >= funcs->size)
    {
        PRINTF_ERROR;
        return false;
    }
    if (index < 0)
        return false;

    return funcs->data.func[index].if_initialized;
}


bool AreArgsCorrect (int index, int num_of_args, Buffer * funcs)
{
    assert(funcs);

    if (index < 0 || index >= funcs->size)
    {
        PRINTF_ERROR;
        return false;
    }

    if (funcs->data.func[index].num_of_args == num_of_args)
        return true;

    printf("Args are not correct\n Num of args: %d, but supposed to be %d\n", num_of_args, funcs->data.func[index].num_of_args);
    return false;
}


Node * GetVarNumOrFuncArgs (Buffer * tokens, int * current, int * num_of_args, Nametables table)
{
    assert(tokens);
    assert(current);
    assert(num_of_args);

    static int count = 0;

    Node * node = GetE(tokens, current, table); // changed from GetP!

    count++;

    printf("Curr inside getting args: %d\n", *current);

    if (IsOpenBracket(tokens, *current))
    {
        printf("Curr %d is open bracket\n", *current);
        SkipBrace(tokens, current);
        node->left = GetVarNumOrFuncArgs(tokens, current, num_of_args, table);
        SkipBrace(tokens, current);
    }
    else
        *num_of_args = count;

    count = 0;
    return node;
}


Node * GetVarArgs (Buffer * tokens, int * current, int * num_of_args, Nametables table)
{
    assert(tokens);
    assert(current);
    assert(num_of_args);

    Node * node = NULL;

    static int count = 0;

    if (tokens->data.token[*current].type == kName)
    {
        node = MAKE_NODE_FROM_CURR_TOKEN;
        node->value.name.type = kVarName;
        table.vars->data.var[table.vars->size] = {node->value.name.name, table.vars->size, true, *(table.local_vars_counter)}; // changed!
        node->value.name.index = table.vars->size;
        printf("\n\n\nGET VAR ARGS: COUNTER BEFORE: %d\n\n", *(table.local_vars_counter));
        (*(table.local_vars_counter))++;
        printf("\n\n\nGET VAR ARGS: COUNTER AFTER: %d\n\n", *(table.local_vars_counter));
        table.vars->size++;
        (*current)++;
        count++;
    }
    else
        PRINTF_ERROR;

    if (IsOpenBracket(tokens, *current))
    {
        count++;
        SkipBrace(tokens, current);
        node->left = GetVarArgs(tokens, current, num_of_args, table);
        SkipBrace(tokens, current);
    }
    else
        *num_of_args = count;

    return node;
}


bool IsDivider (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kDivider)
        return true;

    return false;
}


Node * GetKeyword (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = NULL;

    switch(tokens->data.token[*current].value.keyword)
    {
        case kIf:
        case kWhile:    node = GetIfOrWhile (tokens, current, table); break;
        case kReturn:   node = GetReturn    (tokens, current, table); break;
        case kIn:       node = GetIn        (tokens, current, table); break;
        case kOut:      node = GetOut       (tokens, current, table); break;
        case kContinue:
        case kBreak:    {node = MAKE_NODE_FROM_CURR_TOKEN; (*current)++; break;}
        case kFunc:     node = GetFuncDefinition(tokens, current, table); break;
        case kDraw:     {node = MAKE_NODE_FROM_CURR_TOKEN; (*current)++; break;}
        case kVarInit:
        case kElse:
        default:        PRINTF_ERROR;
    }

    return node;
}


Node * GetFuncDefinition (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    (*current)++;

    SkipBrace(tokens, current);

    node->left = GetFunc(tokens, current, kInit, table);

    SkipBrace(tokens, current);

    return node;
}


Node * GetIn (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    (*current)++;

    SkipBrace(tokens, current);

    node->left = GetVar(tokens, current, table);

    SkipBrace(tokens, current);

    return node;
}


Node * GetOut (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    printf("ENTERED GET OUT\n");

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    (*current)++;

    SkipBrace(tokens, current);

    node->left = GetE(tokens, current, table);

    SkipBrace(tokens, current);

    return node;
}


Node * GetVar (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    printf("NAME at GetVar: %s\n", tokens->data.token[*current].value.name.name);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;
    node->value.name.type = kVarName;
    int index = FindVarAtNametable(node->value.name.name, table.vars);

    if (index < 0)
    {
        table.vars->data.var[table.vars->size] = {node->value.name.name, table.vars->size};
        node->value.name.index = table.vars->size;
        table.vars->size++;
    }
    else
        node->value.name.index = index;

    (*current)++;

    return node;
}


Node * GetReturn (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    (*current)++;

    SkipBrace(tokens, current);

    node->left = GetE(tokens, current, table);

    SkipBrace(tokens, current);

    return node;
}


Node * GetIfOrWhile (Buffer * tokens, int * current, Nametables table)
{
    assert(tokens);
    assert(current);

    Node * node = MAKE_NODE_FROM_CURR_TOKEN;

    (*current)++; // move to open bracket

    SkipBrace(tokens, current);
    node->left = GetE(tokens, current, table);
    SkipBrace(tokens, current);

    SkipBrace(tokens, current);
    node->right = GetString(tokens, current, table);
    SkipBrace(tokens, current);

    return node;
}


bool IsCloseCurlyBracket (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kOp
     && tokens->data.token[current].value.op == kCloseCurlyBracket)
        return true;

    return false;
}


bool IsOpenCurlyBracket (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kOp
     && tokens->data.token[current].value.op == kOpenCurlyBracket)
        return true;

    return false;
}


bool IsCloseBracket (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kOp
     && tokens->data.token[current].value.op == kCloseBracket)
        return true;

    return false;
}


bool IsOpenBracket (Buffer * tokens, int current)
{
    assert(tokens);

    if (tokens->data.token[current].type == kOp
     && tokens->data.token[current].value.op == kOpenBracket)
        return true;

    return false;
}


void ReadWord (char ** pos, Buffer * tokens)
{
    assert(pos);
    assert(*pos);
    assert(tokens);

    if (**pos == 's' && *(*pos + 1) == 'i' && *(*pos + 2) == 'n')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kSin}};
        (*pos) += 3; // FIXME
        tokens->size++;
    }
    else if (**pos == 'c' && *(*pos + 1) == 'o' && *(*pos + 2) == 's')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kCos}};
        (*pos) += 3;
        tokens->size++;
    }
    else if (**pos == 't' && *(*pos + 1) == 'a' && *(*pos + 2) == 'n')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kTan}};
        (*pos) += 3;
        tokens->size++;
    }
    else if (**pos == 'c' && *(*pos + 1) == 't' && *(*pos + 2) == 'g')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kCtg}};
        (*pos) += 3;
        tokens->size++;
    }
    else if (**pos == 's' && *(*pos + 1) == 'q' && *(*pos + 2) == 'r' && *(*pos + 3) == 't')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kSqrt}};
        (*pos) += 4;
        tokens->size++;
    }
    else if (**pos == 's' && *(*pos + 1) == 'e' && *(*pos + 2) == 't')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kSetPixel}};
        (*pos) += 8;
        tokens->size++;
    }
    else if (**pos == 'd' && *(*pos + 1) == 'i' && *(*pos + 2) == 'f')
    {
        tokens->data.token[tokens->size] = {kOp, {.op = kDif}};
        (*pos) += 3;
        tokens->size++;
    }
    else
        ReadKeyWord(pos, tokens);
}


void ReadKeyWord (char ** pos, Buffer * tokens)
{
    assert(pos);
    assert(*pos);
    assert(tokens);

    char * name = ReadName(pos);

    int count = 0;

    for (count = 0; count < NUM_OF_KEY_WORDS; count++)
    {
        if (strcmp(name, key_words[count].name) == 0)
        {
            tokens->data.token[tokens->size] = {kKeyWord, {.keyword = key_words[count].number}};
            tokens->size++;
            break;
        }
    }
    if (count == NUM_OF_KEY_WORDS)
        ReadVarOrFunc(pos, tokens, name);
    else
        free(name);
}


/*char * ReadName (char ** pos)
{
    assert(pos);
    assert(*pos);

    char * name = (char *) calloc (LENTH_OF_WORD, sizeof(char));
    assert(name); // FIXME check

    char * curr = name;

    while (isalpha(**pos) != 0)
    {
        *curr = **pos;
        (*pos)++;
        curr++;
    }
    *curr = '\0';

    return name;
}*/


char * ReadName (char ** pos)
{
    assert(pos);
    assert(*pos);

    char * name = (char *) calloc (LENTH_OF_WORD, sizeof(char));
    assert(name); // FIXME check

    char * curr = name;

    while ((**pos < 0 && **pos > - ALPH_LENGTH) || **pos == '-' || isalpha(**pos))
    {
        *curr = **pos;
        printf("before ++ [%d][%c]\n", **pos, **pos);
        (*pos)++;
        printf("after ++ [%d][%c]\n", **pos, **pos);
        curr++;
    }
    *curr = '\0';

    printf("NAME: %s (inside readname)\n", name);

    return name;
}


void ReadVarOrFunc (char ** pos, Buffer * tokens, char * name)
{
    assert(pos);
    assert(*pos);
    assert(name);
    assert(tokens);

    printf("NAME: [%s]\n", name);

    tokens->data.token[tokens->size] = {kName, {.name = name}};

    tokens->size++;
}


void ReadOp (char ** pos, Buffer * tokens)
{
    assert(pos);
    assert(*pos);
    assert(tokens);

    char sym = **pos;

    switch(sym)
    {
        case '(':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kOpenBracket}};
                    tokens->size++;
                    break;
                }
        case ')':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kCloseBracket}};
                    tokens->size++;
                    break;
                }
        case '{':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kOpenCurlyBracket}};
                    tokens->size++;
                    break;
                }
        case '}':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kCloseCurlyBracket}};
                    tokens->size++;
                    break;
                }
        case '+':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kAdd}};
                    tokens->size++;
                    break;
                }
        case '-':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kSub}};
                    tokens->size++;
                    break;
                }
        case '*':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kMul}};
                    tokens->size++;
                    break;
                }
        case '\\':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kDiv}};
                    tokens->size++;
                    break;
                }
        case '^':
                {
                    (*pos)++;
                    tokens->data.token[tokens->size] = {kOp, {.op = kPow}};
                    tokens->size++;
                    break;
                }
        case '>':
                {
                    (*pos)++;
                    if (**pos == '=')
                    {
                        (*pos)++;
                        tokens->data.token[tokens->size] = {kOp, {.op = kGreaterThanOrEqual}};
                    }
                    else
                        tokens->data.token[tokens->size] = {kOp, {.op = kGreaterThan}};

                    tokens->size++;
                    break;
                }
        case '<':
                {
                    (*pos)++;
                    if (**pos == '=')
                    {
                        (*pos)++;
                        tokens->data.token[tokens->size] = {kOp, {.op = kLessThanOrEqual}};
                    }
                    else
                        tokens->data.token[tokens->size] = {kOp, {.op = kLessThan}};

                    tokens->size++;
                    break;
                }
        case '!':
                {
                    (*pos)++;
                    if (**pos != '=') {PRINTF_ERROR; return;}
                    (*pos)++;

                    tokens->data.token[tokens->size] = {kOp, {.op = kNotEqual}};
                    tokens->size++;
                    break;
                }
        case '=':
                {
                    (*pos)++;
                    if (**pos == '=')
                    {
                        (*pos)++;
                        tokens->data.token[tokens->size] = {kOp, {.op = kEqual}};
                    }
                    else
                        tokens->data.token[tokens->size] = {kOp, {.op = kAsn}};

                    tokens->size++;
                    break;
                }
        default:    PRINTF_ERROR;
    }
}


bool IsMathOp (char pos)
{
    if (pos == '>' || pos == '<' || pos == '!' || pos == '=' ||
        pos == '(' || pos == ')' || pos == '{' || pos == '}' ||
        pos == '+' || pos == '-' || pos == '*' || pos == '\\'||
        pos == '^')
        return true;

    return false;
}


void ReadDivider (char ** pos, Buffer * tokens)
{
    assert(pos);
    assert(*pos);
    assert(tokens);

    (*pos)++;
    tokens->data.token[tokens->size] = {kDivider, {}};
    tokens->size++;
}


void ReadNum (char ** pos, Buffer * tokens)
{
    assert(pos);
    assert(*pos);
    assert(tokens);

    int val = 0;
    while ('0' <= **pos && **pos <= '9')
    {
        val = **pos - '0' + val * 10;
        (*pos)++;
    }

    tokens->data.token[tokens->size] = {kNum, {.num = val}};
    tokens->size++;
}


Buffer AllocateBuffer (TypeOfBuffer type, int size)
{
    Buffer buffer = {};
    buffer.size = 0;

    switch(type)
    {
        case kNodeBuff:
                        if (!(buffer.data.node = AllocateNodeBuff(size)))
                            PRINTF_ERROR;
                        break;

        case kCharBuff:
                        if (!(buffer.data.sym  = AllocateCharBuff(size)))
                            PRINTF_ERROR;
                        break;

        case kTokenBuff:
                        if (!(buffer.data.token  = AllocateTokenBuff(size)))
                            PRINTF_ERROR;
                        break;

        case kFuncBuff:
                        if (!(buffer.data.func  = AllocateFuncBuff(size)))
                            PRINTF_ERROR;
                        break;

        case kVarBuff:
                        if (!(buffer.data.var  = AllocateVarBuff(size)))
                            PRINTF_ERROR;
                        break;

        default:            PRINTF_ERROR;
    }

    return buffer;
}


Node ** AllocateNodeBuff (int size)
{
    Node ** buffer = (Node **) calloc ((size_t)size, sizeof(Node *));
    if (!buffer) PRINTF_ERROR;

    return buffer;
}


char * AllocateCharBuff (int size)
{
    char * buffer = (char *) calloc ((size_t)size, sizeof(char));
    if (!buffer) PRINTF_ERROR;

    return buffer;
}


Token * AllocateTokenBuff (int size)
{
    Token * buffer = (Token *) calloc ((size_t)size, sizeof(Token));
    if (!buffer) PRINTF_ERROR;

    return buffer;
}


Function * AllocateFuncBuff (int size)
{
    Function * buffer = (Function *) calloc ((size_t)size, sizeof(Function));
    if (!buffer) PRINTF_ERROR;

    return buffer;
}


Variable * AllocateVarBuff (int size)
{
    Variable * buffer = (Variable *) calloc ((size_t)size, sizeof(Variable));
    if (!buffer) PRINTF_ERROR;

    return buffer;
}


void PutTreeToFile (const char * filename, Node * root)
{
    assert(root);

    FILE * fp = fopen(filename, "w");
    if (!fp) PRINTF_ERROR;

    PrintNode(fp, root);

    fclose(fp);
}


void PrintNode (FILE * fp, Node * node)
{
    assert(fp);
    assert(node);

    fprintf(fp, "(");
    PrintNodeValue(fp, node);

    if (node->left)
        PrintNode(fp, node->left);
    if (node->right)
        PrintNode(fp, node->right);

    fprintf(fp, ")");
}


void PrintNodeValue (FILE * fp, Node * node)
{
    assert(fp);
    assert(node);

    switch(node->type)
    {
        case kNum: fprintf(fp, "%d", node->value.num);             break;
        case kOp:
        {
            switch(node->value.op)
            {
                case kAdd:                fprintf(fp, "+");        break;
                case kSub:                fprintf(fp, "-");        break;
                case kMul:                fprintf(fp, "*");        break;
                case kDiv:                fprintf(fp, "\\");       break;
                case kGreaterThan:        fprintf(fp, ">");        break;
                case kLessThan:           fprintf(fp, "<");        break;
                case kGreaterThanOrEqual: fprintf(fp, ">=");       break;
                case kLessThanOrEqual:    fprintf(fp, "<=");       break;
                case kEqual:              fprintf(fp, "==");       break;
                case kNotEqual:           fprintf(fp, "!=");       break;
                case kAsn:                fprintf(fp, "=");        break;
                case kSin:                fprintf(fp, "sin");      break;
                case kCos:                fprintf(fp, "cos");      break;
                case kTan:                fprintf(fp, "tan");      break;
                case kCtg:                fprintf(fp, "ctg");      break;
                case kPow:                fprintf(fp, "pow");      break;
                case kSqrt:               fprintf(fp, "sqrt");     break;
                case kSetPixel:           fprintf(fp, "setpixel"); break;
                case kLn:                 fprintf(fp, "ln");       break;
                case kDif:                fprintf(fp, "dif");      break;
                case kOpenBracket:
                case kCloseBracket:
                case kOpenCurlyBracket:
                case kCloseCurlyBracket:
                default:                  PRINTF_ERROR;
            }
            break;
        }
        case kKeyWord:
        {
            switch(node->value.keyword)
            {
                case kIf:                 fprintf(fp, "if");       break;
                case kWhile:              fprintf(fp, "while");    break;
                case kContinue:           fprintf(fp, "continue"); break;
                case kBreak:              fprintf(fp, "break");    break;
                case kReturn:             fprintf(fp, "return");   break;
                case kIn:                 fprintf(fp, "in");       break;
                case kOut:                fprintf(fp, "out");      break;
                case kFunc:               fprintf(fp, "func");     break;
                case kElse:               fprintf(fp, "else");     break;
                case kDraw:               fprintf(fp, "draw");     break;
                case kVarInit:
                default:                  PRINTF_ERROR;
            }
            break;
        }
        case kName:    fprintf(fp, "%s", node->value.name.name);   break;
        case kDivider: fprintf(fp, ";");                           break;
        case kComma:
        default:       PRINTF_ERROR;
    }
}


Node * MakeNewNode (Type_t type, Value_t value, Node * left, Node * right)
{
    Node * node = AllocateNodeMemory();
    assert(node); // FIXME

    node->value = value;
    node->type  = type;

    node->left  = left;
    node->right = right;

    return node;
}


Node * AllocateNodeMemory (void)
{
    Node * node = (Node *) calloc (1, sizeof(Node));

    return node;
}


void PrintfTokens (Buffer * tokens)
{
    assert(tokens);

    for (int count = 0; count < tokens->size; count++)
    {
        printf("\n[%d] ", count);
        PrintTokenValue(tokens->data.token[count]);
    }
}


void PrintTokenValue (Token token)
{
    switch(token.type)
    {
        case kNum: printf("%d", token.value.num);             break;
        case kOp:
        {
            switch(token.value.op)
            {
                case kAdd:                printf("+");        break;
                case kSub:                printf("-");        break;
                case kMul:                printf("*");        break;
                case kDiv:                printf("\\");       break;
                case kGreaterThan:        printf(">");        break;
                case kLessThan:           printf("<");        break;
                case kGreaterThanOrEqual: printf(">=");       break;
                case kLessThanOrEqual:    printf("<=");       break;
                case kEqual:              printf("==");       break;
                case kNotEqual:           printf("!=");       break;
                case kAsn:                printf("=");        break;
                case kSin:                printf("sin");      break;
                case kCos:                printf("cos");      break;
                case kTan:                printf("tan");      break;
                case kCtg:                printf("ctg");      break;
                case kPow:                printf("^");        break;
                case kSqrt:               printf("sqrt");     break;
                case kOpenBracket:        printf("(");        break;
                case kCloseBracket:       printf(")");        break;
                case kOpenCurlyBracket:   printf("{");        break;
                case kCloseCurlyBracket:  printf("}");        break;
                case kSetPixel:           printf("setpixel"); break;
                case kDif:                printf("dif");      break;
                case kLn:                 printf("ln");       break;
                default:                  PRINTF_ERROR;
            }
            break;
        }
        case kKeyWord:
        {
            switch(token.value.keyword)
            {
                case kIf:                 printf("if");       break;
                case kWhile:              printf("while");    break;
                case kContinue:           printf("continue"); break;
                case kBreak:              printf("break");    break;
                case kReturn:             printf("return");   break;
                case kIn:                 printf("in");       break;
                case kOut:                printf("out");      break;
                case kFunc:               printf("func");     break;
                case kElse:               printf("else");     break;
                case kDraw:               printf("draw");     break;
                case kVarInit:
                default:                  PRINTF_ERROR;
            }
            break;
        }
        case kName:    printf("%s", token.value.name.name);   break;
        case kDivider: printf(";");                           break;
        case kComma:   printf(",");                           break;
        default:       PRINTF_ERROR;
    }
}


void NodeDtor (Node * node)
{
    assert(node);

    if (node->left)
        NodeDtor(node->left);
    if (node->right)
        NodeDtor(node->right);

    if (node->type == kName)
        free(node->value.name.name);

    free(node);
}
