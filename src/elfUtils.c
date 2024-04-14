/**
 * @author N0am
 * @date TODO
 * @name elfUtils.c
*/
//this will force it to be 64 bits
#define _FILE_OFFSET_BITS 64

#include "costumErrors.h"
#include "elfUtils.h"

#include <elf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <inttypes.h>

#define INDENT_STR "  "

static void copyElf32_EhdrToElf64_Ehdr(Elf32_Ehdr *, Elf64_Ehdr *);
static void copyElf32_ShdrToElf64_Shdr(Elf32_Shdr *, Elf64_Shdr *);
static void copyElf32_PhdrToElf64_Phdr(Elf32_Phdr *, Elf64_Phdr *);
static int setupElf_ehdr(void);
static int setupBits(void);
static int initStringTable(void);

static const char *getSectionName(Elf64_Shdr *sectionHdrP);
static const char *getProgramHdrType(Elf64_Phdr *programHdrP);


static void showSectionHdrFlags(Elf64_Shdr *sectionHdrP);
static void showProgramHdrFlags(Elf64_Phdr *programHdrP);


static int getSectionHdrByOffset(Elf64_Shdr *result_Shdr, Elf64_Off sectionHdrOff);
static int getProgramHdrByOffset(Elf64_Phdr *result_Phdr, Elf64_Off programHdrOff);

static void showSectionHdr(Elf64_Off sectionHdrOff);
static void showProgramHdr(Elf64_Off programHdrOff);

static int getSectionHdrByIndex(Elf64_Shdr *result_Shdr, Elf64_Xword section_index);

static uint64_t getSectionsCount(void);
static uint64_t getProgramHeadersCount(void);


//BAD: DEL LATER XD
#define SUPPRESS_WARNING(a) (void)a


/**
 * This will give info and things to do with the elf, 
 * it works on one elf at a time. (because that is what I need)
 * source: http://www.skyfree.org/linux/references/ELF_Format.pdf
 * 
 * for a another implemenation you may look at elfcommon.c in binutils
*/


/*
  note that can save all/most these globals into a struct and in memory (passing a pointer),
  but I have only one instance of this so there is no need,
  in my opinion it will be more readable like what it is now.
*/


//to know if to use elf64_hdr or elf32_hdr and such..
bool is64BitElf;

//this saves both 32 and 64 elf headers.
static Elf64_Ehdr elf_Ehdr;

FILE *file = NULL;

//static char *stringTable = NULL;
static struct{
  Elf64_Shdr Shdr;
  char *data;
} stringTable = {.data = NULL};





//maybe save the String Table here.
//costum struct for "String table", it is pretty naive but I will just 
//struct 



/**
 * Copies all values in elf32_Ehdr_p to elf64_Ehdr_p.
*/
static void copyElf32_EhdrToElf64_Ehdr(Elf32_Ehdr *elf32_Ehdr_p, Elf64_Ehdr *elf64_Ehdr_p){
  //elf64->e_ident = elf32->e_ident;
  
  //EI_NIDENT = sizeof(Elf32_Ehdr.e_ident) == sizeof(Elf64_Addr.e_ident) == 16
  memcpy(&(elf64_Ehdr_p->e_ident), &(elf32_Ehdr_p->e_ident), EI_NIDENT);
  elf64_Ehdr_p->e_type = elf32_Ehdr_p->e_type;
  elf64_Ehdr_p->e_machine = elf32_Ehdr_p->e_machine;
  elf64_Ehdr_p->e_version = elf32_Ehdr_p->e_version;
  elf64_Ehdr_p->e_entry = elf32_Ehdr_p->e_entry;
  elf64_Ehdr_p->e_phoff = elf32_Ehdr_p->e_phoff;
  elf64_Ehdr_p->e_shoff = elf32_Ehdr_p->e_shoff;
  elf64_Ehdr_p->e_flags = elf32_Ehdr_p->e_flags;
  elf64_Ehdr_p->e_ehsize = elf32_Ehdr_p->e_ehsize;
  elf64_Ehdr_p->e_phentsize = elf32_Ehdr_p->e_phentsize;
  elf64_Ehdr_p->e_phnum = elf32_Ehdr_p->e_phnum;
  elf64_Ehdr_p->e_shentsize = elf32_Ehdr_p->e_shentsize;
  elf64_Ehdr_p->e_shnum = elf32_Ehdr_p->e_shnum;
  elf64_Ehdr_p->e_shstrndx = elf32_Ehdr_p->e_shstrndx;
}

