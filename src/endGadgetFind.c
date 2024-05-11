#define _GNU_SOURCE         /* for memmem, the "bad" search  */

#include "endGadgetFind.h"

#include "costumErrors.h"

#include <stdlib.h>

#include <stdio.h>

//for the "bad" search:
#include <string.h>

static int initRETIntel(ArchInfo *arch_p);
static int initJMPIntel(ArchInfo *arch_p);
static int initCALLIntel(ArchInfo *arch_p);
static GadgetLL *searchMiniBranchInstructionsInBuffer(ArchInfo *arch_p, char *buffer, ZyanU64 buffer_vaddr, uint64_t addr_file, size_t bufferSize, MiniBranchInstructionLinkedList *curInstructionN_p);


/*
(note, jcc is every condition jmp(not including jmp))


— BND prefix is encoded using F2H if the following conditions are true:
    (when weird things)
    When the F2 prefix precedes a near CALL, a near RET, a near JMP, a short Jcc, or a near Jcc instruction
    (see Appendix E, “Intel® Memory Protection Extensions,” of the Intel® 64 and IA-32 Architectures
    Software Developer’s Manual, Volume 1)

  BND JMP


If a REX prefix is used when it has no meaning, it is ignored.

*/
/*
also need to check for prefixes, and add them.

note that: The REX prefix is only available in long mode.
When there are two or more prefixes from a single group, the behavior is undefined. Some processors ignore the subsequent prefixes from the same group, or use only the last prefix specified for any group.

so just wtf?

(can put 1 of each group)
if combining a couple from the same group it will be undefined behavior. (I want it)
legacy prefixes:
    group 1:
        0xf0 LOCK
        0xf2 REPNE/REPNZ
        0xf3 REP or REPE/REPZ

    group 2:
        0x2e CS segmenet override
        0x36 SS segmenet override
        0x3e DS
        0x26 ES
        0x64 FS
        0x65 GS
        
        0x2e Branch not taken
        0x3e Branch taken

    group 3:
        0x66 Operand-size override prefix
    
    group 4:
        0x67 Address-size override prefix


REW.x prefix:
(REX is after the prefix, meaning right before the opcode if there is one)
question: what will happen if I use REX in 32 bit? because 
`REX prefixes are instruction-prefix bytes used in 64-bit mode`

note that If a REX prefix is used when it has no meaning, it is ignored.
so this is good for the ROP.

VEX/XOP prefix:

okay maybe I can put a couple of good things for me.
ok this is a whole thing to research, I will do that later.
it seems very interesting!

*/


//RET:
//https://www.felixcloutier.com/x86/ret

//my theory: first ?(4) bits, the instruction, then bit[4](5th) tells if FAR, then bit[7](8th) tells imm16 or not.
#define Intel_mnemonicOpcode_RET_Near (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC3} //0b11000011
#define Intel_mnemonicOpcodeSize_RET_Near 1
#define Intel_additionalSize_RET_Near 0
#define Intel_checkThis_RET_Near false

#define Intel_mnemonicOpcode_RET_FAR (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCB} //0b11001011
#define Intel_mnemonicOpcodeSize_RET_FAR 1
#define Intel_additionalSize_RET_FAR 0
#define Intel_checkThis_RET_FAR false


#define Intel_mnemonicOpcode_RET_Near_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xC2} //0b11000010
#define Intel_mnemonicOpcodeSize_RET_Near_imm16 1
#define Intel_additionalSize_RET_Near_imm16 2
#define Intel_checkThis_RET_Near_imm16 false


#define Intel_mnemonicOpcode_RET_Far_imm16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xCA} //0b11001010
#define Intel_mnemonicOpcodeSize_RET_Far_imm16 1
#define Intel_additionalSize_RET_Far_imm16 2
#define Intel_checkThis_RET_Far_imm16 false


//@TODO do jumps!
//JMP:
//https://www.felixcloutier.com/x86/jmp

//relative JMPs:


