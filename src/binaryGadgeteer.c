#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE         /* for memmem, the bad search */

//to do stuff with the elf
#include "elfUtils.h"

//to see errors
#include "costumErrors.h"

#include "endGadgetFind.h"

#include "gadget.h"

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



#define READ_AMOUNT 10000



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


static void parseOptionAddress(char const *address_s);
static void readOptions(int argc, char **argv);

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
                err("not implemented");
                exit(1);
                parseOptionAddress(optarg);
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
            default:
                fprintf(stderr, "Try --help for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    filename = argv[optind];
}


/**
 * Searching for all occurnces of searchBytes in buffer.
 * 
 * @param buffer, the buffer that is searched in.
 * @param bufferSize, the size of the buffer that is searched.
 * @param searchBytes, the bytes to search for in the buffer.
 * @param amountOfBytes, the amount of bytes to search.
 * 
 * @note not the most efficent way
*/
/*
I need to implement an algorithm that will do the following:
Search for m1, m2, m3,..., mk (bytes/string) in S(bytes/string).
in an effective way.
*/
/*
foundLocationsNode* searchInBuffer(char *buffer, uint64_t bufferSize, char *searchBytes, uint64_t amountOfBytes){

}*/




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

    Mini_ELF_Phdr_node *curMiniHdrNode = head;
    
    //until I read cur->size
    //TODO: add some zeros to the right and left on the start (first read)
    // (and take from last one) ZYDIS_MAX_INSTRUCTION_LENGTH
    
    //implement this later!
    #define searchAheadInstructions 2
    size_t prefixSize = ZYDIS_MAX_INSTRUCTION_LENGTH*searchAheadInstructions; //before the offset.

    //Todo actually use getArch to see the arch
    ArchInfo *arch_p; 
    if(is64Bit())
        arch_p = initArchInfo("x64");
    else
        arch_p = initArchInfo("x86");
    
    if(arch_p == NULL){
        err("Some error getting the arch info :(");
        return 3;
    }

    ZydisDecoder decoder; //decoder
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, arch_p->machine_mode, arch_p->stack_width))){
        err("some error initiating decoder.");
        return 4;
    }
    
    //the style of the assembly:
    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    initDecoderAndFormatter(&decoder, &formatter);
    
    gadgetGeneralNode *allGadgetsGeneralNodeStart = gadgetGeneralNodeCreate((gadgetGeneral) {.addr_file=0, .checked=true, .length=0, .vaddr=0, .first=NULL});
    if (allGadgetsGeneralNodeStart == NULL){
        err("Error while getting allGadgetsGeneralNodeStart at gadgetGeneralNodeCreate in main.");
        return 5;
    }
    gadgetGeneralLinkedListEnds allGadgetsGeneralEnds = {.start=allGadgetsGeneralNodeStart, .end=allGadgetsGeneralNodeStart};

    //TODO: implement so it will go a little back when checking! MUST!
    while(curMiniHdrNode != NULL){
        //zeroing it out so it wouldnt say there is an instruction that there isn't
        //offset from beggening
        uint64_t offset = 0;
        size_t readAmount;
        for (;offset < curMiniHdrNode->cur_mini_phdr.size; offset += readAmount){
            readAmount = curMiniHdrNode->cur_mini_phdr.size - offset;
            readAmount = readAmount > READ_AMOUNT ? READ_AMOUNT : readAmount;

            uint64_t buf_vaddr = curMiniHdrNode->cur_mini_phdr.vaddr + offset;
            uint64_t buf_fileOffset = curMiniHdrNode->cur_mini_phdr.file_offset + offset;
            //printf("reading.\n");
            if (readFileData( buf_fileOffset , readAmount, buffer)){
                err("error in readFileData at main.");
                return 6;
            }

            
            //foundLocations = searchInBuffer()
            //printf("now location: 0x%" PRIx64 " which is offset 0x%" PRIx64 ";\n", buf_vaddr, offset);
            FoundLocationsBufferNode *locations = searchRetInBuffer(buffer, readAmount, arch_p);
            if(locations == NULL)
                continue; //NON found, continue to next one.
            
            //now here I will add them to all my gadgets

            //printf("woho found :)");
            //printf("cur: 0x%" PRIx64 "\n", buf_vaddr);
            FoundLocationsBufferNode *curBranchInstructionLocation = locations;

            //int debugDELLATER = 0;
            for(; curBranchInstructionLocation != NULL; curBranchInstructionLocation = curBranchInstructionLocation->next){
                //if(++debugDELLATER == 5)
                //    exit(90);
                //printf("%d\n", debugDELLATER);
                
                gadgetGeneralLinkedListEnds curGadgetsGNodeEnds = expandInstructionDown(buffer, buf_vaddr, buf_fileOffset, readAmount, curBranchInstructionLocation, ZYDIS_MNEMONIC_RET, 3);
                if (curGadgetsGNodeEnds.start == NULL){
                    err("Error in expandInstructionDown, into curGadgetGeneralNode inside main.");
                    return 7;
                }

                allGadgetsGeneralEnds.end->next = curGadgetsGNodeEnds.start;
                allGadgetsGeneralEnds.end = curGadgetsGNodeEnds.end;

                //printf("showing:\n");
                //gadgetGeneralNodeShowAll(curGadgetsGNodeEnds.start);
            }
            FoundLocationsBufferNodeFree(locations);
        }
        curMiniHdrNode = curMiniHdrNode->next;
    }
    
    allGadgetsGeneralEnds.start = allGadgetsGeneralEnds.start->next;
    gadgetGeneralNodeFreeCurrent(allGadgetsGeneralNodeStart);
    if(allGadgetsGeneralEnds.end == allGadgetsGeneralNodeStart){
        //if it is equal it means no gadgets are found (WTF) but checking just in case.
        allGadgetsGeneralEnds.end = NULL;
    }


    gadgetGeneralNodeShowOnlyEnds(allGadgetsGeneralEnds.start);

    //printf("freeing\n");
    gadgetGeneralNodeFreeAll(allGadgetsGeneralEnds.start);
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