/**
 * Copies all values in elf32_Shdr_p to elf64_Shdr_p.
*/
static void copyElf32_ShdrToElf64_Shdr(Elf32_Shdr *elf32_Shdr_p, Elf64_Shdr *elf64_Shdr_p){
  elf64_Shdr_p->sh_name = elf32_Shdr_p->sh_name;
  elf64_Shdr_p->sh_type = elf32_Shdr_p->sh_type;
  elf64_Shdr_p->sh_flags = elf32_Shdr_p->sh_flags;
  elf64_Shdr_p->sh_addr = elf32_Shdr_p->sh_addr;
  elf64_Shdr_p->sh_offset = elf32_Shdr_p->sh_offset;
  elf64_Shdr_p->sh_size = elf32_Shdr_p->sh_size;
  elf64_Shdr_p->sh_link = elf32_Shdr_p->sh_link;
  elf64_Shdr_p->sh_info = elf32_Shdr_p->sh_info;
  elf64_Shdr_p->sh_addralign = elf32_Shdr_p->sh_addralign;
  elf64_Shdr_p->sh_entsize = elf32_Shdr_p->sh_entsize;
}


/**
 * Copies all values in elf32_Phdr_p to elf64_Phdr_p.
*/
static void copyElf32_PhdrToElf64_Phdr(Elf32_Phdr *elf32_Phdr_p, Elf64_Phdr *elf64_Phdr_p){
  elf64_Phdr_p->p_type   = elf32_Phdr_p->p_type;
  elf64_Phdr_p->p_offset = elf32_Phdr_p->p_offset;
  elf64_Phdr_p->p_vaddr = elf32_Phdr_p->p_vaddr;
  elf64_Phdr_p->p_paddr = elf32_Phdr_p->p_paddr;
  elf64_Phdr_p->p_filesz = elf32_Phdr_p->p_filesz;
  elf64_Phdr_p->p_memsz = elf32_Phdr_p->p_memsz;
  elf64_Phdr_p->p_flags = elf32_Phdr_p->p_flags;
  elf64_Phdr_p->p_align = elf32_Phdr_p->p_align;
}


/**
 * Initilizes elf_Ehdr. (32 or 64 depended on is64BitElf)
 * 
 * @return sucess, 0 if no errors, else 1 or such if there were errors.
*/
static int setupElf_ehdr(void){
  if (fseek(file, 0, SEEK_SET) != 0){
    err("Error in fseek at setupElf_ehdr");
    return 1;
  }

  if(!is64BitElf){
    Elf32_Ehdr elf32_Ehdr_tmp;
    if (fread(&elf32_Ehdr_tmp, 1, sizeof(Elf32_Ehdr), file) != sizeof(Elf32_Ehdr)){
      err("Couldnt read elf header, the file spesification. in setupElf_ehdr");
      return 2;
    }
    //TODO: fix endian here if needed.

    copyElf32_EhdrToElf64_Ehdr(&elf32_Ehdr_tmp, &elf_Ehdr);
  }
  else {
    if (fread(&elf_Ehdr, 1, sizeof(Elf64_Ehdr), file) != sizeof(Elf64_Ehdr)){
      err("Couldnt read elf header, the file spesification. in setupElf_ehdr");
      return 2;
    }
    //TODO: fix endian here if needed.
  }
  
  return 0;
}

/**
 * Initilizes is64BitElf.
 * 
 * @return sucess, 0 if no errors, else 1 or such if there were errors.
*/
static int setupBits(void){
  if (fseek(file, 0, SEEK_SET) != 0){
    err("Error in fseek at setupBits");
    return 1;
  }
  unsigned char e_ident[EI_NIDENT];
  if (fread(e_ident, sizeof(char), EI_NIDENT, file) != EI_NIDENT){
    err("Couldnt read indent, in setupBits at fread.");
    return 2;
  }

  //Step 1: Checking on indent:  
  //checking that it starts with \x7fELF
  if (!(e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1 && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3)){
    err("File is not ELF. Error in setupBits");
    return 3;
  }
  switch(e_ident[EI_CLASS]){
    case ELFCLASS32:
      is64BitElf = false;
      break;
    case ELFCLASS64:
      is64BitElf = true;
      break;
    case ELFCLASSNONE:
      err("File is ELF of not 64 or 32 bit, class NONE? Error in setupBits");
      return 4;
      break;
    default:
      err("ELF class in not in the options. ERROR in setupBits");
      return 4;
      break;
  }
  return 0;
}




/**
 * Gets the section name from String table using Elf32_Shdr.sh_name
 * 
 * @param sectionHdrP, section header pointer.
 * 
 * @return section name (pointer).
 * 
 * @note in the string table the section name needs to end with \0 (room for vulnerability btw)
*/
static const char *getSectionName(Elf64_Shdr *sectionHdrP){
  return stringTable.data + sectionHdrP->sh_name;
}

static void FIX_UNUSED_DEL(void){
  //DEL LATER!
  getEndiannessEncoding();
  
}

/**
 * Initilizes the stringTable struct with the values from the file as needed.
 * 
 * @return 0 if sucess, else something else.
 * 
 * @note using malloc into stringTable.data!
*/
static int initStringTable(void){
  if (getSectionHdrByIndex(&stringTable.Shdr, elf_Ehdr.e_shstrndx)){
    err("Error in getSectionHdrByIndex at InitStringTable.\n");
    return 1;
  }

  stringTable.data = (char *) malloc(stringTable.Shdr.sh_size);
  if(stringTable.data == NULL){
    // Malloc failed
    err("Malloc for stringTable.data failed inside initStringTable.\n");
    return 2;
  }
  
  if (fseek(file, stringTable.Shdr.sh_offset, SEEK_SET)){
    err("Error in fseek at InitStringTable, seeking to 0x%" PRIx64 "\n", stringTable.Shdr.sh_offset);
    return 3;
  }

  if (fread(stringTable.data, 1, stringTable.Shdr.sh_size, file) != stringTable.Shdr.sh_size){
    err("Error in fread at InitStringTable, raeding to 0x%" PRIx64 "from 0x% " PRIX64 "\n", stringTable.data, stringTable.Shdr.sh_size);
    return 4;
  }

  return 0;
}


