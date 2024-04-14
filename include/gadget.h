#pragma once


typedef struct gadget{
    instruction inst;
    size_t depth;
    struct gadget *next;
    //? startLocation;
    //? length
} gadget;


/*
for each instruction in the ropgadget I want to save:
(some stuff lile ZydisDecodedInstruction_)
1) ZydisMnemonic mnemonic ; instruction (to know which instruction it is)
2) ZyanU8 length
3) (I need to save the params that it gave it somehow, the operands) 

*/