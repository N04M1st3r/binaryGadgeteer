#include "gadget2.h"
#include "costumErrors.h"





/**
 * Expands currentGadgets to have more gadgets, will go down depth number layers.
 * 
 * @param buffer the buffer it was read from
 * @param buf_vaddr the buffer virtual address.
 * @param buf_fileOffset the buffer offset inside the file.
 * @param bufferSize the size of the buffer.
 * @param currentGadgets the linked list of the current gadgets.
 * @param depth the max amount to go down.
 * 
 * @return  GadgetLL * the gadget linked list, allocated so free! with gadgetLLFreeAll or...
 * 
 * @note mallocing onto gadgetsLL, will change it.
*/
GadgetLL *expandGadgetsDown(char *buffer, uint64_t buf_vaddr, uint64_t buf_fileOffset, size_t bufferSize, GadgetLL *gadgetsLL, uint8_t depth){
    if(depth == 0)
        return NULL;
    
    if(gadgetsLL->start == NULL)
        return NULL;

    //going through all the gadgets:
    GadgetNode fakeNode = {.next=NULL};
    GadgetLL *curLevelGadgetLL = gadgetLLCreate(&fakeNode);

    //creating this here so I would have the option to go multithread.
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    

    for(GadgetNode *curGadget = gadgetsLL->start; curGadget != NULL; curGadget = curGadget->next){

        #define OffsetInBuffer curGadget->addr_file - buf_fileOffset;
        char *bufferDecode = buffer + OffsetInBuffer;

        ZyanU64 runtime_address = curGadget->vaddr;

        for(size_t i = 1; i <= ZYDIS_MAX_INSTRUCTION_LENGTH; i++, runtime_address--, bufferDecode--){
            if(bufferDecode < buffer){
                err("Cant Go that down! in expandGadgetsDown. tried: %p   where buffer: %p   at vaddr: 0x%" PRIx64 "\n", bufferDecode ,buffer ,runtime_address);
                break;
            }                                                               //can maybe pass here just i? (branchMiniInstruction.instructionLength + i) instead of i
            if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bufferDecode, i, &decodedInstruction, operands))){
                            //Cant decode this, no such instruction.
                            continue;
                        }
            /*if (decodedInstruction.length < i) //not including prefixes in this.
                continue; //maybe break;? no becuase there can be mini instructions inside big instructions. 
            if(decodedInstruction.length > i)
                continue;*/
            if (decodedInstruction.length != i)
                continue;
            
            GadgetNode *newGadgetNode = GadgetNodeCreateFromDecodedInstAndNextGadget(&decodedInstruction, curGadget, bufferDecode, runtime_address);
            if ( newGadgetNode == NULL ){
                err("Error in expandGadgetsDown while calling GadgetNodeCreateFromDecodedInstAndNextGadget.");

                ////getting rid of the first fake
                curLevelGadgetLL->start = curLevelGadgetLL->start->next;
                if(curLevelGadgetLL->end == &fakeNode)
                    curLevelGadgetLL->end = NULL;
                //freeing everyone because error.
                gadgetLLFreeAll(curLevelGadgetLL);
                return NULL;
            }

            GadgetLLAddGadgetNode(curLevelGadgetLL, newGadgetNode);
        }
    }

    //getting rid of the first fake
    curLevelGadgetLL->start = curLevelGadgetLL->start->next;
    (curLevelGadgetLL->size)--;
    if(curLevelGadgetLL->end == &fakeNode)
        curLevelGadgetLL->end = NULL;

    GadgetLL *nextLevelGadgetLL = expandGadgetsDown(buffer, buf_vaddr, buf_fileOffset, bufferSize, curLevelGadgetLL, depth-1);

    GadgetLLCombine(curLevelGadgetLL, nextLevelGadgetLL);
    gadgetLLFreeOnly(nextLevelGadgetLL);

    return curLevelGadgetLL;
}




/**
 * Initilizes decoder and formatter
 * 
 * HAS TO BE CALLED FIRST!
 * 
 * @return 0 if success, anything else on error.
*/
int initDecoderAndFormatter(ArchInfo *arch_p){
    
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, arch_p->machine_mode, arch_p->stack_width))){
        err("Error Initiating decoder, at initDecoderAndFormatter with ZydisDecoderInit.");
        return 1;
    }
    
    //the style of the assembly:
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL))){
        err("Error Initiating formatter, at initDecoderAndFormatter with ZydisFormatterInit.");
        return 2;
    }
    return 0;
}