/**
 * Gets the section header at file offset sectionHdrOff of size elf_Ehdr.e_shentsize, into result_Shdr.
 * 
 * @param result_Shdr, where the resulting section header will go.
 * @param sectionHdrOff,  section header offset in the file.
 * 
 * @return 0 is sucess, anything else if error.
 * 
 * @note if 32 bits will make it fit to the 64 one, assuming result_Shdr is already allocated space.
*/
static int getSectionHdrByOffset(Elf64_Shdr *result_Shdr, Elf64_Off sectionHdrOff){
  if (fseek(file, sectionHdrOff, SEEK_SET) != 0){
    err("Error in fseek at getSectionHdrByOffset");
    return 1;
  }

  if(!is64BitElf){
    //32 bits
    Elf32_Shdr elf32_Shdr_tmp;
    if (fread(&elf32_Shdr_tmp, 1, elf_Ehdr.e_shentsize, file) != elf_Ehdr.e_shentsize){
      err("Couldnt read section header, the file specification. in getSectionHdrByOffset at offset 0x%" PRIx64 " and size 0x%" PRIx64, sectionHdrOff, elf_Ehdr.e_shentsize);
      return 2;
    }

    copyElf32_ShdrToElf64_Shdr(&elf32_Shdr_tmp, result_Shdr);
  }else{
    //64 bits
    if (fread(result_Shdr, 1, elf_Ehdr.e_shentsize, file) != elf_Ehdr.e_shentsize){
      err("Couldnt read section header, the file specification. in getSectionHdrByOffset at offset 0x%" PRIx64 " and size 0x%" PRIx64, sectionHdrOff, elf_Ehdr.e_shentsize);
      return 2;
    }
  }

  return 0;
}


/**
 * Gets the program header at file offset programHdrOff of size elf_Ehdr.e_phentsize, into result_Phdr.
 * 
 * @param result_Phdr, where the resulting program header will go.
 * @param programHdrOff,  program header offset in the file.
 * 
 * @return 0 is sucess, anything else if error.
 * 
 * @note if 32 bits will make it fit to the 64 one, assuming result_Phdr is already allocated space.
*/
static int getProgramHdrByOffset(Elf64_Phdr *result_Phdr, Elf64_Off programHdrOff){
  if (fseek(file, programHdrOff, SEEK_SET) != 0){
    err("Error in fseek at getProgramHdrByOffset");
    return 1;
  }

  if(!is64BitElf){
    //32 bits
    Elf32_Phdr elf32_Phdr_tmp;
    if (fread(&elf32_Phdr_tmp, 1, elf_Ehdr.e_phentsize, file) != elf_Ehdr.e_phentsize){
      err("Couldn't read program header, the file specification. in getProgramHdrByOffset at offset 0x%" PRIx64 " and size 0x%" PRIx64, programHdrOff, elf_Ehdr.e_phentsize);
      return 2;
    }

    copyElf32_PhdrToElf64_Phdr(&elf32_Phdr_tmp, result_Phdr);
  }else{
    //64 bits
    if (fread(result_Phdr, 1, elf_Ehdr.e_phentsize, file) != elf_Ehdr.e_phentsize){
      err("Couldn't read program header, the file specification. in getProgramHdrByOffset at offset 0x%" PRIx64 " and size 0x%" PRIx64, programHdrOff, elf_Ehdr.e_phentsize);
      return 2;
    }
  }

  return 0;
}


/**
 * Gets the section header of the section_index'th section from the section table array 
 * it is of size elf_Ehdr.e_shentsize, into result_Shdr.
 * 
 * 
 * @param result_Shdr, where the resulting section header will go.
 * @param section_index,  section header index in the section table array.
 * 
 * @return 0 is sucess, anything else if error.
 * 
 * @note if 32 bits will make it fit to the 64 one, assuming result_Shdr is already allocated space.
*/
static int getSectionHdrByIndex(Elf64_Shdr *result_Shdr, Elf64_Xword section_index){

  //TODO: check if the section index is after the size of the sections here. (so no overflow read kinda)
  Elf64_Off sectionHdrOff = elf_Ehdr.e_shoff + elf_Ehdr.e_shentsize * section_index;

  if(getSectionHdrByOffset(result_Shdr, sectionHdrOff) != 0){
    err("Error inside getSectionHdrByIndex, while calling getSectionHdrByOffset index 0x%" PRIx64 "\n", section_index);
    return 1;
  }

  return 0;
}

