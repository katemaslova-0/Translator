#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

#include "asm.h"

#define SIZE_OF_TABLE 997

const int NUM_OF_NAMES = 20;
const int NUM_OF_LABELS = 20;
const int NUM_OF_ACCESSES = 20;
const int LENGTH_OF_FUNC_NAME = 20;
const int BYTE_ARRAY_SIZE = 8000;

unsigned char elf_header[] =          {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x02, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00,
                                       0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char program_header_text[] = {0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // 0x05 -> 0x07
                                       0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xB0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char program_header_data[] = {0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // 0x06 -> 0x07
                                       0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xC9, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0xC9, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

HashElem_t hash_source[] = {{0, "mov", TranslateMov},
                            {0, "lea", TranslateLea},
                            {0, "cal", TranslateCall},
                            {0, "pus", TranslatePush},
                            {0, "pop", TranslatePop},
                            {0, "xor", TranslateXor},
                            {0, "sys", TranslateSyscall},
                            {0, "add", TranslateMath},
                            {0, "sub", TranslateMath},
                            {0, "div", TranslateMath},
                            {0, "mul", TranslateMath},
                            {0, "xch", TranslateXcng},
                            {0, "cmp", TranslateCmp},
                            {0, "jmp", TranslateJmp},
                            {0, "je ", TranslateJmp},
                            {0, "jne", TranslateJmp},
                            {0, "jl ", TranslateJmp},
                            {0, "jle", TranslateJmp},
                            {0, "jg ", TranslateJmp},
                            {0, "jge", TranslateJmp},
                            {0, "cvt", TranslateCvt},
                            {0, "sqr", TranslateSqr},
                            {0, "ret", TranslateRet}};

int num_of_hashsource_elems = sizeof(hash_source) / sizeof(HashElem_t);

HashElem_t hashtable[SIZE_OF_TABLE] = {};

void FwriteAll (FILE * fp_out, Asm_t * asm_struct)
{
    assert(fp_out);
    assert(asm_struct);

    fwrite(&elf_header,          1, sizeof(elf_header),          fp_out);
    fwrite(&program_header_text, 1, sizeof(program_header_text), fp_out);
    fwrite(&program_header_data, 1, sizeof(program_header_data), fp_out);
    fwrite(asm_struct->bytes,    1, (size_t) asm_struct->byte_counter,    fp_out);
}


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


void DestroyAll (Asm_t * asm_struct)
{
    for (int count = 0; count < NUM_OF_NAMES; count++)
    {
        free(asm_struct->names[count].byte_pointers);
        free(asm_struct->names[count].name);
    }

    for (int count = 0; count < NUM_OF_LABELS; count++)
        free(asm_struct->labels[count].byte_pointers);

    free(asm_struct->names);
    free(asm_struct->labels);

    free(asm_struct->buffer);
    free(asm_struct->bytes);
    free(asm_struct->lines);
}


int FillHashtable (void)
{
    int * hash_array = (int *) calloc ((size_t) num_of_hashsource_elems, sizeof(int));

    for (int count = 0; count < num_of_hashsource_elems; count++)
    {
        int hash = (int) Crc32(hash_source[count].string);
        hashtable[hash] = {hash, hash_source[count].string, hash_source[count].func};
        hash_array[count] = hash;
    }

    free(hash_array);
    return 0;
}


int CheckCollisions (Asm_t * asm_struct)
{
    assert(asm_struct);

    int size = num_of_hashsource_elems + asm_struct->name_counter;

    int * hash_array = (int *) calloc ((size_t) size, sizeof(int));

    for (int count = 0; count < num_of_hashsource_elems; count++)
    {
        int hash = (int) Crc32(hash_source[count].string);
        hash_array[count] = hash;
    }

    int name_counter = asm_struct->name_counter;

    for (int count = 0; count < name_counter; count++)
    {
        int hash = (int) Crc32(asm_struct->names[count].name);
        hash_array[num_of_hashsource_elems + count] = hash;
    }

    for (int j = 0; j < size; j++)
    {
        for (int i = j + 1; i < size; i++)
        {
            if (hash_array[j] == hash_array[i])
            {
                printf("Collision found!\nhash: %d, j = %d, i = %d\n", hash_array[j], j, i);
                free(hash_array);
                return -1;
            }
        }
    }

    free(hash_array);
    return 0;
}


int CopyAsmFileToBuffer (Asm_t * asm_struct, const char * input_file)
{
    assert(asm_struct);
    assert(input_file);

    FILE * fp_in  = fopen(input_file, "r");
    if (!fp_in) return -1;

    int filesize = GetFileSize(input_file);

    asm_struct->size_of_buffer = filesize;
    asm_struct->buffer = (char *) calloc ((size_t)filesize + 1, sizeof(char));
    if (!asm_struct->buffer) return -1;

    fread(asm_struct->buffer, sizeof(char), (size_t)filesize, fp_in);
    fclose(fp_in);

    return 0;
}


int FillNametable (Asm_t * asm_struct)
{
    assert(asm_struct);

    char ** ptr = asm_struct->lines;
    int num_of_lines = asm_struct->num_of_lines;

    int count = 0;
    for (; count < num_of_lines; count++, ptr++)
    {
        if (**ptr == '\0')
            continue;

        if (**ptr == '.')
            FillLabel(asm_struct, *ptr);
        else if (strncmp(*ptr, "call", 4) == 0)
            FillFuncCall(asm_struct, *ptr);
        else if (strncmp(*ptr, "section .data", 13) == 0)
            break;
    }

    FillVars(asm_struct, count, ptr);

    #ifdef NDEBUG
        int num_of_names = asm_struct->name_counter;
        int num_of_labels = asm_struct->label_counter;

        printf("Names:\n");
        for (int i = 0; i < num_of_names; i++)
            printf("%s\n", asm_struct->names[i].name);

        printf("Labels:\n");
        for (int i = 0; i < num_of_labels; i++)
            printf(".L%c\n", asm_struct->labels[i].number);

    #endif

    return 0;
}


void FillVars (Asm_t * asm_struct, int count, char ** ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr++;
    count++; // move from "section .data"

    int num_of_lines = asm_struct->num_of_lines;

    for (; count < num_of_lines; count++, ptr++)
    {
        if (**ptr == '\0')
            continue;

        char * var_name = (char *) calloc ((size_t) LENGTH_OF_FUNC_NAME, sizeof(char));

        char * curr_line = *ptr;

        for (int name_count = 0; *(curr_line + name_count) != ':'; name_count++)
            var_name[name_count] = *(curr_line + name_count);

        asm_struct->names[asm_struct->name_counter].name = var_name;
        asm_struct->names[asm_struct->name_counter].type = VAR;
        asm_struct->name_counter++;
    }
}


void FillFuncCall (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 5; // move to func name

    char * name = (char *) calloc ((size_t) LENGTH_OF_FUNC_NAME, sizeof(char));
    sscanf(ptr, "%s", name);

    int name_counter = asm_struct->name_counter;

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(asm_struct->names[count].name, ptr) == 0)
        {
            free(name);
            return;
        }
    }

    asm_struct->names[name_counter].name = name;
    asm_struct->names[name_counter].type = FUNC;
    asm_struct->name_counter++;
}


void FillLabel (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 2; // move to label number

    sscanf(ptr, "%c", &asm_struct->labels[asm_struct->label_counter].number);
    asm_struct->label_counter++;
}


int FirstCompilation (Asm_t * asm_struct)
{
    assert(asm_struct);

    char ** ptr = asm_struct->lines;
    int count = 0;
    int num_of_lines = asm_struct->num_of_lines;

    while (strcmp(*ptr, "main:") != 0)
    {
        ptr++;
        count++;
    }
    ptr++;
    count++;

    char first_three[4] = {};

    for (; count < num_of_lines; count++, ptr++)
    {
        printf("TRANSLATING LINE: %s\n", *ptr);
        if (**ptr == '\0')
            continue;

        if (strncmp(*ptr, "section .data", 13) == 0)
            break;

        memcpy(first_three, *ptr, 3);
        unsigned int hash = Crc32(first_three);

        if (**ptr == '.')
        {
            int res = 0;
            if ((res = AddLabelOffset(asm_struct, *ptr)) != 0)
                return -1;
        }
        else if (hashtable[hash].func)
        {
            int res = 0;
            if ((res = (*hashtable[hash].func)(asm_struct, *ptr)) != 0)
                return -1;
        }
        else
        {
            printf("Add function offset called for line: \"%s\"\n", *ptr);

            int res = 0;
            if ((res = AddFunctionOffset(asm_struct, *ptr)) != 0)
                return -1;
        }
    }

    asm_struct->start_of_data_segment = asm_struct->byte_counter;

    // добавление оттранслированного кода в буфер
    // изменение byte_counter c учётом этого! иначе некорректная AddVarsOffsets; start_of_data_segment - инициализировать
    int res = 0;

    if ((res = AddVarsOffsets(asm_struct)) != 0)
        return -1;

    return 0;
}


int SecondCompilation (Asm_t * asm_struct)
{
    assert(asm_struct);

    int label_counter = asm_struct->label_counter;
    int name_counter  = asm_struct->name_counter;

    for (int count = 0; count < label_counter; count++)
    {
        int byte_ptr_array_size = asm_struct->labels[count].size_of_byte_pointers_array;
        Label_t curr_label = asm_struct->labels[count];

        printf("\nLabel: number: %c offset: %d ", curr_label.number, curr_label.offset);
        for (int one_label_cnt = 0; one_label_cnt < byte_ptr_array_size; one_label_cnt++)
        {
            int offset = curr_label.offset - curr_label.byte_pointers[one_label_cnt];
            printf(" RESULT OFFSET: %d ", offset);
            WriteOffsetToByteArray(asm_struct, offset, curr_label.byte_pointers[one_label_cnt]);

            printf("byteptr: %d ", curr_label.byte_pointers[one_label_cnt]);
        }
    }

    for (int count = 0; count < name_counter; count++)
    {
        int byte_ptr_array_size = asm_struct->names[count].size_of_byte_pointers_array;
        Name_t curr_name = asm_struct->names[count];

        printf("\nName: name: %s offset: %d ", curr_name.name, curr_name.offset);
        for (int one_name_cnt = 0; one_name_cnt < byte_ptr_array_size; one_name_cnt++)
        {
            int offset = 0;

            if (curr_name.type == VAR)
                offset = curr_name.offset - 4 - curr_name.byte_pointers[one_name_cnt];
            else
                offset = curr_name.offset - 4 - curr_name.byte_pointers[one_name_cnt];

            printf(" RESULT OFFSET: %d ", offset);
            WriteOffsetToByteArray(asm_struct, offset, curr_name.byte_pointers[one_name_cnt]);

            printf("byteptr: %d ", curr_name.byte_pointers[one_name_cnt]);
        }
    }

    return 0;
}


void WriteOffsetToByteArray (Asm_t * asm_struct, int offset, int byte_pointer)
{
    assert(asm_struct);

    asm_struct->bytes[byte_pointer] =     (unsigned char)  offset & 0xFF;
    asm_struct->bytes[byte_pointer + 1] = (unsigned char) (offset >> 8)  & 0xFF;
    asm_struct->bytes[byte_pointer + 2] = (unsigned char) (offset >> 16) & 0xFF;
    asm_struct->bytes[byte_pointer + 3] = (unsigned char) (offset >> 24) & 0xFF;
}


int AddVarsOffsets (Asm_t * asm_struct)
{
    assert(asm_struct);

    char ** ptr = asm_struct->lines;
    int count = 0;

    while (strncmp(*ptr, "section .data", 13) != 0)
    {
        ptr++;
        count++;
    }
    ptr++;
    count++;

   int name_quantity = asm_struct->name_counter;
   int num_of_lines = asm_struct->num_of_lines;

    for (; count < num_of_lines; count++, ptr++)
    {
        if (**ptr == '\0')
            continue;

        else if (**ptr == ';')
            continue;

        char name[20] = {}; // FIXME

        for (int i = 0; *(*ptr + i) != ':'; i++)
            name[i] = *(*ptr + i);

        int name_count = 0;
        for (; name_count < name_quantity; name_count++)
        {
            if (strcmp(name, asm_struct->names[name_count].name) == 0)
            {
                asm_struct->names[name_count].offset = asm_struct->byte_counter;
                asm_struct->bytes[asm_struct->byte_counter] = 0x00; // пока только 0
                asm_struct->bytes[asm_struct->byte_counter + 1] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 2] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 3] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 4] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 5] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 6] = 0x00;
                asm_struct->bytes[asm_struct->byte_counter + 7] = 0x00;
                asm_struct->byte_counter += 8;
                break;
            }
        }

        if (name_count == name_quantity)
        {
            printf("Error in AddVarOffsets\n");
            return -1;
        }
    }

    return 0;
}


