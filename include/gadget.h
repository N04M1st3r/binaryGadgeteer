// #pragma once

// #include "endGadgetFind.h"

// #include <inttypes.h>
// #include <Zydis/Zydis.h>
// #include <stdbool.h>


// /*
// for each instruction in the ropgadget I want to save:
// (some stuff lile ZydisDecodedInstruction_)
// 1) ZydisMnemonic mnemonic ; instruction (to know which instruction it is)
// 2) ZyanU8 length
// (I need to save the params that it gave it somehow, the operands)
// TODO change it so it will save the operands instead of the bytes maybe.
// 3) ZyanU8 instructionFullOpcode[ZYDIS_MAX_INSTRUCTION_LENGTH]


// here I had a lot of thought, maybe I should have saved it for each instruction and just a pointer to this
// and that would save a lot of space, 20 instead of 8 for every miniInstruction that is happening the 2nd times.
// but it would mean a lot more computation and time, I thought of saving it in an array and see by the bytes.
// an array, it's max size will be 15, which is way too much space,
// so we can't save this much space...
// okay so the second option is to save in a linked list and check one by one, problem too much time!
// so maybe hashmap, this is a good idea but again too much space, it would not improve the space.
// so I keep saving miniInstruction for each one.
// maybe if miniInstruction will save more than 20 bytes I will go with the other approch

// (btw saving instructionFullOpcode instead of reading from the location in the file because to read from the file takes too much time)
// */
// typedef struct miniInstruction
// {
//     //some basic info about the instruction
//     ZydisMnemonic mnemonic; //4
//     ZyanU8 instructionLength; //1

//     //later maybe save the operands and not the opcode.
//     ZyanU8 instructionFullOpcode[ZYDIS_MAX_INSTRUCTION_LENGTH]; //15

// } miniInstruction; //20


// //doing it like so so I can expand a gadget easily on demand.
// typedef struct gadget{
//     miniInstruction minInstruction;
//     size_t references; //counts the number of references
//     struct gadget *next;
// } gadget;


// /*
// A gadgetGeneral is just something to save the gadget in, I can easily add to a gadget
// but there are stuff that I want to be saved only once because there is no need to save them multiple times.
// */
// typedef struct gadgetGeneral{
//     gadget *first; //(last is Ret/Jmp/branch)
//     size_t length;
//     ZyanU64 vaddr; //(uint64_t) this is the vaddr of the first one (last one is RET/Jmp/Branch).
//     uint64_t addr_file;
//     bool checked; //if got to the end of the gadgetGeneral (cant decode more down).
// } gadgetGeneral;

// /*
// All my gadgets will be saved in this structure.
// (maybe change the structure later? for easier remove duplicates)
// */
// typedef struct gadgetGeneralNode{
//     gadgetGeneral gadgetG; //this is purpously not a pointer.
//     struct gadgetGeneralNode *next;
// } gadgetGeneralNode;

// typedef struct gadgetGeneralLinkedListEnds{
//     gadgetGeneralNode *start;
//     gadgetGeneralNode *end;
// } gadgetGeneralLinkedListEnds;


// gadgetGeneralNode *gadgetGeneralNodeCreate(gadgetGeneral gadgetG);
// gadgetGeneral gadgetGeneralCreateWithGadget(gadget firstGadget, uint64_t vaddr, uint64_t addr_file);
// gadgetGeneral gadgetGeneralCreateWithInstruction(miniInstruction instruction, uint64_t vaddr, uint64_t addr_file);


// void gadgetGeneralNodeFreeAll(gadgetGeneralNode *);

// void gadgetGeneralNodeFreeCurrent(gadgetGeneralNode *);

// //no need to call this, gadgetGeneralNodeFreeCurrent calls this.
// void gadgetFreeAll(gadget *gadget_p);

// int gadgetGeneralNodeAppendGadgetGeneralNode(gadgetGeneralNode *dest, gadgetGeneralNode *src);

// int gadgetGeneralNodeAddInstruction(gadgetGeneralNode *, miniInstruction instruction);


// gadgetGeneralLinkedListEnds expandInstructionDown(char *buffer, uint64_t buf_vaddr, uint64_t buf_fileOffset, size_t readAmount, FoundLocationsBufferNode *curBranchInstructionLocation, ZydisMnemonic branchInstructionMnemonic, size_t depth);
// void initDecoderAndFormatter(ZydisDecoder *decoder_given_p, ZydisFormatter *formatter_given_p);


// void gadgetGeneralNodeShowAll(gadgetGeneralNode *gadgetGNode_p);

// //TODO write this.
// void gadgetGeneralNodeShowAllCombined(gadgetGeneralNode *gadgetGNode_p);

// void gadgetGeneralNodeShowOnlyEnds(gadgetGeneralNode *gadgetGNode_p);