/**
 * Prints the section header at sectionHdrOff of size sectionHdrSize info into stdout.
 * 
 * @param sectionHdrOff,  section header offset in the file.
 * 
 * @note if error writes there was an error, does not return anything special.
*/
static void showSectionHdr(Elf64_Off sectionHdrOff){
  Elf64_Shdr section;
  if(getSectionHdrByOffset(&section, sectionHdrOff)){
    err("Error in showSectionHdr in getSectionHdrByOffset. section header offset: 0x%" PRIx64 ", section header size: 0x%" PRIx16 "\n", sectionHdrOff, elf_Ehdr.e_shentsize);
    return;
  }

  if(!(section.sh_flags & SHF_EXECINSTR))
    return;

  //uint64_t address = elf_Ehdr.e_shstrndx;
  //address += section.sh_name;
  //printf("location: 0x%" PRIx64 "\n", address);
  printf("virtual addr: 0x%" PRIx64 "\n", section.sh_addr);
  printf("name: %s\n", getSectionName(&section));
  printf("file offset: 0x%" PRIx64 "\n", section.sh_offset);
  printf("size: 0x%" PRIx64 "\n", section.sh_size);
  printf("link: 0x%" PRIx32 "\n", section.sh_link);

  printf("info: 0x%" PRIx32 "\n", section.sh_info);
  printf("addralign: 0x%" PRIx64 "\n", section.sh_addralign);
  printf("entsize: 0x%" PRIx64 "\n", section.sh_entsize);

  printf("type: 0x%" PRIx32 "\n", section.sh_type);


  showSectionHdrFlags(&section);

  printf("\n");
}



/**
 * Prints the program header's flags in a nice way.
 * 
 * @param programP, the program's pointer.
 * 
 * this like file premmision in linux,
 * 1 for exec       (PF_X)
 * 2 for write      (PF_W)
 * 4 for read       (PF_R)
 * 
 * 0xf0000000 for Unspecified (PF_MASKPROC)
*/
static void showProgramHdrFlags(Elf64_Phdr *programHdrP){
  printf("flags(0x%" PRIx32 "): ", programHdrP->p_flags);

  if (programHdrP->p_flags & PF_X)
    printf(INDENT_STR "Executable");
  
  if (programHdrP->p_flags & PF_W)
    printf(INDENT_STR "Write");
  
  if (programHdrP->p_flags & PF_R)
    printf(INDENT_STR "Read");

  if (programHdrP->p_flags == PF_MASKPROC)
    printf(INDENT_STR "Unspecified");

  printf("\n");
}

/**
 * return the program header's type in char* in a nice way.
 * 
 * @param programP, the program's pointer.
 * 
 * @return name of type of program header (char*)
 * 
*/
static const char *getProgramHdrType(Elf64_Phdr *programHdrP){
  
  char *typeStr;
  switch(programHdrP->p_type){
    case PT_NULL:
      typeStr = "NULL";
      break;
    case PT_LOAD:
      typeStr = "LOAD";
      break;
    case PT_DYNAMIC:
      typeStr = "DYNAMIC";
      break;
    case PT_INTERP:
      typeStr = "INTERP";
      break;
    case PT_NOTE:
      typeStr = "NOTE";
      break;
    case PT_SHLIB:
      typeStr = "SHLIB";
      break;
    case PT_PHDR:
      typeStr = "PHDR";
      break;
    case PT_LOSUNW: //same as PT_SUNWBSS
      typeStr = "LOSUNW/PT_SUNWBSS";
      break;
    case PT_SUNWSTACK:
      typeStr = "SUNWSTACK";
      break;
    case PT_HISUNW:
      typeStr = "HISUNW";
      break;
    case PT_LOPROC: // values from PT_LOPROC to PT_HIPROC are processor-specific semantics
      typeStr = "LOPROC";
      break;
    case PT_HIPROC:
      typeStr = "HIPROC";
      break; //from here some processor-specific
    case PT_GNU_PROPERTY:
      typeStr = "GNU_PROPERTY";
      break;
    case PT_GNU_EH_FRAME:
      typeStr = "GNU_EH_FRAME";
      break;
    case PT_GNU_RELRO:
      typeStr = "GNU_RELRO";
      
      break;
    case PT_GNU_STACK:
      typeStr = "GNU_STACK";
      break;
    default:
      typeStr = "Unknown";
      if(programHdrP->p_type > PT_LOPROC && programHdrP->p_type < PT_HIPROC)
        typeStr = "Unknown processor-specific semantics";
      break;
  }
  return typeStr;
}