int AddFunctionOffset (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int name_counter = asm_struct->name_counter;

    char name[20] = {}; // FIXME

    for (int i = 0; *ptr != ':'; i++, ptr++)
        name[i] = *ptr;

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(name, asm_struct->names[count].name) == 0)
        {
            asm_struct->names[count].offset = asm_struct->byte_counter;
            return 0;
        }
    }

    printf("Error: func name's not found\n");
    return -1;
}


int AddLabelOffset (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 2; // move to label number

    int label_counter = asm_struct->label_counter;

    for (int count = 0; count < label_counter; count++)
    {
        if (asm_struct->labels[count].number == *ptr)
        {
            asm_struct->labels[count].offset = asm_struct->byte_counter;
            return 0;
        }
    }

    printf("Error: label's not found\n");
    return -1;
}


unsigned int Crc32 (const char * word)
{
    unsigned int res = 0;
    unsigned int len = (unsigned int) strlen (word);

    while (len--)
    {
        res ^= (unsigned int) *word;
        word++;
        for (int i = 0; i < 8; i++)
            res = (res >> 1) ^ ((res & 1) ? 0xedb88320 : 0);
    }

    return res % SIZE_OF_TABLE;
}


int FillPointerBuff (Asm_t * asm_struct)
{
    assert(asm_struct);

    int num_of_lines = CountLines(asm_struct->buffer);
    asm_struct->num_of_lines = num_of_lines;

    ReplaceSymbolsInBuffer(asm_struct->buffer, '\n', '\0', asm_struct->size_of_buffer);

    asm_struct->lines = (char **) calloc((size_t)(num_of_lines), sizeof(char *));
    if (!asm_struct->lines) return -1;

    char * ptr_buffer = asm_struct->buffer;
    (asm_struct->lines)[0] = asm_struct->buffer;

    for (int count = 1; count < num_of_lines; count++)
    {
        while (*ptr_buffer != '\0')
            ptr_buffer++;
        ptr_buffer++;

        (asm_struct->lines)[count] = ptr_buffer;
    }

    return 0;
}