//  JMP rel8 | Jump short | EB cb	
#define Intel_mnemonicOpcode_JMP_Short_rel8 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xEB}
#define Intel_mnemonicOpcodeSize_JMP_Short_rel8 1
#define Intel_additionalSize_JMP_Short_rel8 1 //cb
#define Intel_checkThis_JMP_Short_rel8 false


// JMP rel16 | E9 cw        (NOT SUPPORTED IN 64 BIT!)
#define Intel_mnemonicOpcode_JMP_Near_rel16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xE9}
#define Intel_mnemonicOpcodeSize_JMP_Near_rel16 1
#define Intel_additionalSize_JMP_Near_rel16 2 //cw
#define Intel_checkThis_JMP_Near_rel16 false


// JMP rel32 | E9 cd         
#define Intel_mnemonicOpcode_JMP_Near_rel32 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xE9}
#define Intel_mnemonicOpcodeSize_JMP_Near_rel32 1
#define Intel_additionalSize_JMP_Near_rel32 4 //cd
#define Intel_checkThis_JMP_Near_rel32 false
//NOTE: maybe there is a way to combine those? the rel32 and rel16?

//absolute JMPs:  

//generally there are these that start with FF and then something:
// JMP r/m16	     | FF /4         (ModRM:r/m (r)	)  (NEAR)      (NOT SUPPORTED IN 64 BIT!)
// JMP r/m32	     | FF /4         (ModRM:r/m (r)	)  (NEAR)      (NOT SUPPORTED IN 64 BIT!)
// JMP r/m64	     | FF /4         (ModRM:r/m (r)	)  (NEAR)

// JMP m16:16	     | FF /5	     (ModRM:r/m (r)	)  (FAR )
// JMP m16:32	     | FF /5	     (ModRM:r/m (r)	)  (FAR )
// JMP m16:64	     | REX.W FF /5	 (ModRM:r/m (r)	)  (FAR )

