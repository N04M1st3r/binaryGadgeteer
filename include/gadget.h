#pragma once
#include "gadgetLinkedList.h"
#include "endGadgetFind.h"
#include <stdbool.h>

int initDecoderAndFormatter(ArchInfo *arch_p);
GadgetLL *expandGadgetsDown(char *buffer, uint64_t buf_vaddr, uint64_t buf_fileOffset, size_t bufferSize, GadgetLL *gadgetsLL, uint8_t depth);


void GadgetLLShowAll(GadgetLL *gadgetsLL);

void GadgetLLShowBasedCondition(GadgetLL *gadgetsLL, bool (*checkCondition)(GadgetNode *));

void GadgetLLShowOnlyEnds(GadgetLL *gadgetsLL);

void GadgetLLShowOnlyPrintableAddress(GadgetLL *gadgetsLL);
void GadgetLLShowOnlyPrintableAddressEnds(GadgetLL *gadgetsLL);

void GadgetLLShowOnlyPrintableOrNullAddress(GadgetLL *gadgetsLL);


typedef struct SearchInfo{
    char *searchAsm;
    uint64_t vaddr; //(UINT64_MAX is NON)
} SearchInfo;


void printRegsStatistics(GadgetLL *gadgetsLL, bool is64bit);

void getSearchInfoArr(GadgetLL *gadgetsLL, SearchInfo *searches, size_t searchesLen);

void GadgetLLSearchIntoArr(GadgetLL *gadgetsLL, SearchInfo *searches, size_t searchesLen);