int CountLines (char * buffer)
{
    assert(buffer);

    int num_of_lines = 0;
    int count = 0;
    while(*(buffer + count) != '\0')
    {
        if (*(buffer + count) == '\n')
        {
            num_of_lines++;
        }
        count++;
    }

    return num_of_lines;
}


int GetFileSize (const char * filename)
{
    assert(filename);

    struct stat st;
    stat(filename, &st);

    return (int)st.st_size;
}


void ReplaceSymbolsInBuffer (char * buffer, char sym_to_find, char sym_to_put_instead, int size_of_buffer)
{
    assert(buffer);

    int count = 0;
    for (count = 0; count < size_of_buffer - 1; ++count)
    {
        if (*(buffer + count) == sym_to_find)
        {
            *(buffer + count) = sym_to_put_instead;
        }
    }

    *(buffer + count) = '\0';
}


int TranslateRet (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    asm_struct->bytes[asm_struct->byte_counter] = 0xC3;
    asm_struct->byte_counter++;

    return 0;
}


int TranslateSqr (Asm_t * asm_struct, char * ptr) // for now only for sqrtsd xmm0, xmm0!
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xF2;
    asm_struct->bytes[count + 1] = 0x0F;
    asm_struct->bytes[count + 2] = 0x51;
    asm_struct->bytes[count + 3] = 0xC0;

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateCvt (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int res = 0;

    if (*(ptr + 4) == 'i')
    {
        if ((res = TranslateCvtTypeSpecified(CVTSI2SD, asm_struct, ptr)) != 0)
            return -1;
    }
    else if (*(ptr + 4) == 'd')
    {
        if ((res = TranslateCvtTypeSpecified(CVTSD2SI, asm_struct, ptr)) != 0)
            return -1;
    }
    else
    {
        printf("Error in TranslateCvt\n");
        return -1;
    }

    return 0;
}


