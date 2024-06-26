#pragma once

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_MEMONIC_OPCODE_LEN 3

/*
MiniBranchInstruction ideas to improve and explanation
note: can maybe get rid of mnemonicOpcode but there is no must that a mnemonicOpcode will not contain 0

maybe add instructionPrefix to there.

mnemonicOpcode can also be looked at as startOpcode.

it is less space to save 3 bytes instead of 4/8 for a pointer. (and a little faster and cleaner)

mnemonicOpcode is not ending with \x00!!! it only saves data!!
Example 1:
    CA iw(word)       RET        Far  return

    will be saved like so:
    {{0xCA, 0, 0}, 1, 2}

    (there is no promise that they will contain 0 in mnemonicOpcode!)
    {{0xCA, ?, ?}, 1, 2}

Example 2:
    C3                RET        Near return
    
    will be saved as:
    {{0xC3, 0, 0}, 1, 0};


*/
typedef struct MiniBranchInstruction{ //saving the minimum info needed on the instruction (and diffrerent from regular instruciton because I need to search with that)
    uint8_t mnemonicOpcode[MAX_MEMONIC_OPCODE_LEN];       /*Max mnemonicOpcode/startOpcode length is 3 (according to intel manual), start opcode*/
    uint8_t mnemonicOpcodeSize;
    uint8_t additionSize; //in a case checkThis is true, this is max.
    ZydisMnemonic mnemonic;
    bool checkThis; //A hacky solution becuase I don't have time, make better later. when this is on the others are unknown except the mnemonic and mnemonicOpcode(kinda)
} MiniBranchInstruction; //YOU ARE NOT SUPPOUS TO CREATE OBJECT OF THIS, just use miniInstructionLinkedListFreeNoInstructionInfo

typedef struct MiniBranchInstructionNode{
    MiniBranchInstruction instructionInfo;
    struct MiniBranchInstructionNode *next;
} MiniBranchInstructionNode;

typedef struct MiniBranchInstructionLinkedList{
    MiniBranchInstructionNode *start;
    //maybe add end for faster miniBranchInstructionLinkedListCombine.
    size_t size;
} MiniBranchInstructionLinkedList;


int miniBranchInstructionLinkedListAdd(MiniBranchInstructionLinkedList *, uint8_t mnemonicOpcode[3], uint8_t mnemonicOpcodeSize, uint8_t additionSize, ZydisMnemonic mnemonic, bool checkThis);

void miniBranchInstructionLinkedListCombine(MiniBranchInstructionLinkedList *into, MiniBranchInstructionLinkedList *from);

MiniBranchInstructionLinkedList *miniBranchInstructionLinkedListCreate(void);

//use this with miniBranchInstructionLinkedListAdd.
void miniBranchInstructionLinkedListFreeRegular(MiniBranchInstructionLinkedList *);

