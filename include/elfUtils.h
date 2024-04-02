#pragma once

//call this before you do anything and you have to get 0.
int initElfUtils(char const *filename, unsigned long long entryP);

//call this when you finish using this class.
void cleanElfUtils(void);

const char* getArch(void);
unsigned long long getEntryPoint(void);