//examples:
/*
(Created using https://defuse.ca/online-x86-assembler.htm)


first lets talk about ModR/M:
https://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM

BIT    |7|6|5|4|3|2|1|0|
Usage  |MOD|REG  | R/M |

MODRM.mod	2 bits.
    ~ In general, when this field is b11, then register-direct addressing mode is used; otherwise register-indirect addressing mode is used.

MODRM.reg   3 bits.
    ~ This field can have one of two values:
        - A 3-bit opcode extension, which is used by some instructions but has no further meaning other than distinguishing the instruction from other instructions.
        - A 3-bit register reference, which can be used as the source or the destination of an instruction (depending on the instruction).
            The referenced register depends on the operand-size of the instruction and the instruction itself.
            See Registers for the values to use for each of the registers.
            The REX.R, VEX.~R or XOP.~R field can extend this field with 1 most-significant bit to 4 bits total.

MODRM.rm	3 bits.
    ~ Specifies a direct or indirect register operand, optionally with a displacement.
        The REX.B, VEX.~B or XOP.~B field can extend this field with 1 most-significant bit to 4 bits total.

addition information on MODRM.mod:
    00: mostly [r/m]        (changes with SP and R12 to [SIB]. and also with BP and R13 to [eip/rip + disp32](note 1,2))
    01: mostly [r/m+disp8]  (changes with SP and R12 to [SIB+disp8] )
    10: mostly [r/m+disp32] (changes with SP and R12 to [SIB+disp32])
    11: mostly r/m

    note:
        1.  In protected/compatibility mode, this is just disp32, but in long mode this is [RIP]+disp32 (for 64-bit addresses) or [EIP]+disp32
        2.  In long mode, to encode disp32 as in protected/compatibility mode, use the SIB byte.

I will not be reasearching SIB byte right now, I will leave it for later.

in these JMP the cases are these:
    MODRM.reg -> Used to distinguishing the instruction from other instructions.
    MODRM.rm  -> Specifies a direct or indirect register operand.
    MODRM.mod -> using 00, and 01




SIB:
    https://wiki.osdev.org/X86-64_Instruction_Encoding#SIB
    SIB.scale	2 bits
        This field indicates the scaling factor of SIB.index, where s (as used in the tables) equals 2^SIB.scale.
        so:
            00 -> factor 1.
            01 -> factor 2.
            10 -> factor 4.
            11 -> factor 8.
    
    SIB.index   3 bits
        The index register to use.
        (See Registers for the values to use for each of the registers.)
        (The REX.X, VEX.~X or XOP.~X field can extend this field with 1 most-significant bit to 4 bits total.)
    
    SIB.base	3 bits
        The base register to use.
        (See Registers for the values to use for each of the registers.)
        (The REX.B, VEX.~B or XOP.~B field can extend this field with 1 most-significant bit to 4 bits total.)



to format in a nice way in python:
'|'+'|'.join(bin(byteToFormatNum)[2:].rjust(8, '0'))+'|'

x86(32 bit):

    firstly lets look at "JMP r/m16", "FF /4"
    JMP WORD PTR [eax]         -> "66 ff 20"                    (JMP r (16))
    JMP WORD PTR [ecx]         -> "66 ff 21"                    (JMP r (16))
    JMP WORD PTR [edx]         -> "66 ff 22"                    (JMP r (16))
    JMP WORD PTR [ebx]         -> "66 ff 23"                    (JMP r (16))

    JMP WORD PTR [0x12345678]  -> "66 ff 25 78 56 34 12"        (JMP m16)

    JMP WORD PTR [esi]         -> "66 ff 26"                    (JMP r (16))
    JMP WORD PTR [edi]         -> "66 ff 27"                    (JMP r (16))

    JMP WORD PTR [esp]         -> "66 ff 24 24"                 (JMP r (16))
    
    

    
    explenation:
        1st byte:
            the 0x66 is a Operand-size override prefix (PREFIX)
            note that without the prefix it would have been "JMP DWORD PTR [eax]"
        (I will talk about the other bytes later in extended)

        2nd byte:
            https://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM
            https://wiki.osdev.org/X86-64_Instruction_Encoding#SIB
            https://wiki.osdev.org/X86-64_Instruction_Encoding#Registers

            that is the ModRM:r/m byte.
            BIT    |7|6|5|4|3|2|1|0|
            Usage  |MOD|REG  | R/M |
            0x20 = |0|0|1|0|0|0|0|0|       -> [eax]
            0x21 = |0|0|1|0|0|0|0|1|       -> [ecx]
            0x22 = |0|0|1|0|0|0|1|0|       -> [edx]
            0x23 = |0|0|1|0|0|0|1|1|       -> [ebx]
            0x24 = |0|0|1|0|0|1|0|0|       -> [RELATED TO SIB]                         (note this is on the esp location)
            0x25 = |0|0|1|0|0|1|0|1|       -> look at 4 next for answer ([0x12345678](little endian)) (note this is on the ebp location)
            0x26 = |0|0|1|0|0|1|1|0|       -> [esi]
            0x27 = |0|0|1|0|0|1|1|1|       -> [edi]
            
            and there is no more because it will change REG or MOD.
            

            lets explain:
            MOD, the MOD in all is 0, which means we are looking inside them, looking for [r/m]. (different in different JMPs)
            REG, the REG in all is 0b100=4 (as specified is just to differenciate from the different JMPs)

            now the R/M and here it is a bit complicated.
            generally it is getting from a register by using this table: https://wiki.osdev.org/X86-64_Instruction_Encoding#Registers
            but this is only for what fits in the first 3 bits, and for the more registers (r8,r9...) it uses a kind of prefix that I will not talk about.
            (BAD I am ignoring JMPs with these registers...) 

            but still there are exceptions 0b100=4 and 0b101=5 are not registers...
            yes, they are suppous to be SP and BP.
            but they are not.
            0x25 tells us that it needs to jump to what is the address of the next 4 bytes.

            and 0x24 is telling us there is an SIB, go to optional 3rd byte.
        
        optional 3rd byte SIB:
            (this is not the DATA, data is just data)
            https://wiki.osdev.org/X86-64_Instruction_Encoding#SIB
            lets look at a few examples with SIB:
            JMP WORD PTR [ecx + eax]    -> "66 ff 24 01"
            JMP WORD PTR [ecx + ecx]    -> "66 ff 24 09"
            JMP WORD PTR [eax + ebx]    -> "66 ff 24 18"
            JMP WORD PTR [esp]          -> "66 ff 24 24"
            JMP WORD PTR [ecx + eax*2]  -> "66 ff 24 41"
            JMP WORD PTR [esp + eax*2]  -> "66 ff 24 44" 
            JMP WORD PTR [esp + ecx*2]  -> "66 ff 24 4c"
            JMP WORD PTR [ecx + eax*4]  -> "66 ff 24 81"
            JMP WORD PTR [ecx + eax*8]  -> "66 ff 24 c1"
            JMP WORD PTR [ecx + eax*8]  -> "66 ff 24 c1"


            I already explained everything before the SIB byte so lets look at it:
                *writing scl instaed of scale because not enough space.

                BIT    |7|6|5|4|3|2|1|0|
                Usage  |SCL|Idx  | BASE|
                0x01 = |0|0|0|0|0|0|0|1| -> [ecx + eax*1]
                0x09 = |0|0|0|0|1|0|0|1| -> [ecx + ecx*1]
                0x18 = |0|0|0|1|1|0|0|0| -> [eax + ebx*1]
                0x24 = |0|0|1|0|0|1|0|0| -> [esp]
                0x41 = |0|1|0|0|0|0|0|1| -> [ecx + ecx*2]
                0x44 = |0|1|0|0|0|1|0|0| -> [esp + eax*2]
                0x4c = |0|1|0|0|1|1|0|0| -> [esp + eax*2]
                0x81 = |1|0|0|0|0|0|0|1| -> [ecx + eax*4]
                0xc1 = |1|1|0|0|0|0|0|1| -> [ecx + eax*8]

                first the scale/SCL, this is just multiplying the base by 2**(scale).
                IDX/index and BASE/base are both registers, from https://wiki.osdev.org/X86-64_Instruction_Encoding#Registers
                and it just adds them and goes there.

                but wait, what is happening is esp? lets look:
                00 so 2**0=1 scale.
                index register is esp
                source register is esp
                wait so why isn't it [esp+esp]?
                lets look at another one that is it + itself, we have [ecx+ + ecx*1] and it is just as we expected.

                okay weird, and after I checked i see that it weird for everything that esp is an idx for them.
                so to add to RSP I need to put it in the BASE, and when ESP is in the base the index is multiplied instead of the base.
                so that is kinda all the weird, because of that wierd sitiuation this happens:
                    We cannot do something like that: JMP WORD PTR [EAX + ESP*2]
                because everything that ESP is in the Idx is "special"
                so everytime ESP is mentined it wants to put it in the BASE, and the other in the Idx and just reverse the what is multiplied.
                but if we do that we cannot multiply ESP, we throw away the option to do so.

                another weird thing is that EBP in ebp the same thing happens, it also cannot be in the BASE for some reason,
                try: 0x2d=0b00101101 / 0x05=0b00000101 for example. (it won't work)

                but we can do:
                jmp    WORD PTR [ecx+ebp*1]

                like so: 0x29=0b00101001
                meaning again EBP is in the index but it itself is multiplied, exactly as happened in esp

                but there is still ebp we can't reach.
                well for ebp we have to in another JMP, becuase it does JMP [rbp+0]
                

         
        more:
            if there is more then its for the address


    JMP WORD PTR [ebp] will not be in that category because there is no such instruction 
    JMP WORD PTR [ebp+1]         -> "66 ff 65 01"                 (JMP r (16))
    JMP WORD PTR [eax+1]         -> "66 ff 60 01"   
    note that in bp it actually does: JMP    WORD PTR [ebp+0x0]

    BIT    |7|6|5|4|3|2|1|0|
    Usage  |MOD|REG  | R/M |
    0x65 = |0|1|1|0|0|1|0|1|
    0x60 = |0|1|1|0|0|0|0|0|

    
    JMP DWORD PTR [eax]         -> "ff 60 04"                    (JMP r (32))
    JMP DWORD PTR [0x12345678]  -> "ff 25 7c 56 34 12"        (JMP m32)

    2nd byte:
            0x20 in the first and 0x25 in the second.
            that is the ModRM:r/m byte.
            lets see what it says:
            
            BIT    |7|6|5|4|3|2|1|0|
            Usage  |MOD|REG  |R/M  |
            0x20 = |0|0|1|0|0|0|0|0|
            0x25 = |0|0|1|0|0|1|0|1|


            


    okay so after this example I will now explain everything.
        in the 


    


    a little weird thing:
        in https://defuse.ca/online-x86-assembler.htm
        both "66 ff 20" and "ff 20" give "JMP DWORD PTR [eax]"
        note that "66 ff 20" gives "JMP WORD PTR [eax]", and "ff 28" gives "JMP FWORD PTR [eax]".

        also: "66 ff 30" is push and "66 ff 38" is not an instruction.
        (I took those because what is changing only the REG in the MODRM and not the R/M)

*/

