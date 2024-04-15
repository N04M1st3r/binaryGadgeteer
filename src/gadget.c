#include "gadget.h"

#include "costumErrors.h"


#include <zydis/Decoder.h>
#include <zydis/Formatter.h>

#include <string.h>

static ZydisDecoder *decoder_p;
static ZydisFormatter *formatter_p;

static gadgetGeneralNode *_expandInstructionDownR(char *buffer, size_t bufferSize, size_t bufferInstructionOffset, gadgetGeneralNode *fatherGadgetGeneralNode, ZydisDecodedInstruction *decodedInstructionStart_p, size_t depth);

/**
 * Creating gadgetGeneralNode and initilizing it with gadgetG.
 * 
 * @param gadgetG gadgetGeneral to put inside gadgetGeneralNode.
 * 
 * @return an allocated gadgetGeneralNode * and initilized(with null). on Error returning NULL
 * 
 * @note THIS IS MALLOCED, CALL gadgetGeneralNodeCleanAll when done!!
*/
gadgetGeneralNode *gadgetGeneralNodeCreate(gadgetGeneral gadgetG){
    gadgetGeneralNode *gadgetGeneralNode_p = (gadgetGeneralNode *) malloc(sizeof(gadgetGeneralNode));
    if(gadgetGeneralNode_p == NULL){
        err("Error inside gadgetGeneralNodeCreate while on Malloc. of size %zu\n", sizeof(gadgetGeneralNode));
        return (gadgetGeneralNode *) NULL;
    }

    memcpy(&(gadgetGeneralNode_p->gadgetG), &gadgetG, sizeof(gadgetGeneral));
    gadgetGeneralNode_p->next = NULL;

    return gadgetGeneralNode_p;
}

/**
 * Creating gadgetGeneral with the provided items, and mallocing space for firstGadget.
 * 
 * @param instruction instruction to be the first. (going to copy this)
 * @param vaddr virturl address to the location of the first gadget.
 * @param addr_file location in the elf file of the first gadget.
 * 
 * @return an initilized gadgetGeneral. if error gadgetG.first = NULL.
 * 
 * @note Mallocing space for firstGadget, remember to get rid of it! (remember to call gadgetFreeAll on the gadget)
*/
gadgetGeneral gadgetGeneralCreateWithInstruction(miniInstruction instruction, uint64_t vaddr, uint64_t addr_file){
    gadgetGeneral gadgetG;

    gadgetG.first = malloc(sizeof(gadget));
    if (gadgetG.first == NULL){
        err("Error inside Malloc in gadgetGeneralCreate, size %zu.", sizeof(gadget));
        return gadgetG; //gadgetG.first = NULL
    }
    memcpy(&(gadgetG.first->minInstruction), &instruction, sizeof(miniInstruction));

    gadgetG.first->refrences = 0;
    gadgetG.first->next = NULL;


    gadgetG.length = 1;
    gadgetG.vaddr = vaddr;
    gadgetG.addr_file = addr_file;
    gadgetG.checked = false;
    return gadgetG;
}


/**
 * Creating gadgetGeneral with the provided items, and mallocing space for firstGadget.
 * 
 * @param firstGadget gadget to be the first. (going to copy this)
 * @param vaddr virturl address to the location of the first gadget.
 * @param addr_file location in the elf file of the first gadget.
 * 
 * @return an initilized gadgetGeneral. if error NULL in gadgetG.first.
 * 
 * 
 * @note Mallocing space for firstGadget, remember to get rid of it! (remember to call gadgetFreeAll on the gadget)
*/
gadgetGeneral gadgetGeneralCreateWithGadget(gadget firstGadget, uint64_t vaddr, uint64_t addr_file){
    gadgetGeneral gadgetG;

    gadgetG.first = malloc(sizeof(gadget));
    if (gadgetG.first == NULL){
        err("Error inside Malloc in gadgetGeneralCreate, size %zu.", sizeof(gadget));
        return gadgetG; //there is NULL inside gadgetG.first.
    }
    memcpy(gadgetG.first, &firstGadget, sizeof(gadget));

    gadgetG.length = 1;
    gadgetG.vaddr = vaddr;
    gadgetG.addr_file = addr_file;
    gadgetG.checked = false;
    return gadgetG;
}