int TranslateCvtTypeSpecified (Cvt_t type, Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 9;

    int count = asm_struct->byte_counter;
    asm_struct->bytes[count] = 0xF2;
    asm_struct->bytes[count + 1] = 0x48;
    asm_struct->bytes[count + 2] = 0x0F;

    Op_t dst = DEFAULT;
    Op_t src = DEFAULT;

    if (type == CVTSI2SD)
    {
        asm_struct->bytes[count + 3] = 0x2A;
        dst = XMM0; // for now only for XMM0!
        src = ParseRegName(ptr);
    }
    else if (type == CVTSD2SI)
    {
        asm_struct->bytes[count + 3] = 0x2D;
        dst = ParseRegName(ptr + 6);
        src = XMM0;
    }
    else
    {
        printf("Error in TranslateCvtTypeSpecified\n");
        return -1;
    }

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 4] = mod_rm;

    asm_struct->byte_counter += 5;
    return 0;
}


int TranslateJmp (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    Jmp_t jump = ParseJmp(ptr);

    if (jump == DEFAULT_JMP)
        return -1;

    if (jump == JMP)
    {
        asm_struct->bytes[count] = 0xE9;
        AddLabelPointer(asm_struct, ptr, asm_struct->byte_counter + 1);
        asm_struct->byte_counter += 5; // 4 bytes for offset are null for now
    }
    else
    {
        asm_struct->bytes[count] = 0x0F;

        switch(jump)
        {
            case JL:  asm_struct->bytes[count + 1] = 0x8C; break;
            case JLE: asm_struct->bytes[count + 1] = 0x8E; break;
            case JG:  asm_struct->bytes[count + 1] = 0x8F; break;
            case JGE: asm_struct->bytes[count + 1] = 0x8D; break;
            case JE:  asm_struct->bytes[count + 1] = 0x84; break;
            case JNE: asm_struct->bytes[count + 1] = 0x85; break;
            case JMP:
            case DEFAULT_JMP:
            default: {printf("Error in TranslateJmp\n"); return -1;}
        }
        AddLabelPointer(asm_struct, ptr, asm_struct->byte_counter + 2);
        asm_struct->byte_counter += 6; // 4 bytes for offset are null for now
    }

    return 0;
}


