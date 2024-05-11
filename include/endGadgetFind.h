#pragma once

#include "miniBranchInstructionLinkedList.h"
#include "gadgetLinkedList.h"

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdbool.h>

/*
//maybe create a file just for this:
typedef struct FoundLocationsBufferNode{
    size_t offset; //offset inside buffer.
    MiniBranchInstruction miniInstructionInfo;
    struct FoundLocationsBufferNode *next;
} FoundLocationsBufferNode;
*/


typedef struct ArchInfo
{
    ZydisMachineMode machine_mode;
    ZydisStackWidth stack_width;

    //some important assembly opcodes to be saved:
    MiniBranchInstructionLinkedList *retEndings; 
    MiniBranchInstructionLinkedList *jmpEndings;
    MiniBranchInstructionLinkedList *callEndings;
} ArchInfo;


GadgetLL *searchBranchInstructionsInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t addr_file, size_t bufferSize, ArchInfo *arch_p);

GadgetLL *searchJmpInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t addr_file, size_t bufferSize, ArchInfo *arch_p);

GadgetLL *searchRetInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t addr_file, size_t bufferSize, ArchInfo *arch_p);

//void FoundLocationsBufferNodeFree(gadgetLL *);

//will maybe expand this function in the future
ArchInfo *initArchInfo(const char *archName);

void freeArchInfo(ArchInfo *arch_p);


//maybe change name to branchFind