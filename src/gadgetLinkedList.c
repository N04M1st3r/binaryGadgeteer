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
    if(next != NULL){
        next->refCnt += 1;
    }
    
    return res;
}


/**
 * Creates a gadgetNode.
 * 
 * @param instNode the start instruction.
 * 
 * @return An allocated GadgetNode *, if error then NULL.
 * 
 * @note this is using malloc, remember to free with gadgetNodeFreeCurrentAll or..
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
 * Creates a GadgetLL * where its next is nextGadget_p->first and all its details are from decodedInstruction_p, and nextGadget_p.
 * 
 * @param decodedInstruction_p decoded instruction to copy from the instruction things.
 * @param nextGadget_p the gadget that made this gadget, the one after it when executing.
 * 
 * @return GadgetLL * as stated in the description, if error NULL.
 * 
 * @note Mallocing, free with the free functions for GadgetNode when done.
*/
GadgetNode *GadgetNodeCreateFromDecodedInstAndNextGadget(ZydisDecodedInstruction *decodedInstruction_p, GadgetNode *nextGadget_p, char *bufferDecode, ZyanU64 runtime_address){
    MiniInstructionNode *newMiniInst = MiniInstructionNodeCreate(decodedInstruction_p->mnemonic, decodedInstruction_p->length, bufferDecode, nextGadget_p->first);
    if (newMiniInst == NULL){
        err("Error in GadgetNodeCreateFromDecodedInstAndNextGadget while calling MiniInstructionNodeCreate.");
        return NULL;
    }

    GadgetNode *newGadgetNode = GadgetNodeCreate(newMiniInst, nextGadget_p->addr_file - decodedInstruction_p->length, runtime_address);
    if (newGadgetNode == NULL){
        err("Error in GadgetNodeCreateFromDecodedInstAndNextGadget while calling GadgetNodeCreate");
        MiniInstructionNodeFree(newMiniInst);
        return NULL;
    }

    return newGadgetNode;
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
        gadgetNodeFreeCurrentAll(cur);
        cur = next;
    }
    
    free(gadgetLL);
}


/**
 * Removes/frees all the gadgets the satisfy the certin condition.
 * 
 * @param gadgetsLL gadgets link list pointer.
 * @param checkCondition function pointer to a function that gets GadgetNode * and returns true/false, it will give this function GadgetNode * and 
 *                       it will need to return true if to show that GadgetNode, else false.
*/
void GadgetLLFreeBasedCondition(GadgetLL *gadgetLL, bool (*checkCondition)(GadgetNode *)){
    GadgetNode *cur = gadgetLL->start;
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    
    ZyanU8 *bufferDecode;
    ZyanU64 runtime_address;
    ZyanU8 instLength;
    MiniInstructionNode *curInstNode;


}


/**
 * Frees only the gadgetLL.
 * 
 * @note assuming gadgetLL != NULL.
*/
void gadgetLLFreeOnly(GadgetLL *gadgetLL){
    free(gadgetLL);
}


/**
 * Frees gadgetNode in a safe way.
 * 
 * @note assuming gadgetNode != NULL.
 * @note not sure there is a use for this but why not.
*/
void gadgetNodeFreeCurrentOnly(GadgetNode *gadgetNode){
    free(gadgetNode);
}

/**
 * Frees gadgetNode in a safe way.
 * 
 * @note assuming gadgetNode != NULL and gadgetNode->first != NULL.
 * @note not freeing next.
*/
void gadgetNodeFreeCurrentAll(GadgetNode *gadgetNode){
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
 * Combining src to dest, like merge.
 * 
 * @param dest the gadgetLL * that will be combined into.
 * @param src the gadgetLL * that will be "taken" from.
 * 
 * @note dest will be changed, but src will not be changed, although the variables inside them are the same.
 * @note assuming dest!=NULL.
 * @note asuuming there will not be integer overflow while combining the sizes.
 * @note if you are no longer using src, run gadgetLLFreeOnly on it.
*/
void GadgetLLCombine(GadgetLL *dest, GadgetLL *src){
    if(src == NULL || src->size==0)
        return;
    
    dest->end->next = src->start;
    dest->end = src->end;

    dest->size += src->size;
}


/**
 * Adding curGadgetNode to gadgetsLL.
 * 
 * @param gadgetsLL The gadgets linked list.
 * @param curGadgetNode the GadgetNode* to add. 
*/
void GadgetLLAddGadgetNode(GadgetLL *gadgetsLL, GadgetNode *curGadgetNode){
    gadgetsLL->end->next = curGadgetNode;
    gadgetsLL->end = curGadgetNode;
    (gadgetsLL->size)++;
}
