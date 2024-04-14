#define _GNU_SOURCE         /* for memmem, the bad search  */

#include "endGadgetFind.h"

#include "costumErrors.h"

#include <stdlib.h>

#include <stdio.h>

//for the bad search:
#include <string.h>

static int initRETIntel(ArchInfo *arch_p);
static int initJMPIntel(ArchInfo *arch_p);

/**
 * This is temporary, create a full search strings so it will also work for others later,
 * and maybe use regex or implement ahoCorasick.
 * 
 * This searches for all ret occurnces in a buffer and returns those indexies.
 * 
 * @param buffer, the buffer.
 * @param bufferSize, the bufferSize
 * 
 * @return FoundLocationsBufferNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
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

    FoundLocationsBufferNode *resultNode = NULL;

    MiniInstructionNode *curInstructionN_p =  arch_p->retEndings->start; 
    for(;curInstructionN_p != NULL; curInstructionN_p = curInstructionN_p->next){
        char *location = memmem(buffer, bufferSize, curInstructionN_p->instructionInfo.mnemonicOpcode, curInstructionN_p->instructionInfo.mnemonicOpcodeSize);
        if (location == NULL)
            continue; //Not found :(
        
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
    }


    return resultNode;
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
        (MiniInstruction){{0xC3, 0, 0}, 1, 0}, // Near RET
        (MiniInstruction){{0xCB, 0, 0}, 1, 0}, // Far  RET
        (MiniInstruction){{0xC2, 0, 0}, 1, 2}, // Near RET {imm16}
        (MiniInstruction){{0xCA, 0, 0}, 1, 2}  // Far RET {imm16}
    };*/
    arch_p->retEndings = miniInstructionLinkedListCreate();
    if(arch_p->retEndings == NULL){
        err("Error creating retEndings inside initRETIntel, inside miniInstructionLinkedListCreate.");
        return 1;
    }
    

    int error = 0;
    //arrays are of size MAX_MEMONIC_OPCODE_LEN(3) 
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3, 0, 0}, 1, 0);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCB, 0, 0}, 1, 0);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC2, 0, 0}, 1, 2);
    error |= miniInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCA, 0, 0}, 1, 2);

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