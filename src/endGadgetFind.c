#define _GNU_SOURCE         /* for memmem, the "bad" search  */

#include "endGadgetFind.h"

#include "costumErrors.h"

#include <stdlib.h>

#include <stdio.h>

//for the "bad" search:
#include <string.h>

static int initRETIntel(ArchInfo *arch_p);
static int initJMPIntel(ArchInfo *arch_p);
static FoundLocationsBufferNode *searchMiniBranchInstructionsInBuffer(char *buffer, size_t bufferSize, MiniBranchInstructionLinkedList *curInstructionN_p);

#define Intel_mnemonicOpcode_RET_Near (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3}
#define Intel_mnemonicOpcodeSize_RET_Near 1
#define Intel_additionalSize_RET_Near 0

#define Intel_mnemonicOpcode_RET_FAR (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCB}
#define Intel_mnemonicOpcodeSize_RET_FAR 1
#define Intel_additionalSize_RET_FAR 0 

#define Intel_mnemonicOpcode_RET_Near_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC2}
#define Intel_mnemonicOpcodeSize_RET_Near_imm16 1
#define Intel_additionalSize_RET_Near_imm16 2

#define Intel_mnemonicOpcode_RET_Far_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCA}
#define Intel_mnemonicOpcodeSize_RET_Far_imm16 1
#define Intel_additionalSize_RET_Far_imm16 2


/**
 * This is searching for every branch instruction in the buffer, including ret, jmp...
 * what is a branch instruction? A branch instruction is an instruction which changes or can change the location of the ip/pc. (example: ret, jmp, call)
 * 
 * @param buffer The buffer.
 * @param bufferSize the bufferSize.
 * @param arch_p The ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return FoundLocationsBufferNode*, a linked list of all the location it found (which contain branch instruction).
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with FoundLocationsBufferNodeFree
*/
FoundLocationsBufferNode *searchBranchInstructionsInBuffer(char *buffer, size_t bufferSize, ArchInfo *arch_p){
    
    
    MiniBranchInstructionLinkedList *allBranchInstructionsMiniInstructionLL = arch_p->retEndings;
    miniInstructionLinkedListCombine(allBranchInstructionsMiniInstructionLL, arch_p->jmpEndings);

    return searchMiniBranchInstructionsInBuffer(buffer, bufferSize, allBranchInstructionsMiniInstructionLL);
}

/**
 * This searches for all MiniBranchInstructions in MiniBranchInstructionNode that occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param MiniBranchInstructionLinkedList* A linked list of branch instructions to search in the buffer.
 * 
 * @return FoundLocationsBufferNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with FoundLocationsBufferNodeFree
 */
static FoundLocationsBufferNode *searchMiniBranchInstructionsInBuffer(char *buffer, size_t bufferSize, MiniBranchInstructionLinkedList *miniBarnchInstructionLL){
    //can later maybe make this better with ahoCorasick algorithm. (or just regular regex)

    FoundLocationsBufferNode *resultNode = NULL;
    MiniBranchInstructionNode *curInstructionN_p = miniBarnchInstructionLL->start;

    for(;curInstructionN_p != NULL; curInstructionN_p = curInstructionN_p->next){
        char *buffer_p = buffer;
        char *location;

        while ( (location = memmem(buffer_p, bufferSize - (buffer_p-buffer), curInstructionN_p->instructionInfo.mnemonicOpcode, curInstructionN_p->instructionInfo.mnemonicOpcodeSize)) ){
            //Found :)

            //printf("woho found RET {0x%" PRIx8 "} in that buffer at: %p which is %ld FINAL: 0x%" PRIx64 "\n", curInstructionN_p->instructionInfo.mnemonicOpcode[0] ,location, location - buffer, buf_vaddr+location - buffer);
            //each one I find I will write its address

            FoundLocationsBufferNode *bufferLocationNode_p = (FoundLocationsBufferNode *) malloc(sizeof(FoundLocationsBufferNode));
            if ( bufferLocationNode_p == NULL ){
                err("Error in malloc, inside searchRetInBuffer, while allocating for bufferLocationNode_p, size %ld.", sizeof(FoundLocationsBufferNode));
                FoundLocationsBufferNodeFree(resultNode);
                return NULL;
            }
            bufferLocationNode_p->offset = location - buffer;
            bufferLocationNode_p->miniInstructionInfo = curInstructionN_p->instructionInfo;
            
            bufferLocationNode_p->next = resultNode;
            resultNode = bufferLocationNode_p;

            buffer_p = location + curInstructionN_p->instructionInfo.mnemonicOpcodeSize;
        }
    }


    return resultNode;
}


