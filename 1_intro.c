#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct ArqResources{
	uint32_t* Mem;
	uint32_t Reg[32];
}ArqResources;

typedef struct typeU{
	uint8_t opcode;
	uint8_t z,x,y;
	uint16_t l;
}typeU;

typedef struct typeF{
	uint8_t opcode;
	uint8_t z,x;
	uint16_t i;
}typeF;

typedef struct typeS{
	uint8_t opcode;
	uint32_t i;
}typeS;

ArqResources* inicialization(){
	ArqResources* newArq = (ArqResources*)malloc(sizeof(ArqResources));
	newArq->Mem = (uint32_t*)calloc(8,1024);
	return newArq;
}

char opcodeType(uint8_t op){
	if(op <= 0b1001 && op >= 0b0000)
		return 'U';
	else if((op <= 0b011101 && op >= 0b011000) || (op <= 0b010111 && op >= 0b010010))
		return 'F';
	else if(op >= 0b101010 && op <= 0b111111)
		return 'S';
	else
		return 'E';
}


typeU* newInstU(uint8_t op,uint8_t x, uint8_t y, uint8_t z, uint16_t l){
	typeU* newT = (typeU*)malloc(sizeof(typeU));
	newT->opcode = op;
	newT->x = x;
	newT->y = y;
	newT->z = z;
	newT->l = l;
	return newT;
}
typeF* newInstF(uint8_t op,uint8_t x,uint8_t z,uint16_t i){
	typeF* newT = (typeF*)malloc(sizeof(typeF));
	newT->x = x;
	newT->opcode = op;
	newT->i = i;
	return newT;	
}
typeS* newInstS(uint8_t op,uint32_t i){
	typeS* newT = (typeS*)malloc(sizeof(typeS));
	newT->opcode = op;
	newT->i = i;
	return newT;
}

void showReg(ArqResources* arq){
	for(int i = 0; i < 26;i++){
		printf("R[%d] = 0x%08X\n",i,arq->Reg[i]);
	}
	printf("==============================\n");
	printf("IR = 0x%08X\n",arq->Reg[28]);
	printf("PC = 0x%08X\n",arq->Reg[29]);
	printf("SP = 0x%08X\n",arq->Reg[30]);
	printf("SR = 0x%08X\n",arq->Reg[31]);
	printf("==============================\n");

}

void showMemory(ArqResources* arq){
	printf("--------------------------------------------\n");
	printf("                  Memória                   \n");
	printf("--------------------------------------------\n");
	for(int i = 0; i < 186; i = i + 1) {
		// Impressao lado a lado
		printf("0x%08X: 0x%08X (0x%02X 0x%02X 0x%02X 0x%02X)\n", i << 2, arq->Mem[i], ((uint8_t*)(arq->Mem))[(i << 2) + 3], ((uint8_t*)(arq->Mem))[(i << 2) + 2], ((uint8_t*)(arq->Mem))[(i << 2) + 1], ((uint8_t*)(arq->Mem))[(i << 2) + 0]);
	}
	printf("--------------------------------------------\n");

}

void processFile(FILE* input,FILE* output){
	ArqResources* newArq = inicialization();
	uint8_t i = 0;
	while(!feof(input)){
		fscanf(input,"0x%8X\n",&newArq->Mem[i]);
		i++;
	}
	showMemory(newArq);
	showReg(newArq);
	i = 0;
	while(i < 186){
		newArq->Reg[28] = newArq->Mem[newArq->Reg[29] >> 2];
		uint8_t opcode = (newArq->Reg[28] & (0b111111 << 26)) >> 26;
		char optype = opcodeType(opcode);
		typeU* newIU = malloc(sizeof(typeU));
		if(optype == 'U'){
			newIU->opcode = opcode;
			newIU->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
			//printf("res = 0x%08X\n",(newArq->Reg[28] & (0b11111 << 16)) >> 16);
			newIU->y = (newArq->Reg[28] & (0b11111 << 11)) >> 11;
			newIU->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
			newIU->l = (newArq->Reg[28] & (0b11111111111));
		}
		printf("0x%08X - 0x%08X: Opcode: 0x%08X - type:%c - ",i << 2,newArq->Mem[newArq->Reg[29] >> 2],opcode,opcodeType(opcode)) ;
		if(optype == 'U' && newIU != NULL) printf("x = 0x%08X,y = 0x%08X,z = 0x%08X,l = 0x%08X \n",newIU->x,newIU->y,newIU->z,newIU->l); else printf("\n");
		newArq->Reg[29] = newArq->Reg[29] + 4;
		i++;
	}
}

int main(int argc, char* argv[]){
	
	char* inputFile = argv[1];
	char* outputFile = argv[2];

	FILE* input = fopen(inputFile,"r");
	if(!input){
		return 1;
	}

	FILE* output = fopen(outputFile, "w");
	if(!output){
		fclose(input);
		return 1;
	}

	processFile(input,output);

	fclose(input);
	fclose(output);

	return 0;
}
