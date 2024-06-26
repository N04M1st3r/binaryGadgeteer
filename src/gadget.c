#include "gadget.h"
#include "costumErrors.h"

#define UNUSED(x) (void)(x)

static ZydisFormatter formatter;
static ZydisDecoder decoder;

static bool IsMnemonicBranchNotConditionBranch(ZydisMnemonic mnemonic);

static bool alwaysTrueCondition(GadgetNode *curGadget);
static bool onlyEndsCondition(GadgetNode *curGadget);
static bool onlyPrintableAddressCondition(GadgetNode *curGadget);
static bool onlyPrintableAddressConditionEnds(GadgetNode *curGadget);

static bool isCharPrintable(char a);

static void _printRegsStatistics(GadgetLL *gadgetsLL, SearchInfo *regsSearch, size_t searchesLen);

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
 * @note this is a recursive fuction, the depth is depnding on `depth`
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

        // OffsetInBuffer is curGadget->addr_file - buf_fileOffset;
        char *bufferDecode = buffer + (curGadget->addr_file - buf_fileOffset);

        ZyanU64 runtime_address = curGadget->vaddr;

        for(size_t i = 1; i <= ZYDIS_MAX_INSTRUCTION_LENGTH; i++, runtime_address--, bufferDecode--){
            if(bufferDecode < buffer){
                //err("Cant Go that down! in expandGadgetsDown. tried: %p   where buffer: %p   at vaddr: 0x%" PRIx64 "\n", bufferDecode ,buffer ,runtime_address);
                //cant Go that down, it may be fine because execute can't be there prob.
                break;
            }                     

            if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bufferDecode, i, &decodedInstruction, operands))){
                            //Cant decode this, no such instruction.
                            continue;
                        }
            
            if (decodedInstruction.length != i-1)
                continue;

            if(IsMnemonicBranchNotConditionBranch(decodedInstruction.mnemonic)){
                continue;
            }
            
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

            if(runtime_address == 0x000000000040126d)
                printf("added?\n");

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
 * Checks if the mnemonic given is a branch instruction (not includign jcc, conditional jump instructions)
 * if it is returns true, else false.
 * 
 * @param mnemonic The instruction mnemonic.
 *  
 * @return true if the instruction is a branch instruction(not conditional), else false.
