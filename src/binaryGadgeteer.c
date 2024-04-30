#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE         /* for memmem, the bad search */

//to do stuff with the elf
#include "elfUtils.h"

//to see errors
#include "costumErrors.h"

#include "endGadgetFind.h"

#include "gadget.h" //it is contaminated

#include "gadgetLinkedList.h"

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

//for UCHAR_MAX and such.
#include <limits.h>



#define READ_AMOUNT 10000
#define DEFAULT_DEPTH 3


static uint64_t base_address_number = 0;
static char* filename = NULL;
static uint8_t depth = DEFAULT_DEPTH; 

static char const *HELP_TEXT =
    "Usage: %s [OPTIONS] BINARY_FILE ....\n"
    "Description of the program here.\n\n"
    " -a, --address 0XADDRESS           Give the start address in hex.\n"
    " -v, --version                     Display version and information and exit.\n"
    " -h, --help                        Display this help and exit.\n"
    " -o, --output outputfile.txt       Give output file.\n"
    " -d, --depth NUM                   Set depth to NUM.\n";

static char const *VERSION_TEXT = 
    "binaryGadgeteer: Version 1.0\n";


static int parseNumber(char const *number_s, uint64_t *result);

static int parseOptionAddress(char const *address_s);
static int parseOptionDepth(char const *depth_s);

static void readOptions(int argc, char **argv);

/**
 * Parses the number from string to a number.
 * can be hex(base 16), binary(base2) or decimal(base10)
 * I DO NOT SUPPORT OCTAL THIS IS THE BASE OF THE DEMONS!
 * 
 * @param number_s number in a string format.
 * @param result where the result will be saved.
 * 
 * @return 0 is success, anything else in error.
*/
static int parseNumber(char const *number_s, uint64_t *result){
    char *endptr = NULL;

    //long long Address
    if(!strncmp(number_s, "0x", 2))      //hex, base 16
        *result = strtoull(number_s+2, &endptr, 16);
    else if(!strncmp(number_s, "0b", 2)) //binary, base 2
        *result = strtoull(number_s+2, &endptr, 2);
    else                                  //decimal, base 10 (default).
        *result = strtoull(number_s, &endptr, 10);

    if(errno != 0){ //TODO: add --depth
        err("strtoull error inside parseNumber");
        return 1;
    }
    
    if (endptr == number_s){
        err("No digits were found! strtoull inside parseNumber\n");
        return 2;
    }

    if (*endptr != '\0'){
        err("unknown characters in number, ignoring. strtoull. continuing with base %llX.", *result);
    }
    return 0;
}

/**
 * Parsing base address option.
 * 
 * @param address_s the address in a string format from the user.
 * 
 * @return 0 is success, anything else in error.
 * 
 * @note touching base_address_number
*/
static int parseOptionAddress(char const *address_s){
    if (parseNumber(address_s, &base_address_number)){
        err("Error inside parseOptionAddress, at parseNumber\n");
        return 1;
    }
    return 0;
}

/**
 * Parsing depth option.
 * 
 * @param depth_s the depth in a string format from the user.
 * 
 * @return 0 is success, anything else in error.
*/
static int parseOptionDepth(char const *depth_s){
    uint64_t depthL;
    if(parseNumber(depth_s, &depthL)){
        err("Error inside parseOptionDepth, at parseNumber\n");
        return 1;
    }

    if(depthL > UCHAR_MAX){
        err("depth is too much, lowering it to 255");
        depth = 255;
    }else{
        depth = (uint8_t)depthL;
    }
    return 0;
}

