#pragma once

//call this before you do anything and you have to get 0.
int initElfUtils(char const *filename, unsigned long long entryP);

//call this when you finish using this class.
int cleanElfUtils(void);

const char* getArch(void);

unsigned long long getEntryPoint(void);
void showScanSections(void);
void showProgramHeaders(void);

int getEndiannessEncoding(void);
//could have used libelf but I want to create it myself.