/**
 * Prints the section header's flags in a nice way.
 * 
 * @param sectionP, the section's header pointer.
 * 
*/
static void showSectionHdrFlags(Elf64_Shdr *sectionP){
  printf("flags:");

  //sh_flags
  if ( sectionP->sh_flags & SHF_EXECINSTR )
    printf(INDENT_STR "Executable");

  if ( sectionP->sh_flags & SHF_WRITE)
    printf(INDENT_STR "Writable");

  if ( sectionP->sh_flags & SHF_ALLOC)
    printf(INDENT_STR "Allocatable");
  
  if ( sectionP->sh_flags & SHF_MERGE)
    printf(INDENT_STR "Merge");

  if ( sectionP->sh_flags & SHF_STRINGS)
    printf(INDENT_STR "SHF_STRINGS");
  
  if ( sectionP->sh_flags & SHF_LINK_ORDER)
    printf(INDENT_STR "SHF_LINK_ORDER");
  
  if ( sectionP->sh_flags & SHF_OS_NONCONFORMING)
    printf(INDENT_STR "SHF_OS_NONCONFORMING(WTF HOW)");

  if ( sectionP->sh_flags & SHF_GROUP)
    printf(INDENT_STR "GROUP(wow)");
  
  if ( sectionP->sh_flags & SHF_TLS)
    printf(INDENT_STR "SHF_TLS(WTF)");

  if ( sectionP->sh_flags & SHF_MASKOS)
    printf(INDENT_STR "MASKOS(WHAT?)");
  
  if ( sectionP->sh_flags & SHF_ORDERED)
    printf(INDENT_STR "ORDERED(WHAT?)");
  
  if ( sectionP->sh_flags & SHF_EXCLUDE)
    printf(INDENT_STR "EXCLUDE(!!)");
  
  if ( sectionP->sh_flags & SHF_MASKPROC)
    printf(INDENT_STR "MASKPROC(WTF)");
  
  printf("\n");
}


/**
 * Initilizes the elf header into this Utils, this can work on one file at a time.
 * 
 * Initilizes everything. 
 * 
 * @param filename      Filename of the elf.
 * @param entryP        Entry point, ULLONG_MAX(0xffff_ffff_ffff_ffff) if None (detect here).
 * 
 * @return sucess, 0 if no errors, another if there were errors.
*/
int initElfUtils(char const *filename, unsigned long long entryP){
  if ((file = fopen(filename, "rb")) == NULL){ //TODO CHANGE TO rb FROM rb+!!!!!
    err("Error opening the file in initElfUtils");
    return 1;
  }

  //checks if the ELF is correct and sets if its 64 bit.
  if(setupBits()){
    err("error in initElfUtils at setupBits.");
    return 2;
  }


  if(setupElf_ehdr()){
    err("error in initElfUtils at setupElf_ehdr");
    return 3;
  }

  if (initStringTable()){
    err("Error in initElfUtils at initStringTable");
    return 4;
  }

  //can add a check to see if elf header size is e_ehsize.

  //maybe I should do something with the EI_DATA?

  if (entryP != ULLONG_MAX){
      elf_Ehdr.e_entry = entryP;
  }

  return 0;
  FIX_UNUSED_DEL(); //DEL LATER!
}

/**
 * 
 * Returns the architecture of the file.
 * 
 * @return architecture of the file.
 * can return: "AT&T WE 32100", "SPARC", "Intel 80386", "Motorola 68000", "Motorola 88000", "Intel 80860", "MIPS RS3000"
 * if None then return "None", if unknown will return "Unknown"
 * 
 * @note YOU HAVE TO CALL initElfUtils before calling this, and it to be doen with no errors.
 * @note can make larger with this https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html
*/
const char *getArch(void){
  Elf32_Half machine = elf_Ehdr.e_machine;
  
  switch(machine){
    case EM_M32:
      return "AT&T WE 32100";
    case EM_SPARC:
      return "SPARC";
    case EM_386:
      return "Intel 80386";
    case EM_68K:
      return "Motorola 68000";
    case EM_88K:
      return "Motorola 88000";
    case EM_860:
      return "Intel 80860";
    case EM_MIPS:
      return "MIPS RS3000";
    case EM_960:
      return "Intel 80960";
    case EM_X86_64:
      return "AMD x86-64 architecture";
    case EM_NONE:
      return "None";
  }
  printf("unknown: %x\n", machine);

  return "Unknown";
}

/**
 * Retruns the endianness encoding used in the file.
 * 
 * @return encoding (little endian or big endian)
 *        0 if ERROR.
 *        1 if little endian.
 *        2 if big endian.
 * 
 * @note:
 * ELFDATANONE is 0, Invalid data encoding
 * ELFDATA2LSB is 1, specifies 2's complement values, with the least significant byte occupying the lowest address.
 * ELFDATA2MSB is 2, specifies 2's complement values, with the most  significant byte occupying the lowest address.
 * 
 * ELFDATA2LSB:
 * [08	07	06	05	04	03	02	01] => 0x0102030405060708
 * 
 * ELFDATA2MSB:
 * [01	02	03	04	05	06	07	08] => 0x0102030405060708
 * 
 * 
 * @note: FOR NOW I DON'T SUPPORT OTHER ENDIANESS (ONLY FOR COMPILER COMPUTER ENDIANNESS)
*/
int getEndiannessEncoding(void){
  //can read everything with the endianess thing.

  char encodingByte = elf_Ehdr.e_ident[EI_DATA];
  
  if(encodingByte == ELFDATANONE){
    err("Invalid data encoding for the file.");
    return 0;
  }
  //ELFDATANONE is 0, Invalid data encoding
  //ELFDATA2LSB is 1, byte address zero on the left (specifies 2's complement values, with the least significant byte occupying the lowest address.)
  //ELFDATA2MSB is 2, byte address zero on the left (specifies 2's complement values, with the most  significant byte occupying the lowest address.)


  return encodingByte;
}