//for now as stated I will ignore the one with the REX prefix, so the `JMP m16:64` will be ignored.

//As I don't have enough time I will do the slow but easy solution.
#define Intel_mnemonicOpcode_JMP_FF (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xff}
#define Intel_mnemonicOpcodeSize_JMP_FF 1
#define Intel_additionalSize_JMP_FF 8 //unknown, that is just MAX
#define Intel_checkThis_JMP_FF true


//EA cd	| JMP ptr16:16	| Jump far, absolute, address given in operand. (NOT ON 64 bit)
#define Intel_mnemonicOpcode_JMP_FAR_ptr16_16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xEA}
#define Intel_mnemonicOpcodeSize_JMP_FAR_ptr16_16 1
#define Intel_additionalSize_JMP_FAR_ptr16_16 4
#define Intel_checkThis_JMP_FAR_ptr16_16 false

//EA cp	| JMP ptr16:32	| Jump far, absolute, address given in operand. (NOT ON 64 bit)
#define Intel_mnemonicOpcode_JMP_FAR_ptr16_32 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xEA}
#define Intel_mnemonicOpcodeSize_JMP_FAR_ptr16_32  1
#define Intel_additionalSize_JMP_FAR_ptr16_32 6
#define Intel_checkThis_JMP_FAR_ptr16_32 false

