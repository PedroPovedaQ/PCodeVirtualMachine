/*
// Name : Pedro Poveda
// PROG: pmachine.c
// Description: Virtual Machine for running P-Code
*/
#include <stdio.h>
#include <stdlib.h>
#define MAX_STACK_HEIGHT = 2000;
#define MAX_CODE_LENGTH = 500;
#define MAX_LEXI_LEVELs = 3;
#define STACK_MAX 100

typedef struct{
	int data[STACK_MAX];
	int size;
} stack;


//stuff I have to use program wide
typedef struct{
	int op; //op code
	int l; // L
	int m; // M
} instruction;

char *instructionCodes[]= {
	"LIT", "OPR","LOD","STO","CAL",
	"INC","JMP","JPC", "SIO" 
};

void printInstructions(instruction *list , int length, FILE *outputFile);
void virtualize (instruction *list, stack *slist,int length, FILE  *outputFile);

void stackPush(stack *s, int d);
int stackPop(stack *s);
void stackEdit(stack *s, int value, int offset);
int stackGet(stack *s, int offset);
void stackMath(stack *s, int op);
void printStack(stack *s, FILE *out);

instruction *instructionList = NULL; 
stack *stackList = NULL;


int main(){
	stack *stackList = malloc(3*sizeof(stack)); //for each lexigraphical level

	FILE *inputFilePTR, *outputFilePTR;
	inputFilePTR = fopen("mcode.txt", "r");
	outputFilePTR = fopen("stacktrace.txt", "w");
	int counter =0;
	char line[80];

	while (fgets(line, 80, inputFilePTR)) {
		//intialize a pointer and keep mallocing and adding stuff to it.
		instructionList = (instruction*) realloc(instructionList, (counter + 1)* sizeof(instruction));

		
		sscanf(line, "%d %d %d\n", &instructionList[counter].op, &instructionList[counter].l, &instructionList[counter].m);
		counter++;
	}
	int length = counter++; //not sure if this is messy

	//print instructions here
	printInstructions(instructionList, length, outputFilePTR);
	virtualize(instructionList, stackList, length, outputFilePTR);

	//Maintenance section
	free(instructionList);
	free(stackList);
	fclose(inputFilePTR);
	fclose(outputFilePTR);
	return 0;
}

void printInstructions(instruction *list , int length, FILE *outputFile)
{
	fprintf(outputFile,"%.4s\t%.3s\t%.1s\t%.2s\n", "Line","OP", "L", "M" );
	int i;

	for(i=0; i < length; i++){
		fprintf(outputFile,"%2d\t\t%s\t%d\t%d\n", i, instructionCodes[list[i].op-1], list[i].l, list[i].m);
	}
	fprintf(outputFile,"\n\n");
}

