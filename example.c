//gcc ./example.c -no-pie -fno-stack-protector -w -o example

#include <stdio.h>
#include <stdlib.h>


void (*func[])(char *) = {1, system, 0};

//RDI, RSI, RDX, RCX, R8, R9 (System V AMD64 ABI, x86-64) calling convension 

//because this will use edi 
void win(int doWin) {
    func[doWin]("echo WOHO you got 100 on the test!");
    exit(0);
}

int getGrade(void){
    char name[10];
    printf("enter your name:\n");
    gets(name);
    printf("you entered: %s\n", name);
    printf("returning your grade.\n");
    return 50;
}

int main(){
    int myGrade = getGrade();
    printf("You got %d\n", myGrade);
    
    return 0;
}

void non(){
    __asm__("pop %rdi;ret;");
}