//now the CALL

//https://www.felixcloutier.com/x86/call
//E8 cw	| CALL rel16	| Near
#define Intel_mnemonicOpcode_CALL_Near_rel16 (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xE8}
#define Intel_mnemonicOpcodeSize_CALL_Near_rel16  1
#define Intel_additionalSize_CALL_Near_rel16 2
#define Intel_checkThis_CALL_Near_rel16 true //false (for some reason I can't call near all)
//for example: `e8 b0 45` caused error.

//E8 cd	| CALL rel32	| Near
#define Intel_mnemonicOpcode_CALL_Near_rel32  (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xE8}
#define Intel_mnemonicOpcodeSize_CALL_Near_rel32   1
#define Intel_additionalSize_CALL_Near_rel32  4
#define Intel_checkThis_CALL_Near_rel32  true //false (for some reason I can't call near all)

//As I don't have enough time I will do the slow but easy solution.
#define Intel_mnemonicOpcode_CALL_FF (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0xff}
#define Intel_mnemonicOpcodeSize_CALL_FF 1
#define Intel_additionalSize_CALL_FF 8 //unknown, that is just MAX
#define Intel_checkThis_CALL_FF true

//9A cd	| CALL ptr16:16	(Invalid for 64 bit)
#define Intel_mnemonicOpcode_CALL_Far_ptr_16_16  (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0x9a}
#define Intel_mnemonicOpcodeSize_CALL_Far_ptr_16_16   1
#define Intel_additionalSize_CALL_Far_ptr_16_16  4
#define Intel_checkThis_CALL_Far_ptr_16_16  false

