#define _FILE_OFFSET_BITS 64

//to do stuff with the elf
#include "elfUtils.h"

//to see errors
#include "costumErrors.h"

//to print
#include <stdio.h>

//for some absured zydis types
#include <inttypes.h>

//to decompile
#include <Zydis/Zydis.h>

#include <zydis/Decoder.h>
#include <zydis/Formatter.h>

//to get options from terminal
#include <getopt.h>

//to strerror and such
//#include <string.h>

//to malloc, free and such
#include <unistd.h>

//for true and false
#include <stdbool.h>

//for tolower() in the parsing of the input
#include <ctype.h>



static unsigned long long base_address_number = 0;
static char* filename = NULL;

static char const *HELP_TEXT =
    "Usage: %s [OPTIONS] BINARY_FILE ....\n"
    "Description of the program here.\n\n"
    " -a, --address 0XADDRESS           Give the start address in hex.\n"
    " -v, --version                     Display version and information and exit.\n"
    " -h, --help                        Display this help and exit.\n"
    " -o, --output outputfile.txt       Give output file.\n";

static char const *VERSION_TEXT = 
    "binaryGadgeteer: Version 1.0\n";


static void parseOptionAddress(char const *address_s){
    char *endptr = NULL;

    //long long Address
    if(!strncmp(address_s, "0x", 2))      //hex, base 16
        base_address_number = strtoull(address_s+2, &endptr, 16);
    else if(!strncmp(address_s, "0b", 2)) //binary, base 2
        base_address_number = strtoull(address_s+2, &endptr, 2);
    else                                  //decimal, base 10 (default).
        base_address_number = strtoull(address_s, &endptr, 10);

    if(errno != 0){
        err("strtoull error inside parseOptionAddress");
        exit(EXIT_FAILURE);
    }
    
    if (endptr == address_s){
        err("No digits were found! strtoull\n");
        exit(EXIT_FAILURE);
    }

    if (*endptr != '\0'){
        err("unknown characters in number, ignoring. strtoull. continuing with base %llX.", base_address_number);
    }
}

