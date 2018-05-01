#include <stdio.h>
#include <malloc.h>
#include <string.h>

 struct student{
	char ini[4];
	int number;
			   };
void swap(struct student *x,struct student *y);

int main(void){
	 struct student *A=(struct student *)malloc(sizeof(struct student));
	 strcpy(A->ini,"IHS");
	 A->number=02;
	  
	struct student *B=(struct student *)malloc(sizeof(struct student));
	strcpy(B->ini,"JUS");
	B->number=201302423;

	printf("%10d %5s  ",A->number,A->ini);
	printf("%10d %5s\n",B->number,B->ini);
	swap(A,B);

	printf("%10d %5s  ",A->number,A->ini);
	printf("%10d %5s\n",B->number,B->ini);
	free(A);
	free(B);
	return 0;}

 void swap(struct student *a,struct student *b){
			int temp = a->number;
			a->number = b->number;
			b->number = temp;
			char t[4];
			strcpy(t,a->ini);
			strcpy(a->ini,b->ini);			
			strcpy(b->ini,t);
			}
