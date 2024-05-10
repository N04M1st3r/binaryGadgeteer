#pragma once
//could have used libelf and such but I want to create it myself.
#include <stdint.h>
#include <stdbool.h>


//mini program header, with only the stuff I need.
typedef struct mini_ELF_Phdr{
  uint64_t vaddr;        //virtual addr in the executable.            (Elf64_Addr)
  uint64_t file_offset;  //location in the file where the amount is.  (Elf64_Off)
  uint64_t size;         //size.                                      (Elf64_Xword)
} mini_ELF_Phdr;

typedef struct Mini_ELF_Phdr_node{
  mini_ELF_Phdr cur_mini_phdr;
  struct Mini_ELF_Phdr_node *next;
} Mini_ELF_Phdr_node;

//call this before you do anything and you have to get 0.
int initElfUtils(char const *filename, uint64_t entryP);

//call this when you finish using this class.
int cleanElfUtils(void);

const char *getArch(void);
bool is64Bit(void);

uint64_t getEntryPoint(void); //(Elf64_Addr, uint64_t)
void showSectionsHeaders(void);
void showProgramHeaders(void);

int getEndiannessEncoding(void);

Mini_ELF_Phdr_node *getAllExec_Mini_Phdr(void);
void freeAll_Mini_Phdr_nodes(Mini_ELF_Phdr_node *node);

int readFileData(uint64_t from, uint64_t size, char *buffer);