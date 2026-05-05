#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include "asm.h"

int main (int argc, char * argv[])
{
    const char * input_file = "ASM.s";

    if (argc == 2)
        input_file = argv[1];
    else if (argc > 2)
    {
        printf("Invalid cmd line arguments\n");
        return -1;
    }

    Asm_t asm_struct = InitAsmStruct();
    int res = 0;

    if ((res = FillHashtable()) != 0)
        return -1;

    if ((res = CopyAsmFileToBuffer(&asm_struct, input_file)) != 0)
    {
        printf("Error while copying file\n");
        return -1;
    }

    if ((res = FillPointerBuff(&asm_struct)) != 0)
    {
        printf("Error while filling pointer buff\n");
        return -1;
    }

    if ((res = FillNametable(&asm_struct)) != 0)
    {
        printf("Error while filling nametable\n");
        return -1;
    }

    if ((res = CheckCollisions(&asm_struct)) != 0)
    {
        printf("Collision error\n");
        return -1;
    }

    if ((res = FirstCompilation(&asm_struct)) != 0)
    {
        printf("Error while first compilation\n");
        return -1;
    }

    // первый проход: трансляция команд, заполнение буферов меток, функций и переменных, адреса пустые

    if ((res = SecondCompilation(&asm_struct)) != 0)
        printf("Error while second compilation\n");

    // второй проход: заполнение адресов

    // пока без подключения стандартных функций

    // Сборка исполняемого файла (ниже)

    FILE * fp_out = fopen("asm_output", "w+b");
    assert(fp_out);

    int code_size = asm_struct.start_of_data_segment;
    int data_size = asm_struct.byte_counter - code_size;

    SetCodeSize(code_size);
    SetDataSizeAndOffset(data_size, code_size);

    FwriteAll(fp_out, &asm_struct);

    fclose(fp_out);
    chmod("asm_output", 0755);

    DestroyAll(&asm_struct);

    return 0;
}