//9A cp	| CALL ptr16:32	(Invalid for 64 bit)
#define Intel_mnemonicOpcode_CALL_Far_ptr_16_32  (uint8_t [MAX_MEMONIC_OPCODE_LEN]){0x9a}
#define Intel_mnemonicOpcodeSize_CALL_Far_ptr_16_32   1
#define Intel_additionalSize_CALL_Far_ptr_16_32  6
#define Intel_checkThis_CALL_Far_ptr_16_32  false


/*
mov ax, a
jmp ax
jmp WORD PTR [b]
jmp DWORD PTR [b]
jmp QWORD PTR [b]

a:
inc ax

b:
inc bx


0:  66 8b 04 25 00 00 00    mov    ax,WORD PTR ds:0x0
7:  00
8:  66 ff e0                jmp    ax
b:  66 ff 24 25 00 00 00    jmp    WORD PTR ds:0x0
12: 00
13: 66 ff 2c 25 00 00 00    jmp    DWORD PTR ds:0x0
1a: 00
1b: ff 24 25 00 00 00 00    jmp    QWORD PTR ds:0x0
0000000000000022 <a>:
22: 66 ff c0                inc    ax
0000000000000025 <b>:
25: 66 ff c3                inc    bx
*/





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
    //todo: copy this instead of just doing equals.
    //can also put them all together before:
    //MiniBranchInstructionLinkedList *allBranchInstructionsMiniInstructionLL = arch_p->retEndings;
    
    GadgetLL *retInstructions = searchMiniBranchInstructionsInBuffer(arch_p, buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->retEndings);

    GadgetLL *allBranchInstructions = retInstructions;

    GadgetLL *jmpInstructions = searchMiniBranchInstructionsInBuffer(arch_p, buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->jmpEndings);
    GadgetLLCombine(allBranchInstructions, jmpInstructions);
    gadgetLLFreeOnly(jmpInstructions);

    GadgetLL *callInstructions = searchMiniBranchInstructionsInBuffer(arch_p, buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->callEndings);
    GadgetLLCombine(allBranchInstructions, callInstructions);
    gadgetLLFreeOnly(callInstructions);



    return allBranchInstructions;
}

/**
 * This searches for all MiniBranchInstructions in MiniBranchInstructionNode that occurnces in a buffer and returns those indexies.
 * 
 * @param arch_p the arch, maybe remove this later.
 * @param buffer the buffer.
 * @param bufferSize the bufferSize
 * @param MiniBranchInstructionLinkedList* A linked list of branch instructions to search in the buffer.
 * 
 * @return GadgetLL*, a linked list of all the location it found.
 *          returning NULL in error, when none are found just resultNode->size=0.
 * 
 * @note This is using malloc, remember to free at the end with gadgetLLFree
 */
