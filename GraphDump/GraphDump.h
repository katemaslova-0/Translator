#ifndef DUMP_H
#define DUMP_H

/*Dump functions*/
void MakeGraphCodeFile      (Node * node);
void OutputNode             (FILE * fp, Node * node);
const char * GetNodeType    (Node * node);
char * GetCommand           (char * pic_name);
char * GetPicName           (void);
const char * GetNodeOp      (Node * node);
const char * GetNodeKeyword (Node * node);

#endif // DUMP_H