void virtualize (instruction *list, stack *slist,int length, FILE  *outputFile)
{
	// instantiate default values
	int sp, bp, pc;
	sp = 0;
	bp = 1;
	pc = 0;
	instruction *ir = NULL;
	int breakFlag=0;
	int incrementLevelFlag=0;
	int currentLevel=0;
	//int levelSeperator = 0;
	//print out initial values
	fprintf(outputFile,"\t\t\t\t%.2s\t%.2s\t%.2s\t%s\n", "pc", "bp","sp","stack");
	fprintf(outputFile,"Initial Values\t\t\t%2d\t%2d\t%2d\t\n", pc, bp, sp);
	//loop through all instructions and run them
	int temp; //temp variable for all operations
	while(1){
		ir= &list[pc];
		//print out the first part of the instruction
		fprintf(outputFile,"%2d\t%.3s\t%2d\t%2d\t", pc, instructionCodes[ir->op -1],ir->l, ir->m);


		//perform instruction, huge switch statement
		switch(ir->op)
		{
			case 1: //Push value M onto stack *DONE
			sp+=1;
			stackPush(&slist[currentLevel],ir->m);
			pc++;
			break;

			case 2: //Arithmetic Operation
			switch(ir->m){
				case 0: // RET Return from function or procedure DONE
				sp = bp-1;
				pc = stackGet(&slist[currentLevel], 3);
				bp = stackGet(&slist[currentLevel], 2);
				currentLevel--;
				break;

				case 1:
				temp = stackPop(&slist[currentLevel]);
				stackPush(&slist[currentLevel], -1*temp);
				pc++; //DONE
				break;

				case 7:
				temp = stackPop(&slist[currentLevel]);
				stackPush(&slist[currentLevel], temp%2);
				//DONE
				pc++;
				break;

				default: //for Math functions that require popping DONE
				stackMath(&slist[currentLevel],ir->m);
				sp--;
				break;
			}
			break;

			case 3: //Get value in frame L levels down at offset M and push it *DONE
			sp++;
			temp =stackGet(&slist[currentLevel -ir->l], ir->m);
			stackPush(&slist[currentLevel], temp );
			break;

			case 4:; //Pop stack and insert value in frame L levels down at offset M *DONE
			temp= stackPop(&slist[currentLevel]);
			stackEdit(&slist[currentLevel -ir->l], temp, ir->m);
			pc++;
			sp--;
			break;

			case 5: //Call procedure at M (generates new stack frame) *DONE
			//instantiate values in the new stack
			stackPush(&slist[currentLevel + 1], 0);
			stackPush(&slist[currentLevel + 1], currentLevel +1);
			stackPush(&slist[currentLevel +1], bp);
			stackPush(&slist[currentLevel +1], ++pc);
			bp= sp + 1;
			pc = ir->m;
			incrementLevelFlag = 1;
			break;

			case 6:;//Allocate M locals on stack *DONE
			int k;
			temp = slist[currentLevel].size;
			for(k = 0; k < (ir->m - temp); k++){ //this way we don't over-push
				stackPush(&slist[currentLevel],0);
		}
		sp += ir->m;
		pc++;
		break;

			case 7: //Jump to M *DONE
			pc = ir->m;
			break;

			case 8: //Pop stack and jump to M if value is equal to 0 *DONE
			if(stackPop(&slist[ir->l]) == 0)
				pc= ir->m;
			else{
				pc++;
			}
			sp--;
			break;

			case 9:
			switch(ir->m){
				case 0: //Pop stack and print out value *DONE
				stackPop(&slist[currentLevel]);
				sp--;
				break;

				case 1:; //Read in input from user and push it *DONE
				int tempInt;
				scanf("%d", &tempInt);
				stackPush(&slist[currentLevel], tempInt);
				break;

				case 2: //Halt the machine *DONE
				breakFlag =1;
				break;
			}
			break;
		}
		//print out contents of stack and stuff to disk.
		fprintf(outputFile,"%2d\t%2d\t%2d\t", pc,bp,sp);
		//print stacks
		int c;
		for(c =0; c <= currentLevel; c++)
		{
			printStack(&slist[c], outputFile);
			if(c!= currentLevel)
				fprintf(outputFile,"| ");
		}

		fprintf(outputFile,"\n");
		if(breakFlag)
			break;
		else if(incrementLevelFlag){
			currentLevel++;
			incrementLevelFlag=0;
		}
	}

	
	return;
	
}



void stackPush(stack *s, int d)
{
	if (s->size < STACK_MAX)
		s->data[s->size++] = d;
}

int stackPop(stack *s)
{
	if (s->size == 0){
		return -1;
	}
	else{
		s->size--;
		return s->data[s->size];
	}
}

void stackEdit(stack *s, int value, int offset)
{	
	s->data[offset] = value;
	return;
}

int stackGet(stack *s, int offset)
{
	return s->data[offset];
}

void stackMath(stack *s, int op)
{
	int temp1; //local temp1 variable
	int temp2; //local temp2 variable
	switch(op){
		case 2: //ADD
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2+temp1);
		break;

		case 3: //SUBTRACT
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2-temp1);
		break;

		case 4: //Multiply
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2*temp1);
		break;

		case 5: //Divide
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2/temp1);
		break;

		case 7: //MOD
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2%temp1);
		break;

		case 8: //EQL
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2==temp1);
		break;

		case 9: //NEQ
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2!=temp1);
		break;

		case 10: //LSS
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2 < temp1);
		break;

		case 11: //LEQ

		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2 <= temp1);
		break;

		case 12: //GTR
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2 > temp1);
		break;

		case 13: //GEQ
		temp1 = stackPop(s);
		temp2 = stackPop(s);
		stackPush(s, temp2 >= temp1);
		break;
	}

	return;
}

void printStack(stack *s, FILE *out)
{
	if (s->size != 0) {
		int i;

		for(i = 0; i < s->size; i++){
			fprintf(out,"%d ", s->data[i]);
		}
	}
	return;
}