static GadgetLL *searchMiniBranchInstructionsInBuffer(ArchInfo *arch_p, char *buffer, ZyanU64 buffer_vaddr, uint64_t bufferAddrFile, size_t bufferSize, MiniBranchInstructionLinkedList *miniBarnchInstructionLL){
    //can later maybe make this better with ahoCorasick algorithm. (or just regular regex)

    //from here bad
    //cam get decoder from the decoder but this is like this to prove that this is bad.
    ZydisDecoder decoder;
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, arch_p->machine_mode, arch_p->stack_width))){
        err("Error Initiating decoder, at searchMiniBranchInstructionsInBuffer with ZydisDecoderInit.");
        return NULL;
    }
    ZydisDecodedInstruction decodedInstruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    //to here bad

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

        bool doCheck = curInstructionN_p->instructionInfo.checkThis; //Bad and hacky..
        
        while ( (location = memmem(buffer_p, bufferSize - (buffer_p-buffer), curInstructionN_p->instructionInfo.mnemonicOpcode, curInstructionN_p->instructionInfo.mnemonicOpcodeSize)) ){
            //Found :)
            uint64_t offset = location - buffer;
            //printf("woho found RET {0x%" PRIx8 "} in that buffer at: %p which is %ld FINAL: 0x%" PRIx64 "\n", curInstructionN_p->instructionInfo.mnemonicOpcode[0] ,location, location - buffer, buf_vaddr+location - buffer);
            //each one I find I will write its address
            if(curInstructionLength + offset > bufferSize){
                //there is not enough space for the instruction.

                //doing > instead of >= because lets say I am at offset=0, with bufferSize=1.
                //then curInstructionLength=1 for example, I want to return it.
                //or in other words it reads until what is calcualted, not after.
                break;
            }
            
            if (doCheck){
                //again.. bad and hacky..
                
                if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, location, curInstructionLength, &decodedInstruction, operands))){
                            //Cant decode this, no such instruction.
                            buffer_p = location + 1;
                            continue;
                        }
                if(decodedInstruction.mnemonic != curInstructionMnemonic){
                    buffer_p = location + 1;
                    continue;
                }
                //this is a good instruction woho.
                curInstructionLength = decodedInstruction.length;
                //printf("WOHO FOUND IN %lx\n", buffer_vaddr+offset);
            }

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
            
            

            //if I don't want things that are on the same things
            //buffer_p = location + curInstructionN_p->instructionInfo.mnemonicOpcodeSize;

            //if I do want things that are on the same things.
            buffer_p = location + 1;
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
    return searchMiniBranchInstructionsInBuffer(arch_p, buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->jmpEndings);
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

    return searchMiniBranchInstructionsInBuffer(arch_p, buffer, buffer_vaddr, bufferAddrFile, bufferSize, arch_p->retEndings);
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
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near, Intel_mnemonicOpcodeSize_RET_Near, Intel_additionalSize_RET_Near, ZYDIS_MNEMONIC_RET, Intel_checkThis_RET_Near);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_FAR, Intel_mnemonicOpcodeSize_RET_FAR, Intel_additionalSize_RET_FAR, ZYDIS_MNEMONIC_RET, Intel_checkThis_RET_FAR);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Near_imm16, Intel_mnemonicOpcodeSize_RET_Near_imm16, Intel_additionalSize_RET_Near_imm16, ZYDIS_MNEMONIC_RET, Intel_checkThis_RET_Near_imm16);
    error |= miniBranchInstructionLinkedListAdd(arch_p->retEndings, Intel_mnemonicOpcode_RET_Far_imm16, Intel_mnemonicOpcodeSize_RET_Far_imm16, Intel_additionalSize_RET_Far_imm16, ZYDIS_MNEMONIC_RET, Intel_checkThis_RET_Far_imm16);
    
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
    arch_p->jmpEndings = miniBranchInstructionLinkedListCreate();
    if(arch_p->jmpEndings == NULL){
        err("Error creating jmpEndings inside initJMPIntel, inside miniBranchInstructionLinkedListCreate.");
        return 1;
    }
    

    int error = 0;

    if(arch_p->machine_mode == ZYDIS_MACHINE_MODE_LONG_64){
        //64-Bit Mode	

    }else{
        //Compat/Leg Mode
        error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_Near_rel16, Intel_mnemonicOpcodeSize_JMP_Near_rel16, Intel_additionalSize_JMP_Near_rel16, ZYDIS_MNEMONIC_JMP, Intel_checkThis_JMP_Near_rel16);

        error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_FAR_ptr16_16, Intel_mnemonicOpcodeSize_JMP_FAR_ptr16_16, Intel_additionalSize_JMP_FAR_ptr16_16, ZYDIS_MNEMONIC_JMP, Intel_checkThis_JMP_FAR_ptr16_16);
        error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_FAR_ptr16_32, Intel_mnemonicOpcodeSize_JMP_FAR_ptr16_32, Intel_additionalSize_JMP_FAR_ptr16_32, ZYDIS_MNEMONIC_RET, Intel_checkThis_JMP_FAR_ptr16_32);
    }

    error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_Short_rel8, Intel_mnemonicOpcodeSize_JMP_Short_rel8, Intel_additionalSize_JMP_Short_rel8, ZYDIS_MNEMONIC_JMP, Intel_checkThis_JMP_Short_rel8);
    error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_Near_rel32, Intel_mnemonicOpcodeSize_JMP_Near_rel32, Intel_additionalSize_JMP_Near_rel32, ZYDIS_MNEMONIC_JMP, Intel_checkThis_JMP_Near_rel32);


    error |= miniBranchInstructionLinkedListAdd(arch_p->jmpEndings, Intel_mnemonicOpcode_JMP_FF, Intel_mnemonicOpcodeSize_JMP_FF, Intel_additionalSize_JMP_FF, ZYDIS_MNEMONIC_JMP, Intel_checkThis_JMP_FF);


    if (error){
        err("Error adding instruction to arch->jmpEndings, in initJMPIntel at one of the miniBranchInstructionLinkedListAdd.");
        miniBranchInstructionLinkedListFreeRegular(arch_p->jmpEndings);
        arch_p->jmpEndings = NULL;
        return 2;
    }

    return 0;
}