/**
 * Frees gadget and the gadgets its pointing to.
 * (decreasing the refrence counter)
 * 
 * @param gadget_p gadget to start freeing from
 * 
*/
void gadgetFreeAll(gadget *gadget_p){
    gadget *tmp;
    while(gadget_p != NULL && gadget_p->refrences == 0){
        tmp = gadget_p->next;
        free(gadget_p);
        gadget_p = tmp;
        (gadget_p->refrences)--;
    }
}


/**
 * Frees gadgetGeneralNode * and everything in it. except for the next one.
 * 
 * @param gadgetGeneralNode_p the gadgetGeneralNode* to free.
 * 
 * @note Remeber not to use it after freed!
*/
void gadgetGeneralNodeFreeCurrent(gadgetGeneralNode *gadgetGeneralNode_p){
    if (gadgetGeneralNode_p != NULL){
        if(gadgetGeneralNode_p->gadgetG.first != NULL){
            gadgetFreeAll(gadgetGeneralNode_p->gadgetG.first);
            gadgetGeneralNode_p->gadgetG.first = NULL; //just as a fail safe
        }
        free(gadgetGeneralNode_p);
    }
}


/**
 * Frees gadgetGeneralNode * and everything in it. including for the next one and the whole chain.
 * 
 * @param gadgetGeneralNode_p, the gadgetGeneralNode* to free.
*/
void gadgetGeneralNodeFreeAll(gadgetGeneralNode *gadgetGeneralNode_p){
    gadgetGeneralNode *tmp;
    while (gadgetGeneralNode_p != NULL){
        tmp = gadgetGeneralNode_p->next;
        gadgetGeneralNodeFreeCurrent(gadgetGeneralNode_p);
        gadgetGeneralNode_p = tmp;
    }
}

/**
 * Append Gadget General node to another general gadget node.
 * 
 * @param dest the destination of where to copy to (to the next)
 * @param src the src of where to copy from.
 * 
 * @return 0 if successfull. otherwise error.
 * 
 * @note dest->next needs to be NULL!
*/
int gadgetGeneralNodeAppendGadgetGeneralNode(gadgetGeneralNode *dest, gadgetGeneralNode *src){
    if(dest->next != NULL){
        err("Cant have not NULL inside dest->next inside gadgetGeneralNodeAppendGadgetGeneralNode");
        return 1;
    }
    dest->next = src;
    return 0;
}


/**
 * Adds mini instruction to gadget inside Gadget General Node.
 * 
 * @param gadgetGeneralNode_p to add into.
 * @param instruction mini instruction to add to the gadget.
 * 
 * @return 0 if success. otherwise(-1, ...) error.
 * 
 * @note Mallocing here.
*/
int gadgetGeneralNodeAddInstruction(gadgetGeneralNode *gadgetGeneralNode_p, miniInstruction instruction){
    gadget *gadgetAdd = (gadget *) malloc(sizeof(gadget*));
    if (gadgetAdd == NULL){
        err("Error in malloc for gadgetAdd, in gadgetGeneralNodeAddInstruction of size %zu.", sizeof(gadget *));
        return -1;
    }
    
    memcpy(&(gadgetAdd->minInstruction), &instruction, sizeof(miniInstruction));

    gadgetAdd->next = gadgetGeneralNode_p->gadgetG.first;
    
    gadgetAdd->refrences = 1;

    gadgetGeneralNode_p->gadgetG.first = gadgetAdd->next;

    (gadgetGeneralNode_p->gadgetG.length)++;
    gadgetGeneralNode_p->gadgetG.vaddr     -= instruction.instructionLength;
    gadgetGeneralNode_p->gadgetG.addr_file -= instruction.instructionLength;

    return 0;    
}