*/
static bool IsMnemonicBranchNotConditionBranch(ZydisMnemonic mnemonic){
    //IRET maybe also?
    switch(mnemonic){
        case ZYDIS_MNEMONIC_RET: 
        case ZYDIS_MNEMONIC_JMP:
        case ZYDIS_MNEMONIC_CALL:
            return true; 
        default:
            return false;
    }
    return false;
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



/**
 * helper for GadgetLLShowAll,
 * always returns true, for GadgetLLShowAll when calling GadgetLLShowBasedCondition.
 * 
 * @param curGadget, gadget node pointer.
 * 
 * @return true
*/
static bool alwaysTrueCondition(GadgetNode *curGadget){
    UNUSED(curGadget);
    return true;
}

/**
 * Shows all the gadgets inside it, decoding each one seperatly.
 * 
 * @param gadgetsLL gadgets link list pointer.
*/
void GadgetLLShowAll(GadgetLL *gadgetsLL){
    GadgetLLShowBasedCondition(gadgetsLL, alwaysTrueCondition);
}

/**
 * Helper for GadgetLLShowOnlyEnds,
 * shows only the ends by looking at the refCnt.
 * 
 * @param curGadget The current gadget.
 * 
 * @return ture if it has noone pointing at it (an end, refCnt==1), false otherwise.
*/
static bool onlyEndsCondition(GadgetNode *curGadget){
    return curGadget->first->refCnt == 1;
}

/**
 * Shows all the gadgets inside it, decoding each one seperatly,
 * shows only the ends.
 * doing so by detecting ends by references 1.
 * 
 * @param gadgetsLL gadgets link list pointer.
*/
void GadgetLLShowOnlyEnds(GadgetLL *gadgetsLL){
    GadgetLLShowBasedCondition(gadgetsLL, onlyEndsCondition);
}

/**
 * Checks if a character is printable (in ascii table).
 * 
 * @param c the chatacter.
 * 
 * @return True if the a is printable, False otherwise.
 * @cite https://www.asciitable.com/
*/
static bool isCharPrintable(char a){
    return a >= 0x20 && a <= 0x7E;
}

/**
 * Helper for GadgetLLShowOnlyPrintableAddress.
 * shows only the printable addresses.
 * 
 * @param curGadget The current gadget.
 * 
 * @return true if it has only printable, false otherwise.
*/
static bool onlyPrintableAddressCondition(GadgetNode *curGadget){
    uint8_t width = 4;
    uint64_t address = curGadget->vaddr;
    char currentChar;

    if(decoder.machine_mode == ZYDIS_MACHINE_MODE_LONG_64){
        width = 8;
    }
    for(; width!=0; width--){
        currentChar = (char) (address & 0xff);
        if (isCharPrintable(currentChar))
            return false;
        address >>= 8;
    }
    
    return true;
}


/**
 * Helper for GadgetLLShowOnlyPrintableAddress.
 * shows only the printable addresses or NULL.
 * or null meaning null in the end.
 * 
 * @param curGadget The current gadget.
 * 
 * @return true if it has only printable and ends with null, false otherwise.
*/
static bool onlyPrintableAddressOrNullConditionEnds(GadgetNode *curGadget){
    uint8_t width = 4;
    uint64_t address = curGadget->vaddr;
    char currentChar;

    if(decoder.machine_mode == ZYDIS_MACHINE_MODE_LONG_64){
        width = 8;
    }
    for(; width != 0; width--){
        currentChar = (char) (address & 0xff);
        if (isCharPrintable(currentChar))
            break;
        address >>= 8;
    }

    //now for what is left it has to be NULL! or in other words the adderss left has to be 0
    return address == 0;
}

/**
 * Helper for GadgetLLShowOnlyPrintableAddressEnds
 * shows only the printable addresses which are ends.
 * 
 * @param curGadget The current gadget.
 * 
 * @return true if it has only printable and is an end, false otherwise.
*/
static bool onlyPrintableAddressConditionEnds(GadgetNode *curGadget){
    return onlyEndsCondition(curGadget) && onlyPrintableAddressCondition(curGadget);
}

/**
 * Shows all the gadgets that are printable.
 * 
 * @param gadgetsLL gadgets link list pointer.
*/
void GadgetLLShowOnlyPrintableAddress(GadgetLL *gadgetsLL){
    GadgetLLShowBasedCondition(gadgetsLL, onlyPrintableAddressCondition);
}

/**
 * Shows all the gadgets that are printable, but only the ends.
 * 
 * @param gadgetsLL gadgets link list pointer.
*/
void GadgetLLShowOnlyPrintableAddressEnds(GadgetLL *gadgetsLL){
    GadgetLLShowBasedCondition(gadgetsLL, onlyPrintableAddressConditionEnds);
}

/**
 * Shows all the gadgets that are printable or null, but only the ends.
 * 
 * or null meaning null in the end.
 * 
 * @param gadgetsLL gadgets link list pointer.
*/
void GadgetLLShowOnlyPrintableOrNullAddress(GadgetLL *gadgetsLL){
    GadgetLLShowBasedCondition(gadgetsLL, onlyPrintableAddressOrNullConditionEnds);
}



/**
 * Show all the gadgets that check the condition, return true to the condition.
 * 
 * @param gadgetsLL gadgets link list pointer.
 * @param checkCondition function pointer to a function that gets GadgetNode * and returns true/false, it will give this function GadgetNode * and 
 *                       it will need to return true if to show that GadgetNode, else false.
*/
void GadgetLLShowBasedCondition(GadgetLL *gadgetsLL, bool (*checkCondition)(GadgetNode *)){
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    char decodedBuffer[256];

    ZyanU8 *bufferDecode;
    ZyanU64 runtime_address;
    ZyanU8 instLength;
    MiniInstructionNode *curInstNode;

    for(GadgetNode *curGadget = gadgetsLL->start; curGadget != NULL; curGadget = curGadget->next){
        if(!checkCondition(curGadget))
                continue;

        runtime_address = curGadget->vaddr;

        printf("0x%" PRIx64 ": ", runtime_address);
        for(curInstNode = curGadget->first; curInstNode != NULL; runtime_address += instLength, curInstNode = curInstNode->next){
            bufferDecode = curInstNode->miniInst.instructionFullOpcode;
            instLength = curInstNode->miniInst.instructionLength;

            if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bufferDecode, instLength, &decodedInstruction, operands))){
                            //Cant decode this, no such instruction.
                            err("NO INSTRUCTION, HOW DID IT GET HERE inside GadgetLLShowBasedCondition at ZydisDecoderDecodeFull?");
                            /*for(int i=0; i<instLength; i++){
                                printf("%hhx ", bufferDecode[i]);
                            }*/
                            exit(123);
                            continue;
                        }
            

            ZydisFormatterFormatInstruction(&formatter, &decodedInstruction, operands,
                decodedInstruction.operand_count_visible, decodedBuffer, sizeof(decodedBuffer), runtime_address, ZYAN_NULL);
            //(passing ZYAN_NULL in user_data)

            printf("%s", decodedBuffer);
            if(curInstNode->next != NULL)
                printf(" -> ");

        }
        printf("\n");

        
    }
}



/**
 * Prints all the register statistics, both for x86 and x64.
 * 
 * @param gadgetsLL gadgets link list pointer.
 * @param is64bit   True if 64 bit, false otherwise.
 */