/**
 * This searches for all jmp occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param arch_p the ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return FoundLocationsBufferNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with FoundLocationsBufferNodeFree
 */
FoundLocationsBufferNode *searchJmpInBuffer(char *buffer, size_t bufferSize, ArchInfo *arch_p){
    return searchMiniBranchInstructionsInBuffer(buffer, bufferSize, arch_p->jmpEndings);
}


/**
 * This searches for all ret occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param arch_p the ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return FoundLocationsBufferNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with FoundLocationsBufferNodeFree
 */
FoundLocationsBufferNode *searchRetInBuffer(char *buffer, size_t bufferSize, ArchInfo *arch_p){
    /*
    An instruction is built from an opcode and operands.
    
    MNEMONICS, The assembler will transfer it to a machine code.

    https://www.bbc.co.uk/bitesize/guides/z6x26yc/revision/4

    https://doc.zydis.re/v4.0.0/html/EnumMnemonic_8h#a8e72e9a67afaf7087eca5e3aac53202d


    I will need to hardcode the assembly opcodes for the stuff I want.
    (ropper also does that, I checked XD)
    (https://github.dev/sashs/Ropper/blob/master/ropper/gadget.py search for self._pprs)
    and self._endings
    */

    //For now I will do a bad search, I will make it better in the future!    

    return searchMiniBranchInstructionsInBuffer(buffer, bufferSize, arch_p->retEndings);
}


/**
 * Frees Location buffer node.
 * 
 * @param locationsBufferNode, the node to free it and the ones after it.
 * 
 * @note it frees a malloced node and his other ones, make sure not to touch them again.
*/
void FoundLocationsBufferNodeFree(FoundLocationsBufferNode *locationsBufferNode){
    FoundLocationsBufferNode *tmp;
    while(locationsBufferNode != NULL){
        tmp = locationsBufferNode->next;
        free(locationsBufferNode);
        locationsBufferNode = tmp;
    }
}