/**
 * Returns entry point of the program.
 * 
 * @return entry point of executable. (Elf64_Addr, uint64_t)
 * @note 0 is None.
*/
uint64_t getEntryPoint(void){
  return elf_Ehdr.e_entry;
}



/**
 * Returns number of program headers in the elf.
 * 
 * @return the number of program headers. UINT64_MAX if error.
*/
static uint64_t getSectionsCount(void){
  Elf64_Xword numOfSections =  elf_Ehdr.e_shnum; //Elf64_Xword

  //If the number of sections is greater than or equal to SHN_LORESERVE (0xff00), e_shnum has the value SHN_UNDEF (0) and the actual number of section header table entries is contained in the sh_size field of the section header at index 0. Otherwise, the sh_size member of the initial entry contains 0.
  
  if (numOfSections == SHN_UNDEF){
    //the first section has the size.
    //(did not check, cant find an elf to check that with and don't have the time)
    Elf64_Shdr section;
    if(getSectionHdrByOffset(&section, elf_Ehdr.e_shoff)){
      //or maybe not except? idk. not important for my project, maybe I will add later.
      err("Error in getSectionHdrByOffset at getSectionsCount, getting the first(0th) section. section header offset: 0x%" PRIx64 ", section header size: 0x%" PRIx16 "\n", elf_Ehdr.e_shoff, elf_Ehdr.e_shentsize);
      return UINT64_MAX;
    }
    numOfSections = section.sh_size;
  }

  return numOfSections;
  
}



/**
 * Shows all the sections and thier premissions.
 * 
 * @note making this more for me to understand how it works. 
 * 
 * Imporatnt for this, I set _FILE_OFFSET_BITS=64 before so fseek would be fseek64(and fopen, fopen64..)
 * 
 * $ readelf -S prog
*/
void showSectionsHeaders(void){
  //start scanning from elf_Ehdr.e_shoff
  

  //inside the elf header:
  //  e_shoff member gives the byte offset from the beginning of the file to the section header table;
  //  e_shnum tells how many entries the section header table contains. (NOT ALWAYS, LOOK AT NOTES)
  //  e_shentsize gives the size in bytes of each entry.


  // to show the name of the section:
  //  e_shstrndx 


  //Although index 0 is reserved as the undefined value, the section header table contains an entry for index 0. That is, if the e_shnum member of the ELF header says a file has 6 entries in the section header table, they have the indexes 0 through 5. The contents of the initial entry are specified later in this section.
  
  Elf64_Off curSectionFileOffset = elf_Ehdr.e_shoff;
  
  if(curSectionFileOffset == 0){
    printf("No Sections.\n");
    return;
  }

  uint64_t numOfSections;
  if ((numOfSections = getSectionsCount()) == UINT64_MAX){
    err("Error in getSectionsCount at showSectionsHeaders.");
    return;
  }

  if(numOfSections == 0){
    printf("No Sections.\n");
    return;
  }

  for(Elf64_Xword sectionCnt=0; sectionCnt < numOfSections; sectionCnt++){
    showSectionHdr(curSectionFileOffset);

    //can maybe check here for integer overflow
    curSectionFileOffset += elf_Ehdr.e_shentsize;
  }
}

/**
 * Returns number of program headers in the elf.
 * 
 * @return the number of program headers. UINT64_MAX if error.
*/
static uint64_t getProgramHeadersCount(void){
  uint64_t numOfProgramHdrs = elf_Ehdr.e_phnum;

  /* Special value for e_phnum.  This indicates that the real number of
   program headers is too large to fit into e_phnum.  Instead the real
   value is in the field sh_info of section 0.  */
  //#define PN_XNUM		0xffff
  if (numOfProgramHdrs == PN_XNUM){
    //this was NEVER checked!
    //the first (section!) has the size.

    Elf64_Shdr section;
    if(getSectionHdrByOffset(&section, elf_Ehdr.e_shoff)){
      err("Error in getSectionHdrByOffset at getProgramHeadersCount, getting the first(0th) section. section header offset: 0x%" PRIx64 ", section header size: 0x%" PRIx16 "\n", elf_Ehdr.e_shoff, elf_Ehdr.e_shentsize);
      return UINT64_MAX;
    }
    numOfProgramHdrs = section.sh_info;
  }
  return numOfProgramHdrs;
}

