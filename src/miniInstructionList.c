#include "miniInstructionList.h"
#include "costumErrors.h"

/**
 * Appending miniInstruction to miniInstructionLL_p
 * 
 * @param miniInstruction, the mini instruction to append.
 * @param miniInstructionLL_p
 * 
 * @return 0 on sucess. -1 and such on error.
*/
int miniInstructionLinkedListAppend(MiniInstructionLinkedList *miniInstructionLL_p, MiniInstruction *miniInstruction){
    (miniInstructionLL_p->size)++;
    
}


/**
 * Creating (not initilizing) MiniInstruction. (mallocing) 
 * 
 * @return A malloced MiniInstruction pointer that is initilized to 0.
 *          If error returns NULL.
 * 
 * @note !!!THIS IS MALLOCED, REMEMBER TO FREE, note that miniInstructionLinkedListFreeAll also free's them
*/
MiniInstruction *miniInstructionCreate(void){
    MiniInstruction *miniInstruction = (MiniInstruction *)malloc(sizeof(MiniInstruction));
    if(miniInstruction == NULL){
        err("Error, Malloc for miniInstruction failed inside MiniInstructionCreate.\n");
        return NULL;
    }

    return miniInstruction;
}

/**
 * Creating and initilizing MiniInstructionLinkedList. (allocating with malloc, on heap)
 * 
 *  @return a pointer to mini instruction linked list. NULL if error.
 * 
 *  @note !!!!in the end call miniInstructionLinkedListFreeAll!!!! (or miniInstructionLinkedListFreeNoInstructionInfo, depending on how you appended them)
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
    miniInstructionLL_p->end = NULL;
    miniInstructionLL_p->size = 0;
}

/**
 * Frees miniInstructionLL_p, and everything inside it.
 * 
 * @param miniInstructionLL_p, MiniInstructionLinkedList to free
 * 
 * @note assuming instructions a are also malloced
*/
void miniInstructionLinkedListFreeAll(MiniInstructionLinkedList *miniInstructionLL_p){
    
    MiniInstructionNode *curNode = miniInstructionLL_p->start;
    MiniInstructionNode *tmp;
    while(curNode != NULL){
        tmp = curNode->next;

        free(curNode->instructionInfo);
        free(curNode);

        curNode = tmp;
    }
    
    //miniInstructionLL_p->start = NULL;
    //miniInstructionLL_p->end = NULL;
    //miniInstructionLL_p->size = 0;

    free(miniInstructionLL_p);
}


/**
 * Frees miniInstructionLL_p, and everything inside it except instructionInfo and stuff in it.
 * 
 * @param miniInstructionLL_p, MiniInstructionLinkedList to free
 * 
 * @note assuming instructions a are also malloced
*/
void miniInstructionLinkedListFreeNoInstructionInfo(MiniInstructionLinkedList *miniInstructionLL_p){
    
    MiniInstructionNode *curNode = miniInstructionLL_p->start;
    MiniInstructionNode *tmp;
    while(curNode != NULL){
        tmp = curNode->next;

        free(curNode);
        curNode = tmp;
    }
    
    //miniInstructionLL_p->start = NULL;
    //miniInstructionLL_p->end = NULL;
    //miniInstructionLL_p->size = 0;

    free(miniInstructionLL_p);
}