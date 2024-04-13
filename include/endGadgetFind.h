#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdbool.h>

typedef struct FoundLocationsNode{
    uint64_t offset;
    struct FoundLocationsNode *next;
} FoundLocationsNode;

typedef struct ArchInfo
{
    ZydisMachineMode machine_mode;

    //some important assembly opcodes to be saved:
    MiniInstruction *retEndings; 
    MiniInstruction *jmpEndings;
} ArchInfo;


FoundLocationsNode *searchRetInBuffer(char *buffer, uint64_t bufferSize, ArchInfo *arch);

//will maybe expand this function in the future
ArchInfo *initArchInfo(const char *archName);

void freeArchInfo(ArchInfo *arch);



//maybe change name to branchFind