/**
 * Creating gadgetGeneralNode* and expanding it as demanded.
 * 
 * @param buffer the buffer it was read from
 * @param vuf_vaddr the buffer virtual address.
 * @param buf_fileOffset the buffer offset inside the file.
 * @param bufferSize the size of the buffer.
 * @param branchInstructionLoc FoundLocationsBufferNode* is a pointer to the branch instruction found. 
 * @param branchInstructionMnemonic the ZydisMnemonic of the instruction (can calc it here maybe?) ZYDIS_MNEMONIC_RET for example.
 * 
 * @return gadgetGeneralNode * with all the gadgets in it. NULL if Error.
 * 
 * 
 * @note THIS IS MALLOCED, REMEBER TO FREE WITH gadgetGeneralNodeFreeAll!
*/
gadgetGeneralNode *expandInstructionDown(char *buffer, uint64_t buf_vaddr, uint64_t buf_fileOffset, size_t bufferSize, FoundLocationsBufferNode *branchInstructionLoc, ZydisMnemonic branchInstructionMnemonic, size_t depth){
    if (depth == 0)
        return (gadgetGeneralNode *) NULL;
    
    depth--;
    
    miniInstruction branchMiniInstruction;
    branchMiniInstruction.instructionLength = branchInstructionLoc->miniInstructionInfo.mnemonicOpcodeSize + branchInstructionLoc->miniInstructionInfo.additionSize;
    
    if(branchInstructionLoc->offset + branchMiniInstruction.instructionLength > bufferSize){
        err("Too big inside expandInstructionDown on check :(");
        return (gadgetGeneralNode *) NULL;
    }
    memcpy(branchMiniInstruction.instructionFullOpcode, buffer + branchInstructionLoc->offset, branchMiniInstruction.instructionLength);
    branchMiniInstruction.mnemonic = branchInstructionMnemonic;

    gadgetGeneral branchGadgetG = gadgetGeneralCreateWithInstruction(branchMiniInstruction, buf_vaddr + branchInstructionLoc->offset, buf_fileOffset + branchInstructionLoc->offset);
    if(branchGadgetG.first == NULL){
        err("Error inside expandInstructionDown on gadgetGeneralCreateWithInstruction for branchGadgetG.");
        return (gadgetGeneralNode *) NULL;
    }

    gadgetGeneralNode *resultGeneralNode = gadgetGeneralNodeCreate(branchGadgetG);
    if (resultGeneralNode == NULL){
        gadgetFreeAll(branchGadgetG.first);
        err("Error inside expandInstructionDown on gadgetGeneralNodeCreate for resultGeneralNode.");
        return (gadgetGeneralNode *) NULL;
    }

    //now the cool stuff :)

    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    char decodedBuffer[256];

    char *bufferDecode = buffer + branchInstructionLoc->offset; //already checked, it is not less than or more than buffer


    ZyanU64 runtime_address = resultGeneralNode->gadgetG.vaddr;

    runtime_address--; bufferDecode--; //because staring at 1.

    gadgetGeneralNode *lastGeneralNode = resultGeneralNode;

    for(size_t i = 1; i <= ZYDIS_MAX_INSTRUCTION_LENGTH; i++, runtime_address--, bufferDecode--){
        if(bufferDecode < buffer){
            err("Cant Go that down! in expandInstructionDown");
            break;
        }                                                               //can maybe pass here just i? (branchMiniInstruction.instructionLength + i) instead of i
        if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(decoder_p, bufferDecode, i , &decodedInstruction, operands))){
                        //Cant decode this, no such instruction.
                        continue;
                    }
        
        /*if (decodedInstruction.length < i) //not including prefixes in this.
            continue; //maybe break;? no becuase there can be mini instructions inside big instructions. 
        if(decodedInstruction.length > i)
            continue;*/
        if (decodedInstruction.length != i)
            continue;
        
        


        lastGeneralNode->next = _expandInstructionDownR(buffer, bufferSize, bufferDecode-buffer, resultGeneralNode ,&decodedInstruction, depth);
        if(lastGeneralNode->next != NULL){
            //there is actually stuff there. / no error
            lastGeneralNode = lastGeneralNode->next;
        }
        
        //TODO: there is a critical error, it does not remeber depth 2?


        ZydisFormatterFormatInstruction(formatter_p, &decodedInstruction, operands,
                decodedInstruction.operand_count_visible, decodedBuffer, sizeof(decodedBuffer), runtime_address, ZYAN_NULL);

        printf("0x%" PRIx64 ": %s on depth %zu\n", runtime_address ,decodedBuffer, depth);

        printf("%ld showing for i=%ld:\n", depth, i);
        gadgetGeneralNodeShowAll(resultGeneralNode);
        printf("END SHOW\n");
        //runtime_address += instruction.length;
    }
    resultGeneralNode->gadgetG.checked = true;

    return resultGeneralNode;
}