static void readOptions(int argc, char **argv){
    static struct option const options[] = {
        { "address", required_argument, NULL, 'a' },
        { "help", no_argument, NULL, 'h'},
        { "version", no_argument, NULL, 'v'},
        { "output", required_argument, NULL, 'o'},
        { "depth", required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

    if (argc == 1){
        fprintf(stdout, HELP_TEXT, argv[0]);
        exit(EXIT_SUCCESS);
    }

    int n;

    while ((n = getopt_long(argc, argv, "a:hvod:", options, NULL)) != EOF){
        switch(n){
            case 'a': ;
                err("not implemented");
                exit(1);
                if (parseOptionAddress(optarg)){
                    err("Please address use as in help.");
                    fprintf(stdout, HELP_TEXT, argv[0]);
                    exit(1);
                }
                //printf("chosen base: %llX\n", base_address_number);
                break;
            case 'h':
                //help
                fprintf(stdout, HELP_TEXT, argv[0]);
                exit(0);
                break;
            case 'v':
                //version
                fputs(VERSION_TEXT, stdout);
                break;
            case 'o':
                //can get it with outarg
                err("not implemented");
                break;
            case 'd':
                if (parseOptionDepth(optarg)){
                    err("Please depth use as in help.");
                    fprintf(stdout, HELP_TEXT, argv[0]);
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Try --help for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    filename = argv[optind];
}




int main(int argc, char *argv[])
{
    //works:
    readOptions(argc, argv);

    if(initElfUtils(filename, -1)){ //"./checkMe"
        err("error inside initElfUtils at main.");
        return 1;
    }
    uint64_t entry = getEntryPoint(); //(Elf64_Addr, uint64_t)
    printf("entry: 0x%" PRIx64 "\n", entry); //TODO implement so you can control the entry point (for real, that is just changing in the segements)
    printf("arch: %s\n", getArch());

    //endianess:
    //printf("little endian(1), big(2): %d\n", getEndiannessEncoding());

    //showSectionsHeaders(); 
    //showProgramHeaders();

    char *buffer = (char *) malloc(READ_AMOUNT); //on the heap because it is super big.
    if (buffer == NULL){
        err("Error while mallocing for buffer, inside main. of size  %zu .", READ_AMOUNT);
        return 1;
    }
    
    Mini_ELF_Phdr_node *head = getAllExec_Mini_Phdr();
    if(head == NULL){
        err("Error, getAllExec_Mini_Phdr returned NULL, no program headers.");
        return 2;
    }
    //todo: do combine phdr, to combine exexutable program headers (they are suppous to be together but just in case)

    Mini_ELF_Phdr_node *curMiniHdrNode = head;
    

    //the prefix size is ZYDIS_MAX_INSTRUCTION_LENGTH * searchAheadInstructions
    size_t prefixSize = ZYDIS_MAX_INSTRUCTION_LENGTH*(depth-1); //before the offset.
    if(READ_AMOUNT < prefixSize){
        err("READ_AMOUNT has to be bigger then prefixSize. please change depth or READ_AMOUNT.");
        return 3;
    }

    //Todo actually use getArch to see the arch
    ArchInfo *arch_p; 
    if(is64Bit())
        arch_p = initArchInfo("x64");
    else
        arch_p = initArchInfo("x86");
    
    if(arch_p == NULL){
        err("Some error getting the arch info :(");
        return 4;
    }

    if(initDecoderAndFormatter(arch_p)){
        err("Error inside main, while calling initDecoderAndFormatter.");
        return 5;
    }

    

    GadgetLL *allGadgetsLL = NULL; 
 
    while(curMiniHdrNode != NULL){
        uint64_t offset = 0;
        size_t readAmount;
        for (;offset < curMiniHdrNode->cur_mini_phdr.size; offset += readAmount){
            //doing this to go a bit back for instructions that need to go a bit back. (but I don't want to go back in the first one)
            offset = prefixSize > offset ? 0 : offset - prefixSize;

            readAmount = curMiniHdrNode->cur_mini_phdr.size - offset;
            readAmount = readAmount > READ_AMOUNT ? READ_AMOUNT : readAmount;

            uint64_t buf_vaddr = curMiniHdrNode->cur_mini_phdr.vaddr + offset;
            uint64_t buf_fileOffset = curMiniHdrNode->cur_mini_phdr.file_offset + offset;
            if (readFileData( buf_fileOffset , readAmount, buffer)){
                err("Error in readFileData at main.");
                return 6;
            }

            
            GadgetLL *currentBranchGadgets = searchBranchInstructionsInBuffer(buffer, buf_vaddr, buf_fileOffset, readAmount, arch_p);
            if(currentBranchGadgets == NULL)
                continue; //NON found, continue to next one.

            
            //now here I will add them to all my gadgets

            GadgetLL *allCurGadgets = expandGadgetsDown(buffer, buf_vaddr, buf_fileOffset, readAmount, currentBranchGadgets, depth-1);
            
            //for the edge (very very edge or error) case where allCurGadgets is NULL (or none found or depth=1).
            if(allCurGadgets == NULL){
                if(allGadgetsLL == NULL){
                    allGadgetsLL = currentBranchGadgets;
                    continue;
                }
                GadgetLLCombine(allGadgetsLL, currentBranchGadgets);
                gadgetLLFreeOnly(currentBranchGadgets);
                continue;
            }

            GadgetLLCombine(allCurGadgets, currentBranchGadgets);
            gadgetLLFreeOnly(currentBranchGadgets);

            //this is checked everytime but this is more understandable so I like it more like this.
            if(allGadgetsLL == NULL){
                allGadgetsLL = allCurGadgets;
                continue;
            }
            GadgetLLCombine(allGadgetsLL, allCurGadgets);
            gadgetLLFreeOnly(allCurGadgets);
        }
        curMiniHdrNode = curMiniHdrNode->next;
    }


    //now I will go into more depth in each one.
    GadgetLLShowAll(allGadgetsLL);
    GadgetLLShowOnlyEnds(allGadgetsLL);

    //can also maybe sort by whatever with merge sort, https://www.geeksforgeeks.org/sorting-a-singly-linked-list/



    //Frees:
    gadgetLLFreeAll(allGadgetsLL);

    free(buffer);

    freeArchInfo(arch_p);
    arch_p = (ArchInfo *)NULL;

    freeAll_Mini_Phdr_nodes(head);
    head = (Mini_ELF_Phdr_node *)NULL;
    
    if(cleanElfUtils()){
        err("Error cleaning elfUtils, in cleanElfUtils inside main.\n");
    }
    //printf("finished freeing\n");

    return 0;
}
