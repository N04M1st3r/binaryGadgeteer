#include "miniInstructionList.h"
#include "costumErrors.h"

#include <stdlib.h>

static void MiniInstructionLinkedListInit(MiniInstructionLinkedList *);

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
int miniInstructionLinkedListAdd(MiniInstructionLinkedList *miniInstructionLL_p, uint8_t mnemonicOpcode[MAX_MEMONIC_OPCODE_LEN], uint8_t mnemonicOpcodeSize, uint8_t additionSize){
    MiniInstructionNode *newStart = (MiniInstructionNode *)malloc(sizeof(MiniInstructionNode));
    if (newStart == NULL){
        err("Error in Malloc inside miniInstructionLinkedListAdd, for newStart of size %ld.", sizeof(MiniInstructionNode));
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
 * Creating and initilizing MiniInstructionLinkedList. (allocating with malloc, on heap)
 * 
 *  @return a pointer to mini instruction linked list. NULL if error.
 * 
 *  @note !!!!in the end call miniInstructionLinkedListFreeRegular!!!!
*/
MiniInstructionLinkedList *miniInstructionLinkedListCreate(void){
    MiniInstructionLinkedList *miniInstructionLL_p = (MiniInstructionLinkedList *)malloc(sizeof(MiniInstructionLinkedList));
    if (miniInstructionLL_p == NULL){
        err("Error, Malloc for miniInstructionLL_p failed inside miniInstructionLinkedListCreate.\n");
        return NULL;
    }
    
    MiniInstructionLinkedListInit(miniInstructionLL_p);
    
    return miniInstructionLL_p;
}

/**
 * Initilizes instruction linked list.
 * 
 * @param miniInstructionLL_p
*/
static void MiniInstructionLinkedListInit(MiniInstructionLinkedList *miniInstructionLL_p){
    miniInstructionLL_p->start = NULL;
    miniInstructionLL_p->size = 0;
}

/**
 * Frees miniInstructionLL_p, and everything inside it except instructionInfo and stuff in it.
 * 
 * @param miniInstructionLL_p, MiniInstructionLinkedList to free
 * 
 * @note assuming instructions are not malloced (like in miniInstructionLinkedListAdd)
 * 
 * @note USING THIS!
*/
void miniInstructionLinkedListFreeRegular(MiniInstructionLinkedList *miniInstructionLL_p){
    
    MiniInstructionNode *curNode = miniInstructionLL_p->start;
    MiniInstructionNode *tmp;
    while(curNode != NULL){
        tmp = curNode->next;

        free(curNode);
        curNode = tmp;
    }
    
    //miniInstructionLL_p->start = NULL;
    //miniInstructionLL_p->size = 0;

    free(miniInstructionLL_p);
}