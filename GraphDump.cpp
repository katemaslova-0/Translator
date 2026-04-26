#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "GraphDump.h"

const int SIZE_OF_COMMAND = 100;
const int SIZE_OF_PIC_NAME = 50;

#define light_green "\"#C0FFC0\""
#define dark_green  "\"#008000\""
#define light_pink  "\"#ffc0f6ff\""
#define dark_pink   "\"#fa5d82ff\""
#define light_red   "\"#ff0000ff\""
#define dark_red    "\"#bd2222ff\""
#define white       "\"#ffffffff\""

void MakeGraphCodeFile (Node * node)
{
    FILE * fp = fopen("dump.txt", "w");
    assert(fp);

    fprintf(fp, "digraph G{\nrankdir=HR;\n");

    if (node != NULL)
        OutputNode(fp, node);

    fprintf(fp, "}");

    fclose(fp);

    char * pic_name = GetPicName();
    assert(pic_name);

    char * command  = GetCommand(pic_name);
    assert(command);

    system(command);

    free(command);
}


void OutputNode (FILE * fp, Node * node)
{
    assert(fp);
    assert(node);

    fprintf(fp, "f%p [shape=Mrecord; style = filled; fillcolor = "light_pink"; ", node);
    fprintf(fp, "color = "dark_green"; label = \"{address = %p ", node);
    fprintf(fp, "| type = %s ", GetNodeType(node));

    switch(node->type)
    {
        case kNum:     fprintf(fp, "| value: %d ", (node->value).num);       break;
        case kOp:      fprintf(fp, "| value: %s ", GetNodeOp(node));         break;
        case kKeyWord: fprintf(fp, "| value: %s ", GetNodeKeyword(node));    break;
        case kDivider: fprintf(fp, "| DIVIDER");                             break;
        case kName:    fprintf(fp, "| value = %s ", node->value.name.name ); break;
        case kComma:   fprintf(fp, "| COMMA");                               break;
        default:       fprintf(fp, "| NODE TYPE ERROR ");                    break;
    }

    fprintf(fp, "| {left = %p | right = %p}} \"]; \n", node->left, node->right);

    if (node->left)
    {
        OutputNode(fp, node->left);
        fprintf(fp, "f%p -> f%p [color = "dark_pink"];\n", node, node->left);
    }
    if (node->right)
    {
        OutputNode(fp, node->right);
        fprintf(fp, "f%p -> f%p [color = "dark_pink"];\n", node, node->right);
    }
}


const char * GetNodeKeyword (Node * node)
{
    assert(node);

    switch(node->value.keyword)
    {
        case kIf:       return "IF";
        case kWhile:    return "WHILE";
        case kContinue: return "CONTINUE";
        case kBreak:    return "BREAK";
        case kReturn:   return "RETURN";
        case kIn:       return "IN";
        case kOut:      return "OUT";
        case kFunc:     return "FUNC";
        case kVarInit:  return "VAR INIT (ERROR)";
        case kElse:     return "ELSE";
        case kDraw:     return "DRAW";
        default:        return NULL;
    }
}


const char * GetNodeOp (Node * node)
{
    assert(node);

    switch(node->value.op)
    {
        case kAdd:                return "+";
        case kSub:                return "-";
        case kMul:                return "*";
        case kDiv:                return "/";
        case kGreaterThan:        return "GREATER THEN";
        case kLessThan:           return "LESS THEN" ;
        case kGreaterThanOrEqual: return "GREATER THEN OR EQUAL";
        case kLessThanOrEqual:    return "LESS THEN OR EQUAL";
        case kEqual:              return "==";
        case kNotEqual:           return "!=";
        case kPow:                return "POW";
        case kAsn:                return "=";
        case kSin:                return "SIN";
        case kCos:                return "COS";
        case kTan:                return "TAN";
        case kCtg:                return "CTG";
        case kSqrt:               return "SQRT";
        case kOpenBracket:        return "( (error)";
        case kCloseBracket:       return ") (error)";
        case kOpenCurlyBracket:   return "{ (error)";
        case kCloseCurlyBracket:  return "} (error)";
        case kSetPixel:           return "SETPIXEL";
        case kDif:                return "DIF";
        case kLn:                 return "LN";
        default:                  return  NULL;
    }
}


const char * GetNodeType (Node * node)
{
    assert(node);

    switch(node->type)
    {
        case kNum:      return "NUM";     break;
        case kName:     return "NAME";    break;
        case kOp:       return "OP";      break;
        case kKeyWord:  return "KEYWORD"; break;
        case kDivider:  return "DIVIDER"; break;
        case kComma:    return "COMMA";   break;
        default:        return  NULL;
    }
}


char * GetCommand (char * pic_name)
{
    assert(pic_name);

    char * command = (char *) calloc (SIZE_OF_COMMAND, sizeof(char));
    if(!command) return NULL;

    sprintf(command, "dot dump.txt -T png -o %s", pic_name);
    free(pic_name);

    return command;
}


char * GetPicName (void)
{
    static int pic_count = 0;

    char * pic_name = (char *) calloc (SIZE_OF_PIC_NAME, sizeof(char));
    if(!pic_name) return NULL;

    sprintf(pic_name, "dump%d.png", pic_count);
    pic_count++;

    return pic_name;
}