/**
 * Creating gadgetGeneralNode* and expanding it as demanded. recursive
 * this is an inner function that expandInstructionDown is calling, do not call it yourself!
 * 
 * @param buffer the original buffer that was read.
 * @param bufferSize the original buffer size.
 * @param bufferInstructionOffset the location of the current instruction inside the buffer.
 * @param decodedInstructionStart_p the decoded instruction pointer
 * @param depth The depth you want to search of, when reaches 0 stops.
 * 
 * @note USING MALLOC HERE! remeber to free!! with a special function.
*/
static gadgetGeneralNode *_expandInstructionDownR(char *buffer, size_t bufferSize, size_t bufferInstructionOffset, gadgetGeneralNode *fatherGadgetGeneralNode, ZydisDecodedInstruction *decodedInstructionStart_p, size_t depth){
    
    
    gadget firstGadget = {.next=fatherGadgetGeneralNode->gadgetG.first, .refrences=0};
    //later I will increase the refrences (after I see there are no errors.)
    
    firstGadget.minInstruction.instructionLength = decodedInstructionStart_p->length;
    firstGadget.minInstruction.mnemonic = decodedInstructionStart_p->mnemonic;
    memcpy(firstGadget.minInstruction.instructionFullOpcode, buffer + bufferInstructionOffset, decodedInstructionStart_p->length);

    gadgetGeneral branchGadgetG = gadgetGeneralCreateWithGadget(firstGadget, fatherGadgetGeneralNode->gadgetG.vaddr - decodedInstructionStart_p->length,  fatherGadgetGeneralNode->gadgetG.addr_file - decodedInstructionStart_p->length);
    if(branchGadgetG.first == NULL){
        err("Error inside _expandInstructionDownR on gadgetGeneralCreateWithGadget for branchGadgetG.");
        return (gadgetGeneralNode *) NULL;
    }

    gadgetGeneralNode *resultGeneralNode = gadgetGeneralNodeCreate(branchGadgetG);
    if (resultGeneralNode == NULL){
        gadgetFreeAll(branchGadgetG.first);
        err("Error inside _expandInstructionDownR on gadgetGeneralNodeCreate for resultGeneralNode.");
        return (gadgetGeneralNode *) NULL;
    }
    

    (fatherGadgetGeneralNode->gadgetG.first->refrences)++;

    resultGeneralNode->gadgetG.length = fatherGadgetGeneralNode->gadgetG.length + 1;

    if(!depth--)
        return (gadgetGeneralNode *) resultGeneralNode;

    //TODO make a function to do it from here or something... because it is the same as expandInstructionDown.
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    char decodedBuffer[256];


    char *bufferDecode = buffer + bufferInstructionOffset; //already checked in expandInstructionDown its not more than buffer

    
    ZyanU64 runtime_address = resultGeneralNode->gadgetG.vaddr;

    ZyanUSize StartInstructionLength = resultGeneralNode->gadgetG.first->minInstruction.instructionLength;

    runtime_address--; bufferDecode--; //because starting at i=1.

    gadgetGeneralNode *lastGeneralNode = resultGeneralNode;

    
    for(size_t i = 1; i <= ZYDIS_MAX_INSTRUCTION_LENGTH; i++, runtime_address--, bufferDecode--){
        if(bufferDecode < buffer){
            err("Cant Go that down! in _expandInstructionDownR");
            break;
        }                                                               //can maybe pass here just i? (StartInstructionLength + i) instead of i
        if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(decoder_p, bufferDecode, i, &decodedInstruction, operands))){
                        //Cant decode this, no such instruction.
                        continue;
                    }
        
        /*if (decodedInstruction.length < i) //not including prefixes in this.
            continue; //maybe break;? no becuase there can be mini instructions inside big instructions. 
        if(decodedInstruction.length > i)
            continue;*/

        if (decodedInstruction.length != i)
            continue;
        
        lastGeneralNode->next = _expandInstructionDownR(buffer, bufferSize, bufferDecode-buffer, resultGeneralNode ,&decodedInstruction, depth);
        if(lastGeneralNode->next != NULL){
            //there is actually stuff there. / no error
            printf("ADDING");
            lastGeneralNode = lastGeneralNode->next;
        }
        
            
        //now this is a problem, if I will make the next one the next of this one then what about the others?
        //and if I will make the prev one so that his next is this, how could I detect it.
        //I can solve it by saving the last one on the list.
        //problem solved.
        
        ZydisFormatterFormatInstruction(formatter_p, &decodedInstruction, operands,
                decodedInstruction.operand_count_visible, decodedBuffer, sizeof(decodedBuffer), runtime_address, ZYAN_NULL);

        printf("0x%" PRIx64 ": %s on depth %zu\n", runtime_address ,decodedBuffer, depth);

        printf("main showing for i=%ld:\n", i);
        gadgetGeneralNodeShowAll(resultGeneralNode);
        printf("END SHOW\n");
        gadgetGeneralNodeShowAll(lastGeneralNode);
        printf("END SHOW2\n\n");
        
        

    }

    resultGeneralNode->gadgetG.checked = true;

    return resultGeneralNode;
}

