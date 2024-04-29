
#pragma once

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdbool.h>
//second attempt at gadget.c


typedef struct MiniInstruction
{
    //some basic info about the instruction
    ZydisMnemonic mnemonic; //4
    ZyanU8 instructionLength; //1

    //maybe later maybe save the operands and not the opcode.
    ZyanU8 instructionFullOpcode[ZYDIS_MAX_INSTRUCTION_LENGTH]; //15

} MiniInstruction; //20

/*
This is holding a little more information then miniInstruction, and is a node.
*/
typedef struct MiniInstructionNode
{
    /* counts the number of references, there can't be more than 255 refrences. (maybe size_t) */
    uint8_t refCnt; 

    MiniInstruction miniInst;
    
    struct MiniInstructionNode *next;
} MiniInstructionNode;

typedef struct GadgetNode{
    /*
    Length of the gadget, number of instructions (I don't believe anyone will have more than 255 instructions)
    (if that happens then maybe change to size_t)*/
    uint8_t len;

    /* Address in the file */
    uint64_t addr_file;

    /* Address at the running memory */ 
    ZyanU64 vaddr; 

    MiniInstructionNode *first;
    struct GadgetNode *next;
} GadgetNode;


/*
gadget Linked list.
*/
typedef struct GadgetLL{

    /* Number of gadgets in the gadget list (I don't have)*/
    size_t size;
    GadgetNode *start;
    GadgetNode *end;
    
} GadgetLL;


void GadgetLLCombine(GadgetLL *dest, GadgetLL *src);
void GadgetLLAddGadgetNode(GadgetLL *gadgetsLL, GadgetNode *curGadgetNode);

/* Malloc related */
GadgetLL *gadgetLLCreate(GadgetNode *first);
MiniInstructionNode *MiniInstructionNodeCreate(ZydisMnemonic mnemonic, ZyanU8 instructionLength, char *fullOpcode, MiniInstructionNode *next);
GadgetNode *GadgetNodeCreate(MiniInstructionNode *instNode, uint64_t addr_file, ZyanU64 vaddr);

/* Free related */
void gadgetLLFreeAll(GadgetLL *gadgetLL);
void gadgetLLFreeOnly(GadgetLL *gadgetLL);

void gadgetNodeFreeCurrentOnly(GadgetNode *gadgetNode);
void gadgetNodeFreeCurrentAll(GadgetNode *gadgetNode);

void MiniInstructionNodeFree(MiniInstructionNode *cur);