void AddLabelPointer (Asm_t * asm_struct, char * ptr, int label_offset)
{
    assert(asm_struct);
    assert(ptr);

    while (*ptr != 'L')
        ptr++;
    ptr++; // move to label number

    int label_counter = asm_struct->label_counter;

    for (int count = 0; count < label_counter; count++)
    {
        if (*ptr == asm_struct->labels[count].number)
        {
            asm_struct->labels[count].byte_pointers[asm_struct->labels[count].size_of_byte_pointers_array] = label_offset;
            asm_struct->labels[count].size_of_byte_pointers_array++;
            return;
        }
    }

    printf("Error: label not found\n");
}



Jmp_t ParseJmp (char * ptr)
{
    switch(*(ptr + 1))
    {
        case 'm': return JMP;
        case 'l': if (*(ptr + 2) == 'e')
                    return JLE;
                  else
                    return JL;
        case 'g': if (*(ptr + 2) == 'e')
                    return JGE;
                  else
                    return JG;
        case 'e': return JE;
        case 'n': return JNE;
        default: printf("Error in ParseJmp\n"); return DEFAULT_JMP;
    }
}


int TranslateMath (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    switch(*ptr)
    {
        case 'a': TranslateAddOrSub(ADD, asm_struct, ptr); break;
        case 's': TranslateAddOrSub(SUB, asm_struct, ptr); break;
        case 'm': TranslateMulOrDiv(MUL, asm_struct, ptr); break;
        case 'd': TranslateMulOrDiv(DIV, asm_struct, ptr); break;
        default: printf("Error in TranslateMath\n");
    }

    return 0;
}


int TranslateCmp (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // move to 'r'

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;
    asm_struct->bytes[count + 1] = 0x39;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 5);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateXcng (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 5; // move to 'r'

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;
    asm_struct->bytes[count + 1] = 0x87;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t reg1 = ParseRegName(ptr);
    Op_t reg2 = ParseRegName(ptr + 5);

    unsigned char mod_rm = MakeModeRm(mod, reg1, reg2, SrcFirst); // the last arg doesn't matter
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


void TranslateMulOrDiv (MathOp_t type, Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;
    asm_struct->bytes[count + 1] = 0xF7;

    unsigned char mod_rm = 0;

    if (type == MUL)
        mod_rm = 0xE0;
    else if (type == DIV)
        mod_rm = 0xF0;
    else
        printf("Error in TranslateMulOrDiv(invalid math op)\n");

    ptr += 4; // move to 'r'
    Op_t reg = ParseRegName(ptr);

    switch(reg)
    {
        case RAX:              break;
        case RCX: mod_rm++;    break;
        case RDX: mod_rm += 2; break;
        case RBX: mod_rm += 3; break;
        case RSP: mod_rm += 4; break;
        case RBP: mod_rm += 5; break;
        case RSI: mod_rm += 6; break;
        case RDI: mod_rm += 7; break;
        case XMM0:
        case NUM_MOV:
        case NUM_ADD:
        case NUM_SUB:
        case LABEL:
        case DEFAULT:
        default: printf("Error in TranslateMulOrDiv\n");
    }

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;
}


void TranslateAddOrSub (MathOp_t type, Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // move to 'r'

    if (*(ptr + 5) == 'r')
        TranslateAddOrSubTwoRegs(type, asm_struct, ptr);
    else if (*(ptr + 5) <= '9' && *(ptr + 5) >= '0')
        TranslateAddOrSubRegAndNum(type, asm_struct, ptr);
    else
        printf("Error in TranslateAddOrSub\n");
}


void TranslateAddOrSubRegAndNum (MathOp_t type, Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;
    asm_struct->bytes[count + 1] = 0x81;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = DEFAULT;

    if (type == ADD)
        src = NUM_ADD;
    else
        src = NUM_SUB;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    int value = 0;
    sscanf(ptr + 5, "%d", &value); // читаем 4-байтную константу

    // записываем 4-байтное значение в little-endian
    asm_struct->bytes[count + 3] = (unsigned char) value & 0xFF;
    asm_struct->bytes[count + 4] = (unsigned char) (value >> 8) & 0xFF;
    asm_struct->bytes[count + 5] = (unsigned char) (value >> 16) & 0xFF;
    asm_struct->bytes[count + 6] = (unsigned char) (value >> 24) & 0xFF;

    asm_struct->byte_counter += 7;
}


void TranslateAddOrSubTwoRegs (MathOp_t type, Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0x48;

    if (type == ADD)
        asm_struct->bytes[count + 1] = 0x01;
    else
        asm_struct->bytes[count + 1] = 0x29;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 5);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;
}


int TranslateSyscall (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x0F;
    asm_struct->bytes[count + 1] = 0x05;

    asm_struct->byte_counter += 2;

    return 0;
}