static void readOptions(int argc, char **argv){
    static struct option const options[] = {
        { "address", required_argument, NULL, 'a' },
        { "help", no_argument, NULL, 'h'},
        { "version", no_argument, NULL, 'v'},
        { "output", required_argument, NULL, 'o'},
        {0, 0, 0, 0}
    };

    if (argc == 1){
        fprintf(stdout, HELP_TEXT, argv[0]);
        exit(EXIT_SUCCESS);
    }

    int n;

    while ((n = getopt_long(argc, argv, "a:hvo:", options, NULL)) != EOF){
        switch(n){
            case 'a': ;
                parseOptionAddress(optarg);
                //printf("chosen base: %llX\n", base_address_number);
                break;
            case 'h':
                //help
                fprintf(stdout, HELP_TEXT, argv[0]);
                break;
            case 'v':
                //version
                fputs(VERSION_TEXT, stdout);
                break;
            case 'o':
                //can get it with outarg
                err("not implemented");
                break;
            default:
                fprintf(stderr, "Try --help for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    filename = argv[optind];
}


//TODO: delete this
#define BUFFER_SIZE 1024


int main(int argc, char *argv[])
{
    readOptions(argc, argv);

    printf("starting\n");
    if(initElfUtils("./checkMe", -1)){
        err("error inside initElfUtils at main.");
        return 1;
    }
    unsigned long long entry = getEntryPoint();
    printf("hello %llx \n", entry);

    entry = getEntryPoint();
    printf("hello %llx \n", entry);
    printf("%s\n", getArch());

    //endianess:
    //printf("little endian(1), big(2): %d\n", getEndiannessEncoding());

    //showSectionsHeaders(); //X showScanSections();
    showProgramHeaders();

    printf("getting:\n");

    char buffer[1000];

    mini_ELF_Phdr_node *head = getAllExec_mini_Phdr();
    if(head == NULL){
        err("getAllExec_mini_Phdr returned NULL, no program headers.");
        return 1;
    }

    mini_ELF_Phdr_node *cur = head;
    while(cur != NULL){
        //until I read cur->size
        //TODO: add some zeros to the right and left on the start (first read)
        // (and take from last one) ZYDIS_MAX_INSTRUCTION_LENGTH

        if (readFileData(cur->cur_mini_phdr.file_offset, 1000, &buffer)){
            err("error in readFileData at main.");
            return 2;
        }
        char prefix[ZYDIS_MAX_INSTRUCTION_LENGTH]; //before the offset.
        char suffix[ZYDIS_MAX_INSTRUCTION_LENGTH]; //after the offset
        
        //uint64_t cur


    }
    
    



    printf("freeing:\n");
    freeAll_mini_Phdr_nodes(head);
    if(cleanElfUtils()){
        err("Error cleaning elfUtils, in cleanElfUtils inside main.\n");
    }

    exit(0);
    ZydisEncoderRequest req;
    memset(&req, 0, sizeof(req));

    /*
    req.mnemonic = ZYDIS_MNEMONIC_MOV;
    req.machine_mode = ZYDIS_MACHINE_MODE_LONG_64; //find a way to now write that each time maybe?
    req.operand_count = 2; 
    req.operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
    req.operands[0].reg.value = ZYDIS_REGISTER_RAX;
    req.operands[1].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    req.operands[1].imm.u = 0x1337;
    */

   

    req.mnemonic = ZYDIS_MNEMONIC_RET;
    req.machine_mode = ZYDIS_MACHINE_MODE_LEGACY_32;
    req.operand_count = 0;
    ZyanU8 encoded_instruction[ZYDIS_MAX_INSTRUCTION_LENGTH];
    ZyanUSize encoded_length = sizeof(encoded_instruction);

    if (ZYAN_FAILED(ZydisEncoderEncodeInstruction(&req, encoded_instruction, &encoded_length))){
        puts("Filed to encode instruction :(");
        return 1;
    }

    puts("Encoded insturction sucessfully :)");
    for (ZyanUSize i = 0; i < encoded_length; i++){
        printf("%02X ", encoded_instruction[i]);
    }
    puts("");

    return 0;

    //readOptions(argc, argv);

    //printf("binary file: %s\n", argv[optind]);
    //printf("chosen base: %llX\n", base_address_number);


    /*
    I will search for RET by assembeling a RET with the spesific things.

    TODO:
    1) write a function to get the architecture
    2) write a function to get 
    3)


    how do I get the stack width?
    https://doc.zydis.re/v4.0.0/html/group__decoder#ga5448746153e38c32f81dcdc73f177002
    https://doc.zydis.re/v4.0.0/html/SharedTypes_8h#aa771500a3715933bf22eb84285ab6b6e

    */
    
    // printf("Version: %16"PRIX64" \n", 	ZydisGetVersion());
    // printf("%ld\n",sizeof(ZyanU64));
    // printf("%ld\n",sizeof(ZyanU8));
    // printf("%d\n", ZYDIS_MACHINE_MODE_LONG_64);
    // printf("here:%s\n\n", PRIX64);
    // printf("here:%s\n\n", PRIX32);

    // ZyanU8 data[] =
    // {
    //     0x51, 0x8D, 0x45, 0xFF, 0x50, 0xFF, 0x75, 0x0C, 0xFF, 0x75,
    //     0x08, 0xFF, 0x15, 0xA0, 0xA5, 0x48, 0x76, 0x85, 0xC0, 0x0F,
    //     0x88, 0xFC, 0xDA, 0x02, 0x00
    // };

    // // The runtime address (instruction pointer) was chosen arbitrarily here in order to better
    // // visualize relative addressing. In your actual program, set this to e.g. the memory address
    // // that the code being disassembled was read from.
    // ZyanU64 runtime_address = 0x007FFFFFFF400000;

    // // Loop over the instructions in our buffer.
    // ZyanUSize offset = 0;
    // ZydisDisassembledInstruction instruction;
    // while (ZYAN_SUCCESS(ZydisDisassembleIntel(
    //     /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
    //     /* runtime_address: */ runtime_address,
    //     /* buffer:          */ data + offset,
    //     /* length:          */ sizeof(data) - offset,
    //     /* instruction:     */ &instruction
    // ))) {
    //     printf("%016" PRIX64 "  %s\n", runtime_address, instruction.text);
    //     offset += instruction.info.length;
    //     runtime_address += instruction.info.length;
    // }

    

    return 0;
}