/**
 * Shows all the program section headers and thier premissions.
 * 
 * @note making this more for me to understand how it works. 
 * 
 * $ readelf -l prog
*/
void showProgramHeaders(void){ //TODO: do this
  Elf64_Off curProgramHdrFileOffset = elf_Ehdr.e_phoff;
  
  if(curProgramHdrFileOffset == 0){
    printf("No program headers. (e_phoff is 0)\n");
    return;
  }

  uint64_t numOfProgramHdrs;
  if((numOfProgramHdrs = getProgramHeadersCount()) == UINT64_MAX){
    err("Error in getProgramHeadersCount at showProgramHeaders.");
    return;
  }

  if (numOfProgramHdrs == 0){
    printf("No program headers. (e_phnum is 0)");
    return;
  }

  for(Elf64_Xword programHdrCnt = 0; programHdrCnt < numOfProgramHdrs; programHdrCnt++){
    showProgramHdr(curProgramHdrFileOffset);

    //can maybe check here for integer overflow
    curProgramHdrFileOffset += elf_Ehdr.e_phentsize;
  }
}

/**
 * Prints the program header at programHdrOff of size programHdrSize info into stdout.
 * 
 * @param programHdrOff,  program header offset in the file.
 * 
 * @note if error writes there was an error, does not return anything special.
*/
void showProgramHdr(Elf64_Off programHdrOff){
  Elf64_Phdr programHdr;
  if(getProgramHdrByOffset(&programHdr, programHdrOff)){
    err("Error in showProgramHdr in getProgramHdrByOffset. program header offset: 0x%" PRIx64 ", program header size: 0x%" PRIx16 "\n", programHdrOff, elf_Ehdr.e_phentsize);
    return;
  }

  //printf("type:" PRIx32 "\n", programHdr.p_type);
  printf("type: %s\n", getProgramHdrType(&programHdr));

  //printf("flags: " PRIx32 "\n", programHdr.p_flags);
  showProgramHdrFlags(&programHdr);

  printf("offset: 0x%" PRIx64 "\n" , programHdr.p_offset);

  printf("vaddr: 0x%" PRIx64 "\n", programHdr.p_vaddr);
  printf("paddr: 0x%" PRIx64 "\n", programHdr.p_paddr);

  printf("filesz: 0x%" PRIx64 "\n", programHdr.p_filesz);
  printf("memsize: 0x%" PRIx64 "\n", programHdr.p_memsz);
  printf("align: 0x%" PRIx64 "\n", programHdr.p_align);

  printf("\n");
}




/**
 * Frees the Mini_ELF_Phdr_node* and everything if has (recursive, without recursion).
*/
void freeAll_Mini_Phdr_nodes(Mini_ELF_Phdr_node *head){
  
  /*
  //In recursion:
  if(head == NULL)
    return;  
  freeAll_Mini_Phdr_nodes(head->next);
  free(head);
  */

  Mini_ELF_Phdr_node *tmp;

  while(head != NULL){
    tmp = head;
    head = head->next;
    free(tmp);
  }

}

/**
 * Reads from file from from size bytes into buffer.
 * 
 * @param from, location to start reading in file.
 * @param size, amount to read from the file.
 * @param buffer, buffer to read into.
 * 
 * @return returns 0 on success, else something else.(-1 for example)
 * 
 * @note The caller has to make sure that buffer is atleast size size.  
*/
int readFileData(uint64_t from, uint64_t size, char *buffer){
  //TODO: Use this to make the code cleaner.
  //64 bit because I set _FILE_OFFSET_BITS=64 before
  //TODO: look into this later.
  //maybe do something else, https://stackoverflow.com/questions/4357570/use-file-offset64-vs-file-offset-bits-64
  //this is so fseek with be fseek64 and fread will be fread64.

  if (fseek(file, from, SEEK_SET) != 0){
    err("Error in fseek at readFileData. from 0x%" PRIx64, from);
    return 1;
  }
  if (fread(buffer, sizeof(char), size, file) != size){
    err("Couldn't fread buffer, in readFileData at fread. reading to %p, from 0x%" PRIx64 ", size 0x%" PRIx64, buffer, from, size);
    return 2;
  }

  return 0;
}


bool is64Bit(void){
  return is64BitElf;
}

