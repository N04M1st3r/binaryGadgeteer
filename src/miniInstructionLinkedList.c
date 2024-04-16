#include "miniInstructionLinkedList.h"
#include "costumErrors.h"

#include <stdlib.h>

static void MiniBranchInstructionLinkedListInit(MiniBranchInstructionLinkedList *);

/**
 * Adding miniInstruction to miniInstructionLL_p, will add to the start.
 * 
 * @param mnemonicOpcode[MAX_MEMONIC_OPCODE_LEN], miniInstruction mnemonic part of the opcode.
 * @param mnemonicOpcodeSize, the opcode mnemonic size (max is MAX_MEMONIC_OPCODE_LEN/3).
 * @param additionSize, the addition size that needs to be added for the opcode (the operands).
 * @param miniInstructionLL_p, mini instruction linked list pointer.
 * 
 * @return 0 on sucess. -1 and such on error.
*/
int miniInstructionLinkedListAdd(MiniBranchInstructionLinkedList *miniInstructionLL_p, uint8_t mnemonicOpcode[MAX_MEMONIC_OPCODE_LEN], uint8_t mnemonicOpcodeSize, uint8_t additionSize){
    MiniBranchInstructionNode *newStart = (MiniBranchInstructionNode *)malloc(sizeof(MiniBranchInstructionNode));
    if (newStart == NULL){
        err("Error in Malloc inside miniInstructionLinkedListAdd, for newStart of size %ld.", sizeof(MiniBranchInstructionNode));
        return 1;
    }
    
    newStart->instructionInfo.additionSize = additionSize;
    newStart->instructionInfo.mnemonicOpcodeSize = mnemonicOpcodeSize;
    memcpy(newStart->instructionInfo.mnemonicOpcode, mnemonicOpcode, MAX_MEMONIC_OPCODE_LEN);

    newStart->next = miniInstructionLL_p->start;

    miniInstructionLL_p->start = newStart;
    (miniInstructionLL_p->size)++;

    return 0;
}

/**
 * Creating and initilizing MiniBranchInstructionLinkedList. (allocating with malloc, on heap)
 * 
 *  @return a pointer to mini instruction linked list. NULL if error.
 * 
 *  @note !!!!in the end call miniInstructionLinkedListFreeRegular!!!!
*/
MiniBranchInstructionLinkedList *miniInstructionLinkedListCreate(void){
    MiniBranchInstructionLinkedList *miniInstructionLL_p = (MiniBranchInstructionLinkedList *)malloc(sizeof(MiniBranchInstructionLinkedList));
    if (miniInstructionLL_p == NULL){
        err("Error, Malloc for miniInstructionLL_p failed inside miniInstructionLinkedListCreate.\n");
        return NULL;
    }
    
    MiniBranchInstructionLinkedListInit(miniInstructionLL_p);
    
    return miniInstructionLL_p;
}

/**
 * Initilizes instruction linked list.
 * 
 * @param miniInstructionLL_p
*/
static void MiniBranchInstructionLinkedListInit(MiniBranchInstructionLinkedList *miniInstructionLL_p){
    miniInstructionLL_p->start = NULL;
    miniInstructionLL_p->size = 0;
}

/**
 * Frees miniInstructionLL_p, and everything inside it except instructionInfo and stuff in it.
 * 
 * @param miniInstructionLL_p, MiniBranchInstructionLinkedList to free
 * 
 * @note assuming instructions are not malloced (like in miniInstructionLinkedListAdd)
 * 
 * @note USING THIS!
*/
void miniInstructionLinkedListFreeRegular(MiniBranchInstructionLinkedList *miniInstructionLL_p){
    
    MiniBranchInstructionNode *curNode = miniInstructionLL_p->start;
    MiniBranchInstructionNode *tmp;
    while(curNode != NULL){
        tmp = curNode->next;

        free(curNode);
        curNode = tmp;
    }
    
    //miniInstructionLL_p->start = NULL;
    //miniInstructionLL_p->size = 0;

    free(miniInstructionLL_p);
}