void printRegsStatistics(GadgetLL *gadgetsLL, bool is64bit){
    if(is64bit){
        SearchInfo regsSearch[] = {
            {"pop rax", UINT64_MAX},
            {"pop rbx", UINT64_MAX},
            {"pop rcx", UINT64_MAX},
            {"pop rdx", UINT64_MAX},
            {"pop rsi", UINT64_MAX},
            {"pop rdi", UINT64_MAX},
            {"pop rbp", UINT64_MAX},
            {"pop rsp", UINT64_MAX}
        }; //can add r10,11,12,13,...15 but no one uses those usually.
        _printRegsStatistics(gadgetsLL, regsSearch, sizeof(regsSearch)/sizeof(SearchInfo));
    }else{
        SearchInfo regsSearch[] = {
            {"pop eax", UINT64_MAX},
            {"pop ebx", UINT64_MAX},
            {"pop ecx", UINT64_MAX},
            {"pop edx", UINT64_MAX},
            {"pop esi", UINT64_MAX},
            {"pop edi", UINT64_MAX},
            {"pop ebp", UINT64_MAX},
            {"pop esp", UINT64_MAX}
        };
        _printRegsStatistics(gadgetsLL, regsSearch, sizeof(regsSearch)/sizeof(SearchInfo));
    }
}

/**
 * Internal function to print the registers statistics
 * 
 * @param 
 */
static void _printRegsStatistics(GadgetLL *gadgetsLL, SearchInfo *regsSearch, size_t searchesLen){
    getSearchInfoArr(gadgetsLL, regsSearch, searchesLen);
    size_t counter = 0;
    for(size_t i=0; i<searchesLen; i++){
        if(regsSearch[i].vaddr != UINT64_MAX){
            printf("%s - 0x%" PRIx64 "\n", regsSearch[i].searchAsm, regsSearch[i].vaddr);
            counter++;
        }
    }

    printf("vulnerable: %.2f%% \n", ((float)counter/(float)searchesLen)*100);

}


/**
 * Searches gadgets and puts all the register's locations into the RegisterInfo struct inside searches.
 * 
 * @param gadgetsLL gadgets link list pointer.
 * @param searches  SearchInfo struct array pointer which holds what to search and place to hold the found location.
 * @param searchesLen The length of searches.
 */
void getSearchInfoArr(GadgetLL *gadgetsLL, SearchInfo *searches, size_t searchesLen){
    //maybe write this better later.

    //note: this was written in such a shortage of time, prob bad with prefix and..
    GadgetLLSearchIntoArr(gadgetsLL, searches, searchesLen);    
}


/**
 * Searches gadgets inside gadgetLL and puts all the register's locations into
 * the RegisterInfo struct inside searches, knows what to search also thanks to searches
 * 
 * @param gadgetsLL gadgets link list pointer.
 * @param searches  SearchInfo struct array pointer which holds what to search and place to hold the found location.
 * @param searchesLen The length of searches.
 * 
*/
void GadgetLLSearchIntoArr(GadgetLL *gadgetsLL, SearchInfo *searches, size_t searchesLen){
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    char decodedBuffer[256];

    ZyanU8 *bufferDecode;
    ZyanU64 runtime_address;
    ZyanU8 instLength;
    MiniInstructionNode *curInstNode;

    for(GadgetNode *curGadget = gadgetsLL->start; curGadget != NULL; curGadget = curGadget->next){
        //if(!checkCondition(curGadget))
        //        continue;

        runtime_address = curGadget->vaddr;
        

        for(curInstNode = curGadget->first; curInstNode != NULL; runtime_address += instLength, curInstNode = curInstNode->next){
            bufferDecode = curInstNode->miniInst.instructionFullOpcode;
            instLength = curInstNode->miniInst.instructionLength;

            if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bufferDecode, instLength, &decodedInstruction, operands))){
                            //Cant decode this, no such instruction.
                            err("NO INSTRUCTION, HOW DID IT GET HERE inside GadgetLLShowBasedCondition at ZydisDecoderDecodeFull?");
                            /*for(int i=0; i<instLength; i++){
                                printf("%hhx ", bufferDecode[i]);
                            }*/
                            exit(123);
                            continue;
                        }
            

            ZydisFormatterFormatInstruction(&formatter, &decodedInstruction, operands,
                decodedInstruction.operand_count_visible, decodedBuffer, sizeof(decodedBuffer), runtime_address, ZYAN_NULL);
            //(passing ZYAN_NULL in user_data)

            //printf("%s", decodedBuffer);
            //if(curInstNode->next != NULL)
            //    printf(" -> ");
            for(size_t i = 0; i < searchesLen; i++){
                if (searches[i].vaddr == UINT64_MAX && !strcmp(searches[i].searchAsm, decodedBuffer)){
                    searches[i].vaddr = runtime_address;
                }
            }

        }

        
    }
}
