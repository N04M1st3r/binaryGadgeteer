//this will force it to be 64 bits
#define _FILE_OFFSET_BITS 64

#include <elf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "costumErrors.h"
#include "elfUtils.h"


/**
 * This will give info and things to do with the elf, 
 * it works on one elf at a time. (because that is what I need)
 * source: http://www.skyfree.org/linux/references/ELF_Format.pdf
 * 
 * for a another implemenation you may look at elfcommon.c in binutils
*/

#if 0

typedef __u16	Elf32_Half; // same as 64
typedef __u32	Elf32_Word; // same as 64

typedef __u32	Elf32_Off;
typedef __u32	Elf32_Addr;


typedef __u16	Elf64_Half; // same as 32
typedef __u32	Elf64_Word; // same as 32

typedef __u64	Elf64_Off;
typedef __u64	Elf64_Addr;


#define EI_NIDENT	16

typedef struct elf32_hdr {
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;  /* Entry point */
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64_hdr {
  unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;		/* Entry point virtual address */
  Elf64_Off e_phoff;		/* Program header table file offset */
  Elf64_Off e_shoff;		/* Section header table file offset */
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;









/* Section header.  */
typedef struct
{
  Elf32_Word	sh_name;		/* Section name (string tbl index) */
  Elf32_Word	sh_type;		/* Section type */
  Elf32_Word	sh_flags;		/* Section flags */
  Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf32_Off	sh_offset;		/* Section file offset */
  Elf32_Word	sh_size;		/* Section size in bytes */
  Elf32_Word	sh_link;		/* Link to another section */
  Elf32_Word	sh_info;		/* Additional section information */
  Elf32_Word	sh_addralign;		/* Section alignment */
  Elf32_Word	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;
typedef struct
{
  Elf64_Word	sh_name;		/* Section name (string tbl index) */
  Elf64_Word	sh_type;		/* Section type */
  Elf64_Xword	sh_flags;		/* Section flags */
  Elf64_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf64_Off	sh_offset;		/* Section file offset */
  Elf64_Xword	sh_size;		/* Section size in bytes */
  Elf64_Word	sh_link;		/* Link to another section */
  Elf64_Word	sh_info;		/* Additional section information */
  Elf64_Xword	sh_addralign;		/* Section alignment */
  Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;

#endif


//to know if to use elf64_hdr or elf32_hdr and such..
bool is64BitElf;

/*static union{
    Elf64_Ehdr elf64;
    Elf32_Ehdr elf32;
} elf_Ehdr;*/

//this saves both 32 and 64 elf headers.
static Elf64_Ehdr elf_Ehdr;

FILE *file = NULL;


/**
 * Copies all values in elf32 to elf64.
*/
static void copyAllElf32ToElf64(Elf32_Ehdr *elf32, Elf64_Addr *elf64){
  elf64->e_ident = elf32->e_ident;
  
}


/**
 * Initilizes elf_Ehdr. (32 or 64 depended on is64BitElf)
 * 
 * @return sucess, 0 if no errors, else 1 or such if there were errors.
*/
static int setupElf_ehdr(void){
  if (fseek(file, 0, SEEK_SET) != 0){
    perror("error.");
    err("Error in fseek at setupElf_ehdr");
    return 1;
  }

  size_t amount_to_read;
  if(!is64BitElf){
    Elf32_Ehdr elf32_Ehdr_tmp;
    fread(&elf32_Ehdr_tmp, 1, sizeof(Elf32_Ehdr), file);
    
  }
  else
    amount_to_read = sizeof(Elf32_Ehdr);

  if (fread(&elf_Ehdr, 1, amount_to_read, file) != amount_to_read){
    err("Couldnt read elf header, the file spesification. in setupElf_ehdr");
    return 2;
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
    perror("error.");
    err("Error in fseek at setupBits");
    return 1;
  }
  unsigned char e_ident[EI_NIDENT];
  if (fread(e_ident, sizeof(char), EI_NIDENT, file) != EI_NIDENT){
    err("Couldnt read indent, the file spesification. in setupBits");
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
 * Initilizes the elf header into this Utils, this can work on one file at a time.
 * 
 * Initilizes everything. 
 * 
 * @param filename      Filename of the elf.
 * @param entryP        Entry point, ULLONG_MAX(0xffff_ffff_ffff_ffff) if None (detect here).
 * @return sucess, 0 if no errors, another if there were errors.
*/
int initElfUtils(char const *filename, unsigned long long entryP){
  if ((file = fopen(filename, "r")) == NULL){
    perror("Error opening the file.");
    return 1;
  }

  //checks if the ELF is correct and sets if its 64 bit.
  if(setupBits() != 0){
    err("error in initElfUtils at setupBits.");
    return 2;
  }


  if(setupElf_ehdr() != 0){
    err("error in initElfUtils at setupElf_ehdr");
    return 3;
  }

  //maybe I should do something with the EI_DATA?

  if (entryP != ULLONG_MAX){
    if (is64BitElf)
      elf_Ehdr.elf64.e_entry = entryP;
    else
      elf_Ehdr.elf32.e_entry = entryP;
  }
  return 0;
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
const char* getArch(void){
  Elf32_Half machine;
  if(is64BitElf)
    machine = elf_Ehdr.elf64.e_machine;
  else
    machine = elf_Ehdr.elf32.e_machine;
  
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
  printf("aaa: %x\n", machine);

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
int getEndiannessEncoding(){
  //can read everything with the endianess thing.

  char encodingByte;
  if (is64BitElf)
    encodingByte = elf_Ehdr.elf64.e_ident[EI_DATA];
  else
    encodingByte = elf_Ehdr.elf32.e_ident[EI_DATA];
  
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
 * @return entry point of executable.
 * @note 0 is None.
*/
unsigned long long getEntryPoint(void){
  if(is64BitElf)
    return elf_Ehdr.elf64.e_entry;
  
  //32 bit:
  return elf_Ehdr.elf32.e_entry;
}

/**
 *
 * 
 * @note making this more for me to understand how it works. 
*/
void showScanSections(void){
  //start scanning from elf.?.e_shoff


  //inside the elf header:
  //  e_shoff member gives the byte offset from the beginning of the file to the section header table;
  //  e_shnum tells how many entries the section header table contains
  //  e_shentsize gives the size in bytes of each entry.

  //NOTE: If the number of sections is greater than or equal to SHN_LORESERVE (0xff00), e_shnum has the value SHN_UNDEF (0) and the actual number of section header table entries is contained in the sh_size field of the section header at index 0. Otherwise, the sh_size member of the initial entry contains 0.

  //WTF: Some section header table indexes are reserved in contexts where index size is restricted. For example, the st_shndx member of a symbol table entry and the e_shnum and e_shstrndx members of the ELF header. In such contexts, the reserved values do not represent actual sections in the object file. Also in such contexts, an escape value indicates that the actual section index is to be found elsewhere, in a larger field.

  //Although index 0 is reserved as the undefined value, the section header table contains an entry for index 0. That is, if the e_shnum member of the ELF header says a file has 6 entries in the section header table, they have the indexes 0 through 5. The contents of the initial entry are specified later in this section.
  
  //so always the initial(0th) entry will not be a section (I think)
  
  //I will start by writing the 32 bit version and then I will divide it.

  //I can make this a long long
  Elf32_Off location = elf_Ehdr.elf32.e_shoff;

  

  //Elf64_Off is uint64_t, but fseek is getting long	
  //if (fseek(file, location, SEEK_SET))
  
  /*
    if (fseek(file, 0, SEEK_SET) != 0){
    perror("error.");
    err("Error in fseek at setupBits");
    return 1;
  }
  unsigned char e_ident[EI_NIDENT];
  if (fread(e_ident, sizeof(char), EI_NIDENT, file) != EI_NIDENT){
    err("Couldnt read indent, the file spesification. in setupBits");
    return 2;
  }
  */
 //TODO: CHANGE TO open64, lseek64


}


/**
 * Cleans all the stuff created in here that needs to be cleaned (freed)
*/
void cleanElfUtils(void){
  
  if(file){
    fclose(file);
    file = NULL;
  }

}