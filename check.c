#include <stdio.h>
#include <string.h>
typedef struct{
	char A[11];
} C;

int main(){
	C a = {.A="0123456789\x00"};
	C b;

	printf("a: %s\n", a.A);
	printf("b: %s\n", b.A);
	
	printf("\nnow preforming memcpy:\n");
	//b.A = a.A;
	memcpy(&b, &a, sizeof(b));
	
	printf("a: %s\n", a.A);
	printf("b: %s\n", b.A);


}