/**
 * Like initRETIntel but for call.
 * 
 * @param arch, well the arch you want to do it on
 * 
 * @return 0 on success. otherwise any other number on error.
*/
static int initCALLIntel(ArchInfo *arch_p){
    arch_p->callEndings = miniBranchInstructionLinkedListCreate();
    if(arch_p->callEndings == NULL){
        err("Error creating callEndings inside initCALLIntel, inside miniBranchInstructionLinkedListCreate.");
        return 1;
    }
    

    int error = 0;

    if(arch_p->machine_mode == ZYDIS_MACHINE_MODE_LONG_64){
        //64-Bit Mode	

    }else{
        //Compat/Leg Mode
        error |= miniBranchInstructionLinkedListAdd(arch_p->callEndings, Intel_mnemonicOpcode_CALL_Far_ptr_16_16, Intel_mnemonicOpcodeSize_CALL_Far_ptr_16_16, Intel_additionalSize_CALL_Far_ptr_16_16, ZYDIS_MNEMONIC_CALL, Intel_checkThis_CALL_Far_ptr_16_16);
        error |= miniBranchInstructionLinkedListAdd(arch_p->callEndings, Intel_mnemonicOpcode_CALL_Far_ptr_16_32, Intel_mnemonicOpcodeSize_CALL_Far_ptr_16_32, Intel_additionalSize_CALL_Far_ptr_16_32, ZYDIS_MNEMONIC_CALL, Intel_checkThis_CALL_Far_ptr_16_32);

    }

    error |= miniBranchInstructionLinkedListAdd(arch_p->callEndings, Intel_mnemonicOpcode_CALL_Near_rel16, Intel_mnemonicOpcodeSize_CALL_Near_rel16, Intel_additionalSize_CALL_Near_rel16, ZYDIS_MNEMONIC_CALL, Intel_checkThis_CALL_Near_rel16);

    error |= miniBranchInstructionLinkedListAdd(arch_p->callEndings, Intel_mnemonicOpcode_CALL_Near_rel32, Intel_mnemonicOpcodeSize_CALL_Near_rel32, Intel_additionalSize_CALL_Near_rel32, ZYDIS_MNEMONIC_CALL, Intel_checkThis_CALL_Near_rel32);

    error |= miniBranchInstructionLinkedListAdd(arch_p->callEndings, Intel_mnemonicOpcode_CALL_FF, Intel_mnemonicOpcodeSize_CALL_FF, Intel_additionalSize_CALL_FF, ZYDIS_MNEMONIC_CALL, Intel_checkThis_CALL_FF);


    if (error){
        err("Error adding instruction to arch->callEndings, in initJMPIntel at one of the miniBranchInstructionLinkedListAdd.");
        miniBranchInstructionLinkedListFreeRegular(arch_p->callEndings);
        arch_p->callEndings = NULL;
        return 2;
    }

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

    if(initCALLIntel(arch_p)){
        err("Error inside initArchInfo in initCALLIntel.");
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