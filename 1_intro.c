#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct ArqResources{
	uint32_t* Mem;
	uint32_t Reg[32];
	uint32_t xyl;
	uint8_t x,y,i,z;
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

ArqResources* inicialization(uint8_t x,uint8_t y,uint8_t i,uint8_t z,uint32_t xyl){
	ArqResources* newArq = (ArqResources*)malloc(sizeof(ArqResources));
	newArq->Mem = (uint32_t*)calloc(8,1024);
	newArq->xyl = xyl;
	newArq->x = x;
	newArq->z = z;
	newArq->i = i;
	newArq->y = y;
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
	ArqResources* newArq = inicialization(0,0,0,0,0);
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
		printf("0x%08X: Opcode: 0x%08X - type:%c\n",newArq->Mem[newArq->Reg[29] >> 2],opcode,opcodeType(opcode));
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