/**
 * Initilizes the retEndings inside arch. (arch.retEndings)
 * 
 * @param arch, well the arch you want to do it on
 * 
 * @return 0 on success. otherwise any other number on error.
 * 
 * @note arch.machine_mode needs to be set.
 * @note beware, you need to have nothing inside arch.retEndings
 * 
 * @note !!! free it (auto free in freeArchInfo)
 * 
 * http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2a-manual.pdf
 * http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2b-manual.pdf
 * 
 * 
 * Intel® 64 and IA-32 Architectures Software Developer’s Manual Volume 2A: Instruction Set Reference, A-L
 * Chapter 2.1
 * 
 * 
 * Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 1: Basic Architecture
 * Chapter 17.1
 * 
 * In general Chapter 2 is super important for me.
*/
static int initRETIntel(ArchInfo *arch_p){
    /*
    C3                RET        Near return
    CB                RET        Far  return
    C2 iw(word)       RET        Near return
    CA iw(word)       RET        Far  return
    */
    /*arch->retEndings = {
        (MiniBranchInstruction){{0xC3, 0, 0}, 1, 0}, // Near RET
        (MiniBranchInstruction){{0xCB, 0, 0}, 1, 0}, // Far  RET
        (MiniBranchInstruction){{0xC2, 0, 0}, 1, 2}, // Near RET {imm16}
        (MiniBranchInstruction){{0xCA, 0, 0}, 1, 2}  // Far RET {imm16}
    };*/
    arch_p->retEndings = miniInstructionLinkedListCreate();
    if(arch_p->retEndings == NULL){
        err("Error creating retEndings inside initRETIntel, inside miniInstructionLinkedListCreate.");
        return 1;
    }
    

    int error = 0;
    //arrays are of size MAX_MEMONIC_OPCODE_LEN(3) 
    //not doing this in a pointer because a mini instruction is 5 bytes where a pointer is 8. (in 64 bit machines)
    
    //error |= miniInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3, 0, 0}, 1, 0);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near, Intel_mnemonicOpcodeSize_RET_Near, Intel_additionalSize_RET_Near);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_FAR, Intel_mnemonicOpcodeSize_RET_FAR, Intel_additionalSize_RET_FAR);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near_imm16, Intel_mnemonicOpcodeSize_RET_Near_imm16, Intel_additionalSize_RET_Near_imm16);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Far_imm16, Intel_mnemonicOpcodeSize_RET_Far_imm16, Intel_additionalSize_RET_Far_imm16);
    
    //LL    mnemonicOpcode[3]   mnemonicOpcodeSize    additionSize
    if (error){
        err("Error adding instruction to arch->retEndings, in initRETIntel at one of the miniInstructionLinkedListAdd.");
        miniInstructionLinkedListFreeRegular(arch_p->retEndings);
        arch_p->retEndings = NULL;
        return 2;
    }

    return 0;
}



/**
 * Like initRETIntel but for jmp.
 * 
 * @param arch, well the arch you want to do it on
 * 
 * @return 0 on success. otherwise any other number on error.
*/
static int initJMPIntel(ArchInfo *arch_p){
    //todo write later.
    return 0;
}



/**
 * mallocs ArchInfo* and Initilizes/puts all the needed values in it
 * 
 * @param archName, the architecture name.
 * 
 * @return ArchInfo*, a pointer to an allocated area of ArchInfo.
 * 
 * If error returning NULL
*/
ArchInfo *initArchInfo(const char *archName){

    ArchInfo *arch_p = (ArchInfo *) malloc(sizeof(ArchInfo));
    if(arch_p == NULL){
        // Malloc failed
        err("Error, Malloc for head failed inside initArchInfo.\n");
        return NULL;
    }

    if(!strcmp(archName, "x86")){
        arch_p->machine_mode = ZYDIS_MACHINE_MODE_LEGACY_32;
        arch_p->stack_width = ZYDIS_STACK_WIDTH_32;
    }else if(!strcmp(archName, "x64")){
        /*"amd64", "x86_64", "x86-64"*/
        arch_p->machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
        arch_p->stack_width = ZYDIS_STACK_WIDTH_64;
    }else{
        err("Error, can't find machine mode for arch: %s\n", archName);
        free(arch_p);
        return NULL;
    }
    //in all my options (x86 and x64), the memonic is the first and then the operand in the opcode.
    //(zydis does not know anything more than x86 and x64 so I will not take them into account)

    if (initRETIntel(arch_p)){
        err("Error inside initArchInfo in initRETIntel.");
        freeArchInfo(arch_p);
        return NULL;
    }

    if (initJMPIntel(arch_p)){
        err("Error inside initArchInfo in initJMPIntel.");
        freeArchInfo(arch_p);
        return NULL;
    }

    return arch_p;
}

void freeArchInfo(ArchInfo *arch_p){
    if (arch_p != NULL){
        if (arch_p->retEndings != NULL){
            miniInstructionLinkedListFreeRegular(arch_p->retEndings);
            arch_p->retEndings = NULL; //just in case, there is no must to do that.
        }
        if (arch_p->jmpEndings != NULL){
            miniInstructionLinkedListFreeRegular(arch_p->jmpEndings);
            arch_p->jmpEndings = NULL; //Just in case, there is no must to do that.
        }
        free(arch_p);
    }
}