#include "gadgetLinkedList.h"
#include "costumErrors.h"

#include <string.h>

/**
 * creates a linked list of gadgets, gadgetLL
 * 
 * @return created and initilized gadgetLL *. if error NULL
 * 
 * @note malloced, remeber to free with freeGadgetLL
*/
GadgetLL *gadgetLLCreate(GadgetNode *first){
    GadgetLL *res = (GadgetLL *)malloc(sizeof(GadgetLL));
    if(res == NULL){
        err("Error in malloc, at createGadgetLL for res.");
        return NULL;
    }
    res->size = 1;
    res->start = first;
    res->end = first;
    return res;
}




/**
 * Creates a MiniInstructionNode.
 * 
 * @param mnemonic the mnemonic of the opcode.
 * @param instructionLength instruction total length.
 * @param fullOpcode fullopcode, will be copied from there to here.
 * @param next The next MiniInstructionNode after him (prob NULL).
 * 
 * @return MiniInstructionNode * with the coorosponding values, on error NULL.
 * 
 * @note this is using malloc, remember to free using MiniInstructionNodeFree.
*/
MiniInstructionNode *MiniInstructionNodeCreate(ZydisMnemonic mnemonic, ZyanU8 instructionLength, char *fullOpcode, MiniInstructionNode *next){
    MiniInstructionNode *res = (MiniInstructionNode *)malloc(sizeof(MiniInstructionNode));
    if(res == NULL){
        err("Error inside MiniInstructionNodeCreate at malloc for res.");
        return NULL;
    }
    res->miniInst.mnemonic = mnemonic;
    res->miniInst.instructionLength = instructionLength;
    memcpy(res->miniInst.instructionFullOpcode, fullOpcode, instructionLength);
    
    //he himself.
    res->refCnt = 1; 
    
    res->next = next;
    return res;
}


/**
 * Creates a gadgetNode.
 * 
 * @param instNode the start instruction.
 * 
 * @return An allocated GadgetNode *, if error then NULL.
 * 
 * @note this is using malloc, remember to free with gadgetNodeFreeCurrent
*/
GadgetNode *GadgetNodeCreate(MiniInstructionNode *instNode, uint64_t addr_file, ZyanU64 vaddr){
    GadgetNode *res = (GadgetNode *)malloc(sizeof(GadgetNode));
    if(res == NULL){
        err("Error inside GadgetNodeCreate at malloc.");
        return NULL;
    }
    res->len = 1;
    res->addr_file = addr_file;
    res->vaddr = vaddr;
    res->first = instNode;
    res->next = NULL;

    return res;
}




/**
 * Frees all gadgetLL in a safe way.
 * 
 * @note assuming gadgetLL != NULL and gadgetLL->start != NULL and gadgetLL->start->first != NULL.
*/
void gadgetLLFreeAll(GadgetLL *gadgetLL){
    GadgetNode *cur = gadgetLL->start;
    GadgetNode *next;

    while(cur != NULL){
        next = cur->next;
        gadgetNodeFreeCurrent(cur);
        cur = next;
    }
    
    free(gadgetLL);
}

/**
 * Frees gadgetNode in a safe way.
 * 
 * @note assuming gadgetNode != NULL and gadgetNode->first != NULL.
 * @note not freeing next.
*/
void gadgetNodeFreeCurrent(GadgetNode *gadgetNode){
    MiniInstructionNodeFree(gadgetNode->first);
    free(gadgetNode);
}

/**
 * Frees MiniInstructionNode in a safe way.
 * 
 * @note assuming cur != NULL.
 * @note using the refrence way.
*/
void MiniInstructionNodeFree(MiniInstructionNode *cur){
    MiniInstructionNode *next;
    (cur->refCnt)--;

    while(cur != NULL && cur->refCnt == 0){
        next = cur->next;
        free(cur);

        if(next)
            (next->refCnt)--;
        cur = next;
    }
}



/**
 * Combining src to dest.
 * 
 * @param dest the gadgetLL * that will be combined into.
 * @param src the gadgetLL * that will be "taken" from.
 * 
 * @note dest will be changed, but src will not be changed, although the variables inside them are the same.
 * @note assuming dest!=NULL.
 * @note asuuming there will not be integer overflow while combining the sizes.
*/
void GadgetLLCombine(GadgetLL *dest, GadgetLL *src){
    if(src == NULL || src->size==0)
        return;
    
    dest->end->next = src->start;
    dest->end = src->end;

    dest->size += src->size;
}