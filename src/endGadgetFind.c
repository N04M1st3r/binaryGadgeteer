#define _GNU_SOURCE         /* for memmem, the "bad" search  */

#include "endGadgetFind.h"

#include "costumErrors.h"

#include <stdlib.h>

#include <stdio.h>

//for the "bad" search:
#include <string.h>

static int initRETIntel(ArchInfo *arch_p);
static int initJMPIntel(ArchInfo *arch_p);
static GadgetLL *searchMiniBranchInstructionsInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t addr_file, size_t bufferSize, MiniBranchInstructionLinkedList *curInstructionN_p);


/*


When the F2 prefix precedes a near CALL, a near RET, a near JMP, a short Jcc, or a near Jcc instruction
(see Appendix E, “Intel® Memory Protection Extensions,” of the Intel® 64 and IA-32 Architectures
Software Developer’s Manual, Volume 1)

*/

//RET:
//https://www.felixcloutier.com/x86/ret

//my theory: first ?(4) bits, the instruction, then bit[4](5th) tells if FAR, then bit[7](8th) tells imm16 or not.
#define Intel_mnemonicOpcode_RET_Near (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3} //0b11000011
#define Intel_mnemonicOpcodeSize_RET_Near 1
#define Intel_additionalSize_RET_Near 0

#define Intel_mnemonicOpcode_RET_FAR (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCB} //0b11001011
#define Intel_mnemonicOpcodeSize_RET_FAR 1
#define Intel_additionalSize_RET_FAR 0 

#define Intel_mnemonicOpcode_RET_Near_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC2} //0b11000010
#define Intel_mnemonicOpcodeSize_RET_Near_imm16 1
#define Intel_additionalSize_RET_Near_imm16 2

#define Intel_mnemonicOpcode_RET_Far_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCA} //0b11001010
#define Intel_mnemonicOpcodeSize_RET_Far_imm16 1
#define Intel_additionalSize_RET_Far_imm16 2

//@TODO do jumps!
//JMP:
//https://www.felixcloutier.com/x86/jmp

//relative JMPs:
//  JMP rel8 / Jump short	                                not sure about the 0xcb?

//  

//absolute  



/**
 * This is searching for every branch instruction in the buffer, including ret, jmp...
 * what is a branch instruction? A branch instruction is an instruction which changes or can change the location of the ip/pc. (example: ret, jmp, call)
 * 
 * @param buffer The buffer.
 * @param buffer_vaddr The buffer virtual address of the first byte in the buffer.
 * @param bufferAddrFile The buffer address in the file of the first byte.
 * @param bufferSize the bufferSize.
 * @param arch_p The ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return GadgetLL*, a linked list of all the location it found (which contain branch instruction).
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with gadgetLLFree
*/
GadgetLL *searchBranchInstructionsInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t bufferAddrFile, size_t bufferSize, ArchInfo *arch_p){
    
    MiniBranchInstructionLinkedList *allBranchInstructionsMiniInstructionLL = arch_p->retEndings;
    miniBranchInstructionLinkedListCombine(allBranchInstructionsMiniInstructionLL, arch_p->jmpEndings);

    return searchMiniBranchInstructionsInBuffer(buffer, buffer_vaddr, bufferAddrFile, bufferSize, allBranchInstructionsMiniInstructionLL);
}

/**
 * This searches for all MiniBranchInstructions in MiniBranchInstructionNode that occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param MiniBranchInstructionLinkedList* A linked list of branch instructions to search in the buffer.
 * 
 * @return GadgetLL*, a linked list of all the location it found.
 *          returning NULL in error, when none are found just resultNode->size=0.
 * 
 * @note This is using malloc, remember to free at the end with gadgetLLFree
 */
static GadgetLL *searchMiniBranchInstructionsInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t bufferAddrFile, size_t bufferSize, MiniBranchInstructionLinkedList *miniBarnchInstructionLL){
    //can later maybe make this better with ahoCorasick algorithm. (or just regular regex)


    //making the start node a fake one, I will delete it later, just for easier.
    GadgetNode fakeNode = {.next=NULL};
    GadgetLL *resultGadgetLL = gadgetLLCreate(&fakeNode);
    if(resultGadgetLL == NULL){
        err("Error inside searchMiniBranchInstructionsInBuffer while calling gadgetLLCreate.");
        return NULL;
    }

    MiniBranchInstructionNode *curInstructionN_p = miniBarnchInstructionLL->start;

    for(;curInstructionN_p != NULL; curInstructionN_p = curInstructionN_p->next){
        char *buffer_p = buffer;
        char *location;

        uint8_t curInstructionLength = curInstructionN_p->instructionInfo.additionSize + curInstructionN_p->instructionInfo.mnemonicOpcodeSize;
        
        //what:
        ZydisMnemonic curInstructionMnemonic = curInstructionN_p->instructionInfo.mnemonic; //ERROR HEREEEEEE TODO FIX!!!

        while ( (location = memmem(buffer_p, bufferSize - (buffer_p-buffer), curInstructionN_p->instructionInfo.mnemonicOpcode, curInstructionN_p->instructionInfo.mnemonicOpcodeSize)) ){
            //Found :)

            //printf("woho found RET {0x%" PRIx8 "} in that buffer at: %p which is %ld FINAL: 0x%" PRIx64 "\n", curInstructionN_p->instructionInfo.mnemonicOpcode[0] ,location, location - buffer, buf_vaddr+location - buffer);
            //each one I find I will write its address
            
            MiniInstructionNode *miniInstNode = MiniInstructionNodeCreate(curInstructionMnemonic, curInstructionLength, location, NULL);
            if(miniInstNode == NULL){
                err("Error inside searchMiniBranchInstructionsInBuffer at MiniInstructionNodeCreate.");
                
                //so it will not free the first one, and same for last if it is still ==start
                resultGadgetLL->start = resultGadgetLL->start->next;
                if( resultGadgetLL->end == &fakeNode )
                    resultGadgetLL->end = NULL;

                gadgetLLFreeAll(resultGadgetLL);
                return NULL;
            }

            uint64_t offset = location - buffer;
            GadgetNode *curGadgetNode = GadgetNodeCreate(miniInstNode, bufferAddrFile+offset, buffer_vaddr+offset);
            if(curGadgetNode == NULL){
                err("Error inside searchMiniBranchInstructionsInBuffer at GadgetNodeCreate.");

                //so it will not free the first one, and same for last if it is still ==start.
                resultGadgetLL->start = resultGadgetLL->start->next;
                if(resultGadgetLL->end == &fakeNode)
                    resultGadgetLL->end = NULL;
                gadgetLLFreeAll(resultGadgetLL);
                return NULL;
            }

            GadgetLLAddGadgetNode(resultGadgetLL, curGadgetNode);
            

            buffer_p = location + curInstructionN_p->instructionInfo.mnemonicOpcodeSize;
        }
    }

    resultGadgetLL->start = resultGadgetLL->start->next;
    (resultGadgetLL->size)--;
    if(resultGadgetLL->end == &fakeNode)
        resultGadgetLL->end = NULL;
    

    return resultGadgetLL;
}


/**
 * This searches for all jmp occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param arch_p the ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return GadgetNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with gadgetNodeFreeAll...
 */
GadgetLL *searchJmpInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t bufferAddrFile, size_t bufferSize, ArchInfo *arch_p){
    return searchMiniBranchInstructionsInBuffer(buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->jmpEndings);
}


/**
 * This searches for all ret occurnces in a buffer and returns those indexies.
 * 
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param arch_p the ArchInfo which contains info about this spesific architecture, and how to parse it.
 * 
 * @return GadgetLL*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 * 
 * @note This is using malloc, remember to free at the end with gadgetLLFree
 */
GadgetLL *searchRetInBuffer(char *buffer, ZyanU64 buffer_vaddr, uint64_t bufferAddrFile, size_t bufferSize, ArchInfo *arch_p){
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

    return searchMiniBranchInstructionsInBuffer(buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->retEndings);
}


/**
 * Frees Location buffer node.
 * 
 * @param locationsBufferNode, the node to free it and the ones after it.
 * 
 * @note it frees a malloced node and his other ones, make sure not to touch them again.
*/
/*
//TODO: del this
void FoundLocationsBufferNodeFree(gadgetLL *locationsBufferNode){
    FoundLocationsBufferNode *tmp;
    while(locationsBufferNode != NULL){
        tmp = locationsBufferNode->next;
        free(locationsBufferNode);
        locationsBufferNode = tmp;
    }
}*/


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
    arch_p->retEndings = miniBranchInstructionLinkedListCreate();
    if(arch_p->retEndings == NULL){
        err("Error creating retEndings inside initRETIntel, inside miniBranchInstructionLinkedListCreate.");
        return 1;
    }
    

    int error = 0;
    //arrays are of size MAX_MEMONIC_OPCODE_LEN(3) 
    //not doing this in a pointer because a mini instruction is 5 bytes where a pointer is 8. (in 64 bit machines)
    
    //https://github.com/HJLebbink/asm-dude/wiki/RET
    //error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3, 0, 0}, 1, 0);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near, Intel_mnemonicOpcodeSize_RET_Near, Intel_additionalSize_RET_Near, ZYDIS_MNEMONIC_RET);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_FAR, Intel_mnemonicOpcodeSize_RET_FAR, Intel_additionalSize_RET_FAR, ZYDIS_MNEMONIC_RET);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near_imm16, Intel_mnemonicOpcodeSize_RET_Near_imm16, Intel_additionalSize_RET_Near_imm16, ZYDIS_MNEMONIC_RET);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Far_imm16, Intel_mnemonicOpcodeSize_RET_Far_imm16, Intel_additionalSize_RET_Far_imm16, ZYDIS_MNEMONIC_RET);
    
    //LL    mnemonicOpcode[3]   mnemonicOpcodeSize    additionSize
    if (error){
        err("Error adding instruction to arch->retEndings, in initRETIntel at one of the miniBranchInstructionLinkedListAdd.");
        miniBranchInstructionLinkedListFreeRegular(arch_p->retEndings);
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

    //note that some of the jmps are not supported in 64bit but are supported in 32bit.

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
            miniBranchInstructionLinkedListFreeRegular(arch_p->retEndings);
            arch_p->retEndings = NULL; //just in case, there is no must to do that.
        }
        if (arch_p->jmpEndings != NULL){
            miniBranchInstructionLinkedListFreeRegular(arch_p->jmpEndings);
            arch_p->jmpEndings = NULL; //Just in case, there is no must to do that.
        }
        free(arch_p);
    }
}