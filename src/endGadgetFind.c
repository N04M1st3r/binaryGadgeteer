#include "endGadgetFind.h"

#include "costumErrors.h"

//for debugging
#include <stdio.h>

static void initRETIntel(ArchInfo *arch);

/**
 * This is temporary, create a full search strings so it will also work for others later,
 * and maybe use regex or implement ahoCorasick.
 * 
 * This searches for all ret occurnces in a buffer and returns those indexies.
 * 
 * @param buffer, the buffer.
 * @param bufferSize, the bufferSize
 * 
 * @return foundLocationsNode*, a linked list of all the location it found.
 *          returning NULL when none found in buffer.
 */
FoundLocationsNode *searchRetInBuffer(char *buffer, uint64_t bufferSize, ArchInfo *arch){
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

   



    return NULL;
}

/**
 * Initilizes the retEndings inside arch. (arch.retEndings)
 * 
 * @param arch, well the arch you want to do it on
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
static void initRETIntel(ArchInfo *arch){
    /*
    C3                RET        Near return
    CB                RET        Far  return
    C2 iw(word)       RET        Near return
    CA iw(word)       RET        Far  return
    */
    arch->retEndings = {
        (MiniInstruction){{0xC3, 0, 0}, 1, 0}, // Near RET
        (MiniInstruction){{0xCB, 0, 0}, 1, 0}, // Far  RET
        (MiniInstruction){{0xC2, 0, 0}, 1, 2}, // Near RET {imm16}
        (MiniInstruction){{0xCA, 0, 0}, 1, 2}  // Far RET {imm16}
    };
    
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

    ArchInfo *arch = (ArchInfo *) malloc(sizeof(ArchInfo));
    if(arch == NULL){
        // Malloc failed
        err("Error, Malloc for head failed inside initArchInfo.\n");
        return NULL;
    }

    if(!strcmp(archName, "x86")){
        arch->machine_mode = ZYDIS_MACHINE_MODE_LEGACY_32;
    }else if(!strcmp(archName, "x64")){
        /*"amd64", "x86_64", "x86-64"*/
        arch->machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    }else{
        err("Error, can't find machine mode for arch: %s\n", archName);
        free(arch);
        return NULL;
    }
    //in all my options (x86 and x64), the memonic is the first and then the operand in the opcode.
    //(zydis does not know anything more than x86 and x64 so I will not take them into account)

    initRETIntel(arch);
}

void freeArchInfo(ArchInfo *arch){
    free(arch);
}