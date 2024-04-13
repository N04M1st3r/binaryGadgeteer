#include <inttypes.h>


/*
MiniInstruction ideas to improve and explanation
note: can maybe get rid of mnemonicOpcode but there is no must that a mnemonicOpcode will not contain 0

maybe add instructionPrefix to there.

mnemonicOpcode can also be looked at as startOpcode.

it is less space to save 3 bytes instead of 4/8 for a pointer. (and a little faster and cleaner)

mnemonicOpcode is not ending with \x00!!! it only saves data!!
Example 1:
    CA iw(word)       RET        Far  return

    will be saved like so:
    {{0xCA, 0, 0}, 1, 2}

    (there is no promise that they will contain 0 in mnemonicOpcode!)
    {{0xCA, ?, ?}, 1, 2}

Example 2:
    C3                RET        Near return
    
    will be saved as:
    {{0xC3, 0, 0}, 1, 0};

*/
typedef struct MiniInstruction{ //saving the minimum info needed on the instruction
    char mnemonicOpcode[3];       /*Max mnemonicOpcode/startOpcode is 3 (according to intel manual), start opcode*/
    uint8_t mnemonicOpcodeSize;
    uint8_t additionSize;
} MiniInstruction;

typedef struct MiniInstructionNode{
    MiniInstruction instructionInfo;
    struct MiniInstructionNode *next;
} MiniInstructionNode;

typedef struct MiniInstructionLinkedList{
    MiniInstructionNode *start;
    MiniInstructionNode *end;
    size_t size;
} MiniInstructionLinkedList;