int TranslateXor (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // move to 'r'

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x31;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 5);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslatePop (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // move to 'r'

    unsigned char byte = 0x58 + CalculateByteForPushPopInstr(ptr);

    asm_struct->bytes[asm_struct->byte_counter] = byte;
    asm_struct->byte_counter++;

    return 0;
}


int TranslatePush (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 5; // move to 'r'

    unsigned char byte = 0x50 + CalculateByteForPushPopInstr(ptr);

    asm_struct->bytes[asm_struct->byte_counter] = byte;
    asm_struct->byte_counter++;

    return 0;
}


unsigned char CalculateByteForPushPopInstr (char * ptr)
{
    unsigned char byte = 0;

    Op_t reg = ParseRegName(ptr);

    switch(reg)
    {
        case RAX:            break;
        case RCX: byte++;    break;
        case RDX: byte += 2; break;
        case RBX: byte += 3; break;
        case RSP: byte += 4; break;
        case RBP: byte += 5; break;
        case RSI: byte += 6; break;
        case RDI: byte += 7; break;
        case XMM0:
        case NUM_MOV:
        case NUM_ADD:
        case NUM_SUB:
        case LABEL:
        case DEFAULT:
        default: printf("Error in CalculateByteForPushPopInstr\n");
    }

    return byte;
}


int TranslateCall (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xE8;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, ptr + 5, asm_struct->byte_counter + 1)) != 0)
        return -1;

    asm_struct->byte_counter += 5; // 4 bytes for offset are null for now

    return 0;
}


int TranslateLea (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // skip "lea" and space

    if (*(ptr + 6) == 'r' && *(ptr + 9) == ']')
        TranslateLeaFromMemRegNoDisp(asm_struct, ptr);
    else if (*(ptr + 6) == 'r' && *(ptr + 10) == '+')
        TranslateLeaFromMemRegWithDisp(asm_struct, ptr);
    else
        TranslateLeaFromMemLabel(asm_struct, ptr);

    return 0;
}


int TranslateLeaFromMemLabel (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8D;

    unsigned char mod = 0;

    Op_t dst = ParseRegName(ptr);
    Op_t src = LABEL;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, ptr + 6, asm_struct->byte_counter + 3)) != 0)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now
    return 0;
}


void TranslateLeaFromMemRegWithDisp (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8D;

    unsigned char mod = 0;
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 6);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    while (*ptr != '+')
        ptr++;
    unsigned char offset = 0;
    sscanf(ptr + 2, "%hhu", &offset);
    asm_struct->bytes[count + 3] = offset; // disp = 8 bit

    asm_struct->byte_counter += 4;
}


void TranslateLeaFromMemRegNoDisp (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8D;

    unsigned char mod = 0;

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 6);

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;
}


int TranslateMov (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    ptr += 4; // skip "mov" and space

    if (*ptr == 'r' && *(ptr + 5) == 'r') // mov reg, reg
        return TranslateMovRegToReg(asm_struct, ptr);
    else if (*ptr == 'e' && *(ptr + 5) >= '0' && *(ptr + 5) <= '9')
        return TranslateMovNumToReg(asm_struct, ptr);
    else if (*ptr == '[' && *(ptr + 5) == '+')
        return TranslateMovRegToMemRegWithDisp(asm_struct, ptr);
    else if (*ptr == '[' && (*(ptr + 1) != 'r' || (*(ptr + 1) == 'r' && *(ptr + 3) != 'x' && *(ptr + 3) != 'i' && *(ptr + 3) != 'p')))
        return TranslateMovRegToMemLabel(asm_struct, ptr);
    else if (*ptr == '[' && *(ptr + 7) == 'r')
        return TranlateMovRegToMemRegNoDisp(asm_struct, ptr);
    else if (*ptr == 'r' && *(ptr + 6) == 'r' && *(ptr + 10) == '+')
        return TranslateMovMemRegWithDispToReg(asm_struct, ptr);
    else if (*ptr == 'r' && *(ptr + 6) == 'r' && *(ptr + 9) == ']' && (*(ptr + 8) == 'x' || *(ptr + 8) == 'i' || *(ptr + 8) == 'p'))
        return TranslateMovMemRegNoDispToReg(asm_struct, ptr);
    else if (*ptr == 'r' && *(ptr + 5) == '[')
        return TranslateMovMemLabelToReg(asm_struct, ptr);
    else
        printf("Error while translating mov. String: %s\n", ptr);

    return -1;
}


int TranslateMovMemLabelToReg (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8B;

    unsigned char mod = 0;

    Op_t dst = ParseRegName(ptr);
    Op_t src = LABEL;

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, ptr + 6, asm_struct->byte_counter + 3)) != 0)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now

    return 0;
}