/**
 * Init decoder_p and formatter_p.
 * 
 * @param decoder_given_p Initilized ZydisDecoder pointer.
 * @param formatter_given_p Initilized ZydisFormatter pointer.
 * 
*/
void initDecoderAndFormatter(ZydisDecoder *decoder_given_p, ZydisFormatter *formatter_given_p){
    /*
    ZydisDecoder decoder; //decoder
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, arch_p->machine_mode, arch_p->stack_width))){
        err("some error initiating decoder.");
        return 4;
    }
    
    //the style of the assembly:
    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    */
   decoder_p = decoder_given_p;
   formatter_p = formatter_given_p;
}


/**
 * Shows all the gadgets inside it, decoding each one seperatly.
 * 
 * @param gadgetGNode_p gadgetGeneralNode* to show.
 * 
 * 
 * TODO: split to more functions.
*/
void gadgetGeneralNodeShowAll(gadgetGeneralNode *gadgetGNode_p){
    ZydisDecodedInstruction decodedInstruction;
    gadget *curGadget_p;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    char decodedBuffer[256];

    for(;gadgetGNode_p != NULL; gadgetGNode_p = gadgetGNode_p->next){
        printf("0x%" PRIx64 ": ", gadgetGNode_p->gadgetG.vaddr);
        ZyanU64 runtime_address = gadgetGNode_p->gadgetG.vaddr;
        //gadgetGNode_p->gadgetG.first->minInstruction.instructionLength
        curGadget_p = gadgetGNode_p->gadgetG.first;
        for(; curGadget_p != NULL; runtime_address += curGadget_p->minInstruction.instructionLength , curGadget_p = curGadget_p->next){
            //can also decode this in one go and just give the length of all the instructions.
            
            if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(decoder_p, curGadget_p->minInstruction.instructionFullOpcode, curGadget_p->minInstruction.instructionLength , &decodedInstruction, operands))){
                        //Cant decode this, no such instruction.
                        continue;
                    }

            //passing 0 in runtime address because I don't care about it, later care about it
            //TODO care about runtime_address and don't just pass 0.

            ZydisFormatterFormatInstruction(formatter_p, &decodedInstruction, operands,
                decodedInstruction.operand_count_visible, decodedBuffer, sizeof(decodedBuffer), runtime_address, ZYAN_NULL);

            printf("%s", decodedBuffer);
            if(curGadget_p->next != NULL)
                printf(" -> ");
        }
        printf("\n");

    }
}

/**
 * Shows all the gadgets inside it, decoding them together.
 * 
 * @param gadgetGNode_p gadgetGeneralNode* to show.
 * 
*/
void gadgetGeneralNodeShowAllCombined(gadgetGeneralNode *gadgetGNode_p){
    //this does it in assumption they are all glued together.
}