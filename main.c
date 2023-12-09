#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

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
	int16_t i;
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
// OBS SR 6->ZN ,5->ZD, 4->SN , 3->OV, 2->IV , 1- --- , 0->CY;



uint32_t ShiftBit(uint32_t bits, uint8_t shift,uint8_t quantbits){
	uint32_t molde = 0;
	for (int i = 0; i < quantbits; i++) {
        molde |= (1u << i); // Define o i-ésimo bit como 1
    }
	return (bits & (molde << shift)) >> shift;
}

void changeSR(uint8_t ZN,uint8_t ZD,uint8_t SN,uint8_t OV,uint8_t IV,uint8_t CY,ArqResources* arq){
	uint32_t a_ZN,a_ZD,a_SN,a_OV,a_IV,a_CY,molde;
	a_ZN = ShiftBit(arq->Reg[31],6,1);
	a_ZD = ShiftBit(arq->Reg[31],5,1);
	a_SN = ShiftBit(arq->Reg[31],4,1);
	a_OV = ShiftBit(arq->Reg[31],3,1);
	a_IV = ShiftBit(arq->Reg[31],2,1);
	a_CY = ShiftBit(arq->Reg[31],0,1);

	if(a_ZN != ZN) a_ZN = ZN;
	if(a_ZD != ZD) a_ZD = ZD;
	if(a_SN != SN) a_SN = SN;
	if(a_OV != OV) a_OV = OV;
	if(a_IV != IV) a_IV = IV;
	if(a_CY != CY) a_CY = CY;

	molde = (a_ZN << 6) + (a_ZD << 5) + (a_SN << 4) + (a_OV << 3) + (a_IV << 2) + (a_CY);
	arq->Reg[31] = molde;
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



void executionInstructionTypeU(ArqResources* arq,typeU* instruction,uint32_t i){
	switch(instruction->opcode){
		case 0b000000:
			char instrucao[30];
			uint32_t xyl = arq->Reg[28] & 0x1FFFFF;
			if(instruction->z == 28){
				sprintf(instrucao, "mov ir,%u", xyl);
				printf("0x%08X:\t%-25s\tIR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 29){
				sprintf(instrucao, "mov pc,%u", xyl);
				printf("0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao,  arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 30){
				sprintf(instrucao, "mov sp,%u",xyl);
				printf("0x%08X:\t%-25s\tSP=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 31){
				sprintf(instrucao, "mov sr,%u", xyl);
				printf("0x%08X:\t%-25s\tSR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else {
				sprintf(instrucao, "mov r%u,%u", instruction->z, xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tR%u=0x%08X\n", arq->Reg[29],instrucao, instruction->z, arq->Reg[instruction->z]);
			}
			break;
		default:
			printf("[INSTRUÇÃO NÃO MAPEADA - U]\n");
			break;
	}
}

void executionInstructionTypeF(ArqResources* arq,typeF* instruction,uint32_t i){
	char instrucao[30];
	switch(instruction->opcode){
		case 0b011010:
			sprintf(instrucao, "l32 r%d,[r%d%s%d]", instruction->z,arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i);
			arq->Reg[instruction->z] = arq->Mem[(arq->Reg[instruction->x] + instruction->i)];
			printf("0x%08X:\t%-25s\tR%d=MEM[0x%08X]=0x%08X\n", arq->Reg[29],instrucao,instruction->z,(arq->Reg[instruction->x] + instruction->i) << 2,arq->Reg[instruction->z]);
			break;
		case 0b011000:
			sprintf(instrucao,"l8 r%d,[r%d%s%d]",instruction->z,arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i);
			arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2]),0,8);
			printf("0x%08X:\t%-25s\tR%u=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,instruction->z,arq->Reg[instruction->x] + instruction->i,arq->Reg[instruction->z]);
			break;
		case 0b011001:
			sprintf(instrucao, "l16 r%d,[r%d%s%d]", instruction->z,arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i);
			arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 1]),0,16);
			printf("0x%08X:\t%-25s\tR%d=MEM[0x%08X]=0x%04X\n", arq->Reg[29],instrucao,instruction->z,(arq->Reg[instruction->x] + instruction->i) << 1,arq->Reg[instruction->z]);
			break;
		case 0b011011:
			sprintf(instrucao,"s8 [r%d%s%d],r%d",arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i,instruction->z);
			uint8_t b1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2]),0,8);
			uint8_t  b2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2]),8,8);
			uint8_t  b3 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2]),16,8);
			uint8_t  b4 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2]),24,8);
			uint8_t b1_newb = ShiftBit((arq->Reg[instruction->z]),0,8);
			b1 = b1_newb;
			arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2] = (b4 << 24) + (b3 << 16) + (b2 << 8) + b1;
			printf("0x%08X:\t%-25s\tMEM[0x%08X]=R%d=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + instruction->i),instruction->z,arq->Reg[instruction->z]);
			break;
		case 0b011100:
			sprintf(instrucao,"s16 [r%d%s%d],r%d",arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i,instruction->z);
			uint16_t b1_1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 1]),0,16);
			uint16_t  b2_2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 1]),16,16);
			uint16_t b1_1_newb = ShiftBit((arq->Reg[instruction->z]),0,16);
			b1_1 = b1_1_newb;
			arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2] = (b2_2 << 16) + b1_1;
			printf("0x%08X:\t%-25s\tMEM[0x%08X]=R%d=0x%04X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + instruction->i) << 1,instruction->z,arq->Reg[instruction->z]);
			break;
		case 0b011101:
			sprintf(instrucao,"s32 [r%d%s%d],r%d",arq->Reg[instruction->x],(instruction->i >= 0) ? ("+") : (""),instruction->i,instruction->z);
			arq->Mem[(arq->Reg[instruction->x] + instruction->i) >> 2] = arq->Reg[instruction->z];
			printf("0x%08X:\t%-25s\tMEM[0x%08X]=R%d=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + instruction->i) << 2,instruction->z,arq->Reg[instruction->z]);
		case 0b010010:
			sprintf(instrucao,"addi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] + instruction->i;
			uint8_t ZN_1 = arq->Reg[instruction->z] == 0;
			uint8_t SN_1 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_1 = (ShiftBit(arq->Reg[instruction->x],31,1) == ShiftBit(instruction->i,15,1)) && (ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(instruction->x,31,1));
			uint8_t CY_1 = (ShiftBit(arq->Reg[instruction->z],32,1)) == 1;ShiftBit(arq->Reg[31],5,1);
			changeSR(ZN_1,ShiftBit(arq->Reg[31],5,1),SN_1,OV_1,ShiftBit(arq->Reg[31],2,1),CY_1,arq);
			printf("0x%08X:\t%-25s\tR%d=R%d+0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010011:
			sprintf(instrucao,"subi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] - instruction->i;
			uint8_t ZN_2 = arq->Reg[instruction->z] == 0;
			uint8_t SN_2 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_2 = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(instruction->i,15,1)) && (ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(instruction->x,31,1));
			uint8_t CY_2 = (ShiftBit(arq->Reg[instruction->z],32,1)) == 1;
			changeSR(ZN_2,ShiftBit(arq->Reg[31],5,1),SN_2,OV_2,ShiftBit(arq->Reg[31],2,1),CY_2,arq);
			printf("0x%08X:\t%-25s\tR%d=R%d-0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010100:
			sprintf(instrucao,"muli r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] * instruction->i;
			uint8_t ZN_3 = arq->Reg[instruction->z] == 0;
			uint8_t OV_3 = (ShiftBit(arq->Reg[instruction->z],31,1) != 0);
			changeSR(ZN_3,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),OV_3,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			printf("0x%08X:\t%-25s\tR%d=R%d*0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010101:
			sprintf(instrucao,"divi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			uint8_t ZN_4 = arq->Reg[instruction->z] == 0;
			uint8_t ZD_4 = (instruction->i == 0);
			uint8_t OV_4 = 0;
			if(!ZD_4){
				 arq->Reg[instruction->z] = arq->Reg[instruction->x] /instruction->i;
			}
			changeSR(ZN_4,ZD_4,ShiftBit(arq->Reg[31],3,1),OV_4,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			printf("0x%08X:\t%-25s\tR%d=R%d/0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010110:
			sprintf(instrucao,"modi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			uint8_t ZN_5 = arq->Reg[instruction->z] == 0;
			uint8_t ZD_5 = (instruction->i == 0);
			uint8_t OV_5 = 0;
			if(!ZD_5){
				arq->Reg[instruction->z] = arq->Reg[instruction->x] % instruction->i;
			}
			changeSR(ZN_5,ZD_5,ShiftBit(arq->Reg[31],4,1),OV_5,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			printf("0x%08X:\t%-25s\tR%d=R%d%%0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010111:
			sprintf(instrucao,"cmpi r%d,%d",instruction->x,instruction->i);
			uint32_t CMPI = arq->Reg[instruction->x] - instruction->i;
			uint8_t RegX31 = ShiftBit(arq->Reg[31],31,1);
			uint8_t CMPI31 = ShiftBit(CMPI,31,1);
			uint8_t I15 = ShiftBit(instruction->i,15,1);
			changeSR(CMPI == 0,ShiftBit(arq->Reg[31],4,1),CMPI31 == 1,(RegX31 != CMPI31) && (RegX31 != I15),ShiftBit(arq->Reg[31],2,1),ShiftBit(CMPI,32,1) == 1,arq);
			printf("0x%08X:\t%-25s\tSR=0x%08x\n",arq->Reg[29],instrucao,arq->Reg[31]);
			break;
		default:
			printf("[INSTRUÇÃO NÃO MAPEADA - F - Opcode = 0x%08X]\n",instruction->opcode);
			break;
	}
}

void executionInstructionTypeS(ArqResources* arq,typeS* instruction,uint32_t i){
	char instrucao[30];
	switch(instruction->opcode){
		case 0b110111:
			sprintf(instrucao, "bun %d", instruction->i);
			printf("0x%08X:\t%-25s\tPC=0x%08x\n", arq->Reg[29],instrucao,arq->Reg[29] + 4*instruction->i );
			arq->Reg[29] = arq->Reg[29] + 4*instruction->i ;
			break;
		case 0b111111:
			sprintf(instrucao, "int %d", instruction->i);
			printf("0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08x\n", arq->Reg[29],instrucao,0,0);
			break;
		case 0b101010:
			sprintf(instrucao,"bae %d",instruction->i);
			if(ShiftBit(arq->Reg[31],0,1) == 0)
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]);
			break;
		case 0b101011:
			sprintf(instrucao,"bat %d",instruction->i);
			if(ShiftBit(arq->Reg[29],6,1) == 0 && ShiftBit(arq->Reg[29],0,1) == 0)
				arq->Reg[29] = arq->Reg[29] + 4 +(instructon->i <<	2);
			printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]);
		default:
			printf("[INSTRUÇÃO NÃO MAPEADA - S]\n");
			break;
	}
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
	
	//s
	i = 0;
	uint8_t exec = 1;
	printf("[START OF SIMULATION]\n");
	while(exec){
		newArq->Reg[28] = newArq->Mem[newArq->Reg[29] >> 2];
		uint8_t opcode = (newArq->Reg[28] & (0b111111 << 26)) >> 26;
		char optype = opcodeType(opcode);
		typeU* newIU = malloc(sizeof(typeU));
		typeF* newIF = malloc(sizeof(typeF));
		typeS* newIS = malloc(sizeof(typeS));
		if(optype == 'U'){
			newIU->opcode = opcode;
			newIU->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
			newIU->y = (newArq->Reg[28] & (0b11111 << 11)) >> 11;
			newIU->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
			newIU->l = (newArq->Reg[28] & (0b11111111111));
			executionInstructionTypeU(newArq,newIU,i);
		}
		else if(optype == 'F'){
			newIF->opcode = opcode;
			newIF->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
			newIF->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
			newIF->i = (newArq->Reg[28] & (0xFFFF));
			executionInstructionTypeF(newArq,newIF,i);
		}
		else if(optype == 'S'){
			newIS->opcode = opcode;
			newIS->i = (newArq->Reg[28] & (0b11111111111111111111111111));
			executionInstructionTypeS(newArq,newIS,i);
			if(opcode == 0b111111 && newIS->i == 0){
				exec = 0;
				break;
			}
		}else if(optype != 'U' && optype != 'F' && optype != 'S')	{
			printf("[ERRO - INSTRUÇÃO NÃO ENCONTRADA]\n");
		}

		newArq->Reg[29] = newArq->Reg[29] + 4;
		i++;
	}
	printf("[END OF SIMULATION]\n");
	//showMemory(newArq);
	showReg(newArq);
}

void Tests(){
	Equal_Decimal(ShiftBit(0b10110011,6,1),0b0,"Test 1 - Passed");
	//Equal_Decimal(changeSR(1,1,0,1,0,1),0b1101001,"Test 2 - Passed");
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
	Tests();
	return 0;

	
	//return 0;
}