/**
 * Retruns an Mini_ELF_Phdr_node (linked list) pointer of the executable program headers.
 * 
 * @return an allocated linked list for Mini_ELF_Phdr_node* of mini_ELF_Phdr.
 *         if None, returns NULL
 * 
 * @note CALL freeAll_Mini_Phdr_nodes after done using the structure.
*/
Mini_ELF_Phdr_node *getAllExec_Mini_Phdr(void){
  //TODO: MUST!!! SWITCH INTO MORE FUNCTIONS! (To make more clean and understandable)

  //Initilizing curProgramHdrFileOffset with number location of first program header. 
  Elf64_Off curProgramHdrFileOffset = elf_Ehdr.e_phoff;
  if(curProgramHdrFileOffset == 0){
    // No program headers. (e_phoff is 0)
    return NULL;
  }

  //getting numer of program headers.
  uint64_t numOfProgramHdrs;
  if((numOfProgramHdrs = getProgramHeadersCount()) == UINT64_MAX){
    err("Error in getProgramHeadersCount at getAllExec_Mini_Phdr.");
    return NULL;
  }
  if (numOfProgramHdrs == 0){
    // No program headers. (e_phnum is 0)
    return NULL;
  }

  Mini_ELF_Phdr_node *head = (Mini_ELF_Phdr_node *) malloc(sizeof(Mini_ELF_Phdr_node));
  if(head == NULL){
    // Malloc failed
    err("Error, Malloc for head failed inside getAllExec_Mini_Phdr.\n");
    return NULL;
  }
  head->cur_mini_phdr.vaddr = 0;
  head->cur_mini_phdr.file_offset = 0;
  head->cur_mini_phdr.size = 0;
  head->next = NULL;

  Mini_ELF_Phdr_node *cur = head;

  for(Elf64_Xword programHdrCnt = 0; programHdrCnt < numOfProgramHdrs; programHdrCnt++, curProgramHdrFileOffset+=elf_Ehdr.e_phentsize){
    //showProgramHdr(curProgramHdrFileOffset);

    //TODO put this into a function:
    Elf64_Off programHdrOff = curProgramHdrFileOffset;
    
    Elf64_Phdr programHdr;
    if(getProgramHdrByOffset(&programHdr, programHdrOff)){
      err("Error at getProgramHdrByOffset in ???. program header offset: 0x%" PRIx64 ", program header size: 0x%" PRIx16 "\n", programHdrOff, elf_Ehdr.e_phentsize);
      printf("Continuing\n");
      continue;
    }

    //checking if segement is executable
    if (!(programHdr.p_flags & PF_X))
      continue;
    
    printf("OK..\n");

    //checking if segement is LOAD (it has to be if exec but still, lets check).
    if ( programHdr.p_type != PT_LOAD )
      continue;


    printf("GOOD\n");
    
    if(cur != head){
      printf("IN HERE\n");
      cur->next = (Mini_ELF_Phdr_node *) malloc(sizeof(Mini_ELF_Phdr_node)); 
      if(cur->next == NULL){
        // Malloc failed
        err("Error, Malloc for cur failed inside getAllExec_Mini_Phdr.\n");

        //returning what got until now.
        return head;
      }
      cur = cur->next;
    }

    //program header is executable.
    cur->cur_mini_phdr.vaddr = programHdr.p_vaddr;
    cur->cur_mini_phdr.file_offset = programHdr.p_offset;
    cur->cur_mini_phdr.size = programHdr.p_filesz; //same as p_memsz in this case (as far as I know)
    cur->next = NULL;

    //can maybe check here for integer overflow
    curProgramHdrFileOffset += elf_Ehdr.e_phentsize;
  }

  if(head->cur_mini_phdr.size == 0){
    //not even one executable segment was found
    free(head);
    return NULL;
  }
  
  return head;

  /*
  I want to somehow do that:
    get X bytes from entry point.
  
  I can maybe get section section and do for each one of them in a different function.
  problems: what if sections collide.
  solution for problem: check for section collision (one start is at ones end).
  and consider them as one section.

  

  so what to do:
  1) Make a function to get all the executable section headers and put them into a structure.
    (save the [header's name], [virtual addr], [file offset], [size])
    (ingoring sh_addralign, assuming the virtual addr is already aligned)
    

  2) Make a function to sort the headers by the virtual addr
  3) MAYBE, IDK: Make a function to merge the headers that are near eachother.
  
  4) write a function that does:
    general: go through each section and get the contents and so on..

    starting from the first section and going until the last section, call my call.
    (not in this func)

    when reaching the end of a section:
      lets say section A
      we see its distance from the next section, section B
      if the distance is more than max operand size (15 - ZYDIS_MAX_INSTRUCTION_LENGTH):
        then give section A plus some of section B. (pad with 0s in the middle)
        else give section A plus some 0s.
    
    when in the start of a section:
      if the distance from the prev section is more than max operand size (15 - ZYDIS_MAX_INSTRUCTION_LENGTH):
        then give some of section B plus what is needed from section A. (pad with 0s in the middle)
        else give some 0s and add what is needed from section A.
    
    

  or idea 2:
  kinda virtual address.
  I will create a function where you need to tell what location you want.
  (I will get the start location from the first section)




  BTWWWWWW:
    can maybe:
    Implement 2 ways. one in which I am doing this throughout the sections (faster)
    and one which I am doing throught the.

    just figured out a problem with going through the sections, I got to go through the segments.
    there might not be sections for some executable code :(
    which is sad but nah it will be okay, if I have time I will implement a way to do them both on choice.

    now before I will merge those who need merging (although I doubt there will be any that need merge)
    I will start from just getting it and not merging cause I don't have time.



    just future note: remeber to look at PT_INTERP. (in general, not related to rop)
  */
}


/**
 * Cleans all the stuff created in here that needs to be cleaned (freed)
 * 
 * @return 0 if sucess, else something else.
*/
int cleanElfUtils(void){
  
  if(file){
    if (fclose(file) != 0){
      err("Error in cleanElfUtils, while tring to fclose file.\n");
      return 1;
    }
    file = NULL;
  }

  if(stringTable.data){
    free(stringTable.data);
    stringTable.data = NULL;
  }

  //there is no need to clean stuff, I won't use them.
  return 0;
}