int TranslateMovMemRegWithDispToReg (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8B;

    unsigned char mod = 0;
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 6);

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    while (*ptr != '+')
        ptr++;
    unsigned char offset = 0;
    sscanf(ptr + 2, "%hhu", &offset);
    asm_struct->bytes[count + 3] = offset; // disp = 8 bit

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateMovMemRegNoDispToReg (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x8B;

    unsigned char mod = 0;

    Op_t dst = ParseRegName(ptr);
    Op_t src = ParseRegName(ptr + 6);

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, DstFirst);
    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateMovRegToMemRegWithDisp (Asm_t * asm_struct, char * ptr) // max 1 byte disp!
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x89;

    unsigned char mod = 0;
    mod |= (1 << 6);

    Op_t dst = ParseRegName(ptr + 1);

    while (*ptr != ']')
        ptr++;

    Op_t src = ParseRegName(ptr + 3);

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    while (*ptr != '+')
        ptr++;
    unsigned char offset = 0;
    sscanf(ptr + 2, "%hhu", &offset);

    asm_struct->bytes[count + 3] = offset; // disp = 8 bit

    asm_struct->byte_counter += 4;

    return 0;
}


int TranslateMovRegToMemLabel (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x89;

    unsigned char mod = 0;

    int res = 0;
    if ((res = AddNamePointer(asm_struct, ptr + 1, asm_struct->byte_counter + 3)) != 0)
        return -1;

    while (*ptr != ']')
        ptr++; // move from label

    Op_t src = ParseRegName(ptr + 3); // *(ptr + 3) == 'r'
    Op_t dst = LABEL;

    if (src == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 7; // 4 bytes for offset are null for now
    return 0;
}


int AddNamePointer (Asm_t * asm_struct, char * ptr, int var_offset)
{
    assert(asm_struct);
    assert(ptr);

    int name_counter = asm_struct->name_counter;

    char name[20] = {}; // FIXME

    for(int count = 0; *ptr != ']' && *ptr != '\0'; count++, ptr++)
        name[count] = *ptr;

    printf("AddNamePointer: Name came: %s\n", name);

    for (int count = 0; count < name_counter; count++)
    {
        if (strcmp(name, asm_struct->names[count].name) == 0)
        {
            asm_struct->names[count].byte_pointers[asm_struct->names[count].size_of_byte_pointers_array] = var_offset;
            asm_struct->names[count].size_of_byte_pointers_array++;
            return 0;
        }
    }

    printf("Error: var has not been found\n");
    return -1;
}


int TranlateMovRegToMemRegNoDisp (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x89;

    unsigned char mod = 0;

    Op_t src = ParseRegName(ptr + 7); // ptr points on 'r' - the beginnig of reg name
    Op_t dst = ParseRegName(ptr + 1);

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


int TranslateMovNumToReg (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count] = 0xC7;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t src = NUM_MOV;
    Op_t dst = ParseRegName(ptr);

    if (dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 1] = mod_rm;

    int value = 0;
    sscanf(ptr + 5, "%d", &value); // читаем 4-байтную константу

    // записываем 4-байтное значение в little-endian
    asm_struct->bytes[count + 2] = (unsigned char) value & 0xFF;
    asm_struct->bytes[count + 3] = (unsigned char) (value >> 8) & 0xFF;
    asm_struct->bytes[count + 4] = (unsigned char) (value >> 16) & 0xFF;
    asm_struct->bytes[count + 5] = (unsigned char) (value >> 24) & 0xFF;

    asm_struct->byte_counter += 6;

    return 0;
}


int TranslateMovRegToReg (Asm_t * asm_struct, char * ptr)
{
    assert(asm_struct);
    assert(ptr);

    int count = asm_struct->byte_counter;

    asm_struct->bytes[count]     = 0x48;
    asm_struct->bytes[count + 1] = 0x89;

    unsigned char mod = 0;
    mod |= (1 << 7);
    mod |= (1 << 6);

    Op_t src = ParseRegName(ptr + 5);
    Op_t dst = ParseRegName(ptr);

    if (src == DEFAULT || dst == DEFAULT)
        return -1;

    unsigned char mod_rm = MakeModeRm(mod, src, dst, SrcFirst);

    asm_struct->bytes[count + 2] = mod_rm;

    asm_struct->byte_counter += 3;

    return 0;
}


Op_t ParseRegName (char * ptr)
{
    assert(ptr);

    Op_t reg = DEFAULT;

    switch (*(ptr + 1)) // second letter of reg name
    {
        case 'a': {reg = RAX;} break;
        case 'b': {if (*(ptr + 2) == 'x')
                    reg = RBX;
                  else if (*(ptr + 2) == 'p')
                    reg = RBP;
                  else
                    printf("Error\n");} break;
        case 'c': {reg = RCX;} break;
        case 'd': {if (*(ptr + 2) == 'x')
                    reg = RDX;
                  else if (*(ptr + 2) == 'i')
                    reg = RDI;
                  else
                    printf("Error\n");} break;
        case 's': {if (*(ptr + 2) == 'p')
                    reg = RSP;
                  else if (*(ptr + 2) == 'i')
                    reg = RSI;
                  else
                    printf("Error\n");} break;
        default: printf("Reg's not recognized\n");
    }

    return reg;
}


unsigned char MakeModeRm (unsigned char mod, Op_t src, Op_t dst, Order_t order)
{
    unsigned char mod_rm = mod;

    if (order == DstFirst)
    {
        Op_t val = src;
        src = dst;
        dst = val;
    }

    switch(src)
    {
        case RAX: break;
        case RBX: {mod_rm |= (1 << 3); mod_rm |= (1 << 4);} break;
        case RCX: {mod_rm |= (1 << 3);} break;
        case RDX: {mod_rm |= (1 << 4);} break;
        case RDI: {mod_rm |= (1 << 3); mod_rm |= (1 << 4); mod_rm |= (1 << 5);} break;
        case RSI: {mod_rm |= (1 << 4); mod_rm |= (1 << 5);} break;
        case RBP: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case RSP: {mod_rm |= (1 << 5);} break;
        case NUM_MOV: break;
        case NUM_ADD: break;
        case NUM_SUB: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case XMM0: break;
        case LABEL: {mod_rm |= (1 << 3); mod_rm |= (1 << 5);} break;
        case DEFAULT:
        default: printf("Error in MakeModRm(src)\n");
    }

    switch (dst)
    {
        case RAX: break;
        case RBX: {mod_rm |= (1 << 0); mod_rm |= (1 << 1);} break;
        case RCX: {mod_rm |= (1 << 0);} break;
        case RDX: {mod_rm |= (1 << 1);} break;
        case RDI: {mod_rm |= (1 << 0); mod_rm |= (1 << 1); mod_rm |= (1 << 2);} break;
        case RSI: {mod_rm |= (1 << 1); mod_rm |= (1 << 2);} break;
        case RBP: {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case RSP: {mod_rm |= (1 << 2);} break;
        case NUM_MOV: break;
        case NUM_ADD: break;
        case NUM_SUB: {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case XMM0: break;
        case LABEL:   {mod_rm |= (1 << 0); mod_rm |= (1 << 2);} break;
        case DEFAULT:
        default: printf("Error in MakeModRm(dst)\n");
    }

    return mod_rm;
}


void SetCodeSize (int code_size)
{
    program_header_text[32] = (unsigned char)  code_size & 0xFF;
    program_header_text[33] = (unsigned char) (code_size >> 8) & 0xFF;
    program_header_text[34] = (unsigned char) (code_size >> 16) & 0xFF;
    program_header_text[35] = (unsigned char) (code_size >> 24) & 0xFF;

    program_header_text[40] = (unsigned char)  code_size & 0xFF;
    program_header_text[41] = (unsigned char) (code_size >> 8)  & 0xFF;
    program_header_text[42] = (unsigned char) (code_size >> 16) & 0xFF;
    program_header_text[43] = (unsigned char) (code_size >> 24) & 0xFF;
}


void SetDataSizeAndOffset (int data_size, int code_size)
{
    // size
    program_header_data[32] = (unsigned char) data_size & 0xFF;
    program_header_data[33] = (unsigned char) (data_size >> 8) & 0xFF;
    program_header_data[34] = (unsigned char) (data_size >> 16) & 0xFF;
    program_header_data[35] = (unsigned char) (data_size >> 24) & 0xFF;

    program_header_data[40] = (unsigned char) data_size & 0xFF;
    program_header_data[41] = (unsigned char) (data_size >> 8) & 0xFF;
    program_header_data[42] = (unsigned char) (data_size >> 16) & 0xFF;
    program_header_data[43] = (unsigned char) (data_size >> 24) & 0xFF;

    //offset
    int offset = 0x400000 + (int)sizeof(elf_header) + (int)sizeof(program_header_text) + (int)sizeof(program_header_data) + code_size;

    program_header_data[16] = (unsigned char) offset & 0xFF;
    program_header_data[17] = (unsigned char) (offset >> 8)  & 0xFF;
    program_header_data[18] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[19] = (unsigned char) (offset >> 24) & 0xFF;

    program_header_data[24] = (unsigned char) offset & 0xFF;
    program_header_data[25] = (unsigned char) (offset >> 8)  & 0xFF;
    program_header_data[26] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[27] = (unsigned char) (offset >> 24) & 0xFF;

    offset -= 0x400000;

    program_header_data[8] = (unsigned char) offset & 0xFF;
    program_header_data[9] = (unsigned char)  (offset >> 8)  & 0xFF;
    program_header_data[10] = (unsigned char) (offset >> 16) & 0xFF;
    program_header_data[11] = (unsigned char) (offset >> 24) & 0xFF;
}
