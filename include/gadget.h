#pragma once

#include "endGadgetFind.h"

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdbool.h>


/*
for each instruction in the ropgadget I want to save:
(some stuff lile ZydisDecodedInstruction_)
1) ZydisMnemonic mnemonic ; instruction (to know which instruction it is)
2) ZyanU8 length
(I need to save the params that it gave it somehow, the operands)
TODO change it so it will save the operands instead of the bytes maybe.
3) ZyanU8 instructionFullOpcode[ZYDIS_MAX_INSTRUCTION_LENGTH]
*/
typedef struct miniInstruction
{
    //some basic info about the instruction
    ZydisMnemonic mnemonic;
    ZyanU8 instructionLength;

    //later maybe save the operands and not the opcode.
    ZyanU8 instructionFullOpcode[ZYDIS_MAX_INSTRUCTION_LENGTH];

} miniInstruction;


//doing it like so so I can expand a gadget easily on demand.
typedef struct gadget{
    miniInstruction minInstruction;
    size_t refrences; //counts the number of refrences
    struct gadget *next;
} gadget;


/*
A gadgetGeneral is just something to save the gadget in, I can easily add to a gadget
but there are stuff that I want to be saved only once because there is no need to save them multiple times.
*/
typedef struct gadgetGeneral{
    gadget *first; //(last is Ret/Jmp/branch)
    size_t length;
    ZyanU64 vaddr; //(uint64_t) this is the vaddr of the first one (last one is RET/Jmp/Branch).
    uint64_t addr_file;
    bool checked; //if got to the end of the gadgetGeneral (cant decode more down).
} gadgetGeneral;

/*
All my gadgets will be saved in this structure.
(maybe change the structure later? for easier remove duplicates)
*/
typedef struct gadgetGeneralNode{
    gadgetGeneral gadgetG; //this is purpously not a pointer.
    struct gadgetGeneralNode *next;
} gadgetGeneralNode;


gadgetGeneralNode *gadgetGeneralNodeCreate(gadgetGeneral gadgetG);
gadgetGeneral gadgetGeneralCreateWithGadget(gadget firstGadget, uint64_t vaddr, uint64_t addr_file);
gadgetGeneral gadgetGeneralCreateWithInstruction(miniInstruction instruction, uint64_t vaddr, uint64_t addr_file);


void gadgetGeneralNodeFreeAll(gadgetGeneralNode *);

void gadgetGeneralNodeFreeCurrent(gadgetGeneralNode *);

//no need to call this, gadgetGeneralNodeFreeCurrent calls this.
void gadgetFreeAll(gadget *gadget_p);

int gadgetGeneralNodeAppendGadgetGeneralNode(gadgetGeneralNode *dest, gadgetGeneralNode *src);

int gadgetGeneralNodeAddInstruction(gadgetGeneralNode *, miniInstruction instruction);


gadgetGeneralNode *expandInstructionDown(char *buffer, uint64_t buf_vaddr, uint64_t buf_fileOffset, size_t readAmount, FoundLocationsBufferNode *curBranchInstructionLocation, ZydisMnemonic branchInstructionMnemonic, size_t depth);
void initDecoderAndFormatter(ZydisDecoder *decoder_given_p, ZydisFormatter *formatter_given_p);


void gadgetGeneralNodeShowAll(gadgetGeneralNode *gadgetGNode_p);

//TODO write this.
void gadgetGeneralNodeShowAllCombined(gadgetGeneralNode *gadgetGNode_p);