#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char test_buffer[256] = "wakeio=3 kbps=1024 dtim=1　ticks=60 testid=1 packetsize=1 pmmode=2";

typedef struct _test_command {
    char* name;
    int   init_value;
}UNI_TEST_COMMAND;

enum para_id{
    PARA_DTIM = 0,
    PARA_TICKS,
    PARA_TESTID,
    PARA_KBPS,
    PARA_PACKETSIZE,
    PARA_PMMODE,
    PARA_WAKEIO,
    PARA_MAXNUM,
};

static int para_array[PARA_MAXNUM]={0};
static const UNI_TEST_COMMAND command_array[PARA_MAXNUM] = {
    { "dtim=", 1},
    { "ticks=", 60},
    { "testid=", 1},
    { "kbps=", 1024},
    { "packetsize=", 1},
    { "pmmode=", 2},
    { "wakeio=", 3},
};

int retrieve_para(char *test_cmd)
{
    char* ptr1 = NULL;
    char* ptr2 = NULL;
    char* ptr3 = NULL;
    char sub_para[32];
    int index = 0;

    for (index=0; index<PARA_MAXNUM; index++) {
        para_array[index] = command_array[index].init_value;
    }

    index = 0;
    while (index < PARA_MAXNUM) {
        ptr1 = strstr(test_cmd, command_array[index].name);
        if (ptr1) {
            ptr2 = ptr1 + strlen(command_array[index].name);
            ptr3 = strpbrk(ptr2, " ");
            if (ptr3) {
                memcpy(sub_para, ptr2, ptr3-ptr2);
                sub_para[ptr3-ptr2] = '\0';
            } else {
                strcpy(sub_para, ptr2);
            }
            printf("%s%d\n", command_array[index].name, atoi(sub_para));
        }

        index++;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    retrieve_para(test_buffer);

    return 0;
}