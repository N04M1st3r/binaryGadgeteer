#pragma once

#include "miniInstructionLinkedList.h"

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdbool.h>

//maybe create a file just for this:
typedef struct FoundLocationsBufferNode{
    size_t offset; //offset inside buffer.
    struct FoundLocationsBufferNode *next;
} FoundLocationsBufferNode;



typedef struct ArchInfo
{
    ZydisMachineMode machine_mode;

    //some important assembly opcodes to be saved:
    MiniInstructionLinkedList *retEndings; 
    MiniInstructionLinkedList *jmpEndings;
} ArchInfo;


FoundLocationsBufferNode *searchRetInBuffer(char *buffer, size_t bufferSize, ArchInfo *arch_p);

void FoundLocationsBufferNodeFree(FoundLocationsBufferNode *);

//will maybe expand this function in the future
ArchInfo *initArchInfo(const char *archName);

void freeArchInfo(ArchInfo *arch_p);


//maybe change name to branchFind