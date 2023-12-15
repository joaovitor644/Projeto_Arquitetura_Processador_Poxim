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
	int16_t i;
}typeF;

typedef struct typeS{
	uint8_t opcode;
	uint32_t i;
}typeS;

ArqResources* inicialization(){
	ArqResources* newArq = (ArqResources*)malloc(sizeof(ArqResources));
	newArq->Mem = (uint32_t*)calloc(32,1024);
	return newArq;
}
// OBS SR 6->ZN ,5->ZD, 4->SN , 3->OV, 2->IV , 1- --- , 0->CY;

char* whyIsReg(uint8_t pos){
	char* res = (char*)malloc(sizeof(4));
	if(!pos)
		sprintf(res,"r0");
	if(pos >= 28 && pos <= 31){
		switch(pos){
			case 28:
				return "ir";
				break;
			case 29:
				return "pc";
				break;
			case 30:
				return "sp";
				break;
			case 31:
				return "sr";
				break;
		}
	} else {
		
		if(!pos)
			sprintf(res,"r0");
		else 
			sprintf(res,"r%d",pos);
		return res;
	}
}

uint32_t ShiftBit(uint32_t bits, uint8_t shift,uint8_t quantbits){
	uint32_t molde = 0;
	for (int i = 0; i < quantbits; i++) {
        molde |= (1u << i);
    }
	return (bits & (molde << shift)) >> shift;
}

void changeSR(uint8_t ZN,uint8_t ZD,uint8_t SN,uint8_t OV,uint8_t IV,uint8_t CY,ArqResources* arq){
	uint32_t a_ZN,a_ZD,a_SN,a_OV,a_IV,a_CY,molde;
	a_ZN = ShiftBit(arq->Reg[31],6,1);
	a_ZD = ShiftBit(arq->Reg[31],6,1);
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
		char instrucao[30];
		int64_t cyv;
		uint8_t n_ZD;
		uint8_t n_ZN;
		uint8_t n_SN;
		uint8_t n_OV;
		uint8_t n_IV;
		uint8_t n_CY;
		char* res1;
		char* res2;
		char* res3;
		case 0b000000:
			uint32_t xyl = arq->Reg[28] & 0x1FFFFF;
			//printf("-----> xyl = 0x%08x,R28=0x%08X\n",xyl,arq->Reg[28]);
			if(instruction->z == 28){
				sprintf(instrucao, "mov ir,%d", xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tIR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 29){
				sprintf(instrucao, "mov pc,%d", xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao,  arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 30){
				sprintf(instrucao, "mov sp,%d",xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tSP=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 31){
				sprintf(instrucao, "mov sr,%d", xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tSR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else {
				sprintf(instrucao, "mov r%d,%d", instruction->z, xyl);
				arq->Reg[instruction->z] = xyl;
				printf("0x%08X:\t%-25s\tR%u=0x%08X\n", arq->Reg[29],instrucao, instruction->z, arq->Reg[instruction->z]);
			}
			break;
		case 0b000001:
			uint32_t x4 = ShiftBit(arq->Reg[28] & 0x1FFFFF,20,1);
			int32_t xyl_1 = ((int32_t)(arq->Reg[28] & 0x1FFFFF));
			int32_t x4xyl = x4 == 1 ? (0x7FF << 21) + xyl_1 : (arq->Reg[28] & 0x1FFFFF);
			if(instruction->z == 28){
				sprintf(instrucao, "movs ir,%d", x4xyl);
				arq->Reg[instruction->z] = x4xyl;
				printf("0x%08X:\t%-25s\tIR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 29){
				sprintf(instrucao, "movs pc,%d", x4xyl);
				arq->Reg[instruction->z] = x4xyl;
				printf("0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao,  arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 30){
				arq->Reg[instruction->z] = x4xyl;
				sprintf(instrucao, "movs sp,%d",x4xyl);
				printf("0x%08X:\t%-25s\tSP=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else if(instruction->z == 31){
				sprintf(instrucao, "movs sr,%d", x4xyl);
				arq->Reg[instruction->z] = x4xyl;
				printf("0x%08X:\t%-25s\tSR=MEM[0x%08X]=0x%02X\n", arq->Reg[29],instrucao, arq->Reg[instruction->x] + i, arq->Reg[instruction->z]);
			}
			else {
				sprintf(instrucao, "movs r%d,%d", instruction->z, x4xyl);
				arq->Reg[instruction->z] = x4xyl;
				printf("0x%08X:\t%-25s\tR%d=0x%08X\n", arq->Reg[29],instrucao, instruction->z, arq->Reg[instruction->z]);
			}
			break;
		case 0b000010:
			cyv = ((int64_t)(arq->Reg[instruction->x]) + (int64_t)(arq->Reg[instruction->y]));
			arq->Reg[instruction->z] = (uint64_t)arq->Reg[instruction->x] + (uint64_t)arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],32,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],32,1) == ShiftBit(arq->Reg[instruction->y],32,1)) && ShiftBit(arq->Reg[instruction->z],32,1) != ShiftBit(arq->Reg[instruction->x],32,1);
			n_CY = ShiftBit(cyv,33,1) == 1;
			sprintf(instrucao,"add r%d,r%d,r%d",instruction->z,instruction->x,instruction->y);
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			printf("0x%08X:\t%-25s\tR%u=R%d+R%d=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->y,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000011:
			cyv = (int64_t)(arq->Reg[instruction->x]) - (int64_t)(arq->Reg[instruction->y]);
			arq->Reg[instruction->z] = (int64_t)arq->Reg[instruction->x] - (int64_t)arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(arq->Reg[instruction->y],31,1)) && ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(arq->Reg[instruction->x],31,1);
			n_CY = ShiftBit(cyv,32,1) == 1;
			sprintf(instrucao,"sub r%d,r%d,r%d",instruction->z,instruction->x,instruction->y);
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			printf("0x%08X:\t%-25s\tR%u=R%d-R%d=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->y,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000100:
			uint32_t id_op = ShiftBit(instruction->l,8,3);
			if(id_op == 0b000){
				cyv = (arq->Reg[instruction->x])*(arq->Reg[instruction->y]);
				uint32_t L4 = ShiftBit(cyv,31,32);
				uint32_t BitsMsig = ShiftBit(cyv,0,32);
				uint32_t l4_0 = ShiftBit(instruction->l,0,5);
				arq->Reg[l4_0] = L4;
				arq->Reg[instruction->z] = BitsMsig;
				n_CY = arq->Reg[l4_0] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"mul r%d,r%d,r%d,r%d",l4_0,instruction->z,instruction->x,instruction->y);
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				printf("0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(l4_0),whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),cyv,arq->Reg[31]);
			}
			if(id_op == 0b001){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int64_t)(arq->Reg[instruction->y])) << (ShiftBit(instruction->l,0,5) +1);
				arq->Reg[instruction->z] = ShiftBit(cyv,31,32);
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_CY = ShiftBit(arq->Reg[instruction->z],31,1) != 0;
				n_ZN = cyv == 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				sprintf(instrucao,"sll %s,%s,%s,%d",whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->z),ShiftBit(instruction->l,0,5));
				printf("0x%08X:\t%-25s\t%s:%s=%s<<%d=0x%016X,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),ShiftBit(instruction->l,0,5) +1,cyv,arq->Reg[31]);
				
			}
			if(id_op == 0b010){
				/*int64_t cyv2 = (int64_t)(arq->Reg[instruction->x])*(int64_t)(arq->Reg[instruction->y]);
				int32_t L4 = ShiftBit(cyv2,31,32);
				int32_t BitsMsig = ShiftBit(cyv2,0,32);
				int32_t l4_0 = ShiftBit(instruction->l,0,5);
				arq->Reg[l4_0] = L4;
				arq->Reg[instruction->z] = BitsMsig;
				n_CY = arq->Reg[l4_0] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"muls %s,%s,%s,%s",whyIsReg(l4_0),whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y));
				changeSR(a_ZD,n_ZN,a_SN,ShiftBit(arq->Reg[31],3,1),a_IV,n_CY,arq);
				printf("0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(l4_0),whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),cyv2,arq->Reg[31]);*/
				printf("muls\n");
			}
			if(id_op == 0b011){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int64_t)(arq->Reg[instruction->y])) << ShiftBit(instruction->l,0,5) +1;
				arq->Reg[instruction->x] = ShiftBit(cyv,32,32);
				arq->Reg[instruction->z] = ShiftBit(cyv,0,31);
				n_ZN = cyv == 0;
				n_OV = arq->Reg[instruction->z] != 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				sprintf(instrucao,"sla %s,%s,%s,%d",whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->x),ShiftBit(instruction->l,0,5));
				printf("0x%08X:\t%-25s\t%s:%s=%s:%s<<%d=0x%016X,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->z),whyIsReg(instruction->x),ShiftBit(instruction->l,0,5) +1,cyv,arq->Reg[31]);
			}
			if(id_op == 0b100){
				sprintf(instrucao,"div %s,%s,%s,%s",whyIsReg(ShiftBit(instruction->l,0,5)),whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y));
				if(arq->Reg[instruction->y] != 0 ){
					arq->Reg[ShiftBit(instruction->l,0,5)] = arq->Reg[instruction->x] % arq->Reg[instruction->y];
					arq->Reg[instruction->z] = arq->Reg[instruction->x] / arq->Reg[instruction->y];
				}
				changeSR(arq->Reg[instruction->z] == 0,arq->Reg[instruction->y] == 0,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				printf("0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5)),whyIsReg(instruction->x),whyIsReg(instruction->x),arq->Reg[ShiftBit(instruction->l,0,5)],whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),arq->Reg[instruction->z],arq->Reg[31]);
			}
			if(id_op == 0b101){
				printf("srl\n");
			}
			if(id_op == 0b110){
				printf("divs\n");
			}
			if(id_op == 0b111){
				printf("sra\n");
			}
			break;
		case 0b000101:
			int32_t cyv2 = (int32_t)(arq->Reg[instruction->x]) - (int32_t)(arq->Reg[instruction->y]);
			n_ZN = cyv2 == 0;
			n_SN = ShiftBit(cyv2,31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(arq->Reg[instruction->y],31,1)) && (ShiftBit(cyv2,31,1) != ShiftBit(arq->Reg[instruction->x],31,1));
			n_CY = ShiftBit(cyv2,32,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			sprintf(instrucao,"cmp %s,%s",whyIsReg(instruction->x),whyIsReg(instruction->y));
			printf("0x%08X:\t%-25s\tSR=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[31]);
			break;
		case 0b000110:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] & arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"and %s,%s,%s",whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y));
			printf("0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000111:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] | arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"or %s,%s,%s",whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y));
			printf("0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b01000:
			arq->Reg[instruction->z] = ~arq->Reg[instruction->x];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"not %s,%s",whyIsReg(instruction->z),whyIsReg(instruction->x));
			printf("0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b001001:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] ^ arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"xor %s,%s,%s",whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y));
			printf("0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z),whyIsReg(instruction->x),whyIsReg(instruction->y),arq->Reg[instruction->z],arq->Reg[31]);
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
			break;
		case 0b010010:
			sprintf(instrucao,"addi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			
			uint64_t sum = (uint64_t)arq->Reg[instruction->x] + (uint64_t)instruction->i;
			arq->Reg[instruction->z] = (uint64_t)arq->Reg[instruction->x] + (uint64_t)instruction->i;
			uint8_t ZN_1 = arq->Reg[instruction->z] == 0;
			uint8_t SN_1 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_1 = (ShiftBit(arq->Reg[instruction->x],31,1) == ShiftBit(instruction->i,15,1)) && (ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(instruction->x,31,1));
			uint8_t CY_1 = ((sum & (0b1 << 32)) >> 32 )== 1;
			changeSR(ZN_1,ShiftBit(arq->Reg[31],5,1),SN_1,OV_1,ShiftBit(arq->Reg[31],2,1),CY_1,arq);
			printf("0x%08X:\t%-25s\tR%d=R%d+0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010011:
			sprintf(instrucao,"subi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] - instruction->i;
			uint8_t ZN_2 = arq->Reg[instruction->z] == 0;
			uint8_t SN_2 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_2 = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(instruction->i,15,1)) && (ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(instruction->x,31,1));
			uint8_t CY_2 = ((arq->Reg[instruction->z] & (0b1 << 32)) >> 32) == 1;
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
			uint8_t ZD_4 = (instruction->i == 0);
			if(!ZD_4){
				 arq->Reg[instruction->z] = arq->Reg[instruction->x] /instruction->i;
			}
			uint8_t ZN_4 = arq->Reg[instruction->z] == 0;
			
			uint8_t OV_4 = 0;
			changeSR(ZN_4,ZD_4,ShiftBit(arq->Reg[31],3,1),OV_4,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			printf("0x%08X:\t%-25s\tR%d=R%d/0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010110:
			sprintf(instrucao,"modi r%d,r%d,%d",instruction->z,instruction->x,instruction->i);
			uint8_t ZD_5 = (instruction->i == 0);
			if(!ZD_5){
				arq->Reg[instruction->z] = arq->Reg[instruction->x] % instruction->i;
			}
			uint8_t ZN_5 = arq->Reg[instruction->z] == 0;
			
			uint8_t OV_5 = 0;
			changeSR(ZN_5,ZD_5,ShiftBit(arq->Reg[31],4,1),OV_5,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			printf("0x%08X:\t%-25s\tR%d=R%d%%0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,instruction->z,instruction->x,instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b010111:
			sprintf(instrucao,"cmpi r%d,%d",instruction->x,instruction->i);
			int64_t sinal = (((instruction->i & (0b1 << 15))>>15)<< 16);
			int64_t CMPI = (int64_t)arq->Reg[instruction->x] - (int64_t)(sinal+instruction->i);
			printf("i = 0x%08X\n",instruction->i);
			printf("sinal = 0x%016X\n",sinal);
			printf("CMPI = 0x%016X\n",CMPI);
			uint8_t RegX31 = ShiftBit(arq->Reg[31],31,1);
			uint8_t CMPI31 = ShiftBit(CMPI,31,1);
			uint8_t I15 = ShiftBit(instruction->i,15,1);
			changeSR(CMPI == 0,ShiftBit(arq->Reg[31],5,1),CMPI31 == 1,(RegX31 != CMPI31) && (RegX31 != I15),ShiftBit(arq->Reg[31],2,1),((CMPI & (0b1 << 32)) >> 32) == 1,arq);
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
			if(instruction->i == 0){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4*instruction->i + 4);
				arq->Reg[29] = arq->Reg[29] + 4*instruction->i + 4;
			}
			break;
		case 0b111111:
			sprintf(instrucao, "int %d", instruction->i);
			printf("0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08x\n", arq->Reg[29],instrucao,0,0);
			break;
		case 0b101010:
			sprintf(instrucao,"bae %d",instruction->i);
			if(ShiftBit(arq->Reg[31],0,1) == 0){
				
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			break;
		case 0b101011:
			sprintf(instrucao,"bat %d",instruction->i);
			if(ShiftBit(arq->Reg[29],6,1) == 0 && ShiftBit(arq->Reg[29],0,1) == 0){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101100:
			sprintf(instrucao,"bbe %d",instruction->i);
			if(ShiftBit(arq->Reg[29],6,1) == 1 || ShiftBit(arq->Reg[29],0,1) == 1){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101101:
			sprintf(instrucao,"bbt %d",instruction->i);
			if(ShiftBit(arq->Reg[31],0,1) == 1){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101110:
			sprintf(instrucao,"beq %d",instruction->i);
			if(ShiftBit(arq->Reg[31],5,1) == 1){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101111:
			sprintf(instrucao,"bge %d",instruction->i);
			if(ShiftBit(arq->Reg[31],4,1) == ShiftBit(arq->Reg[31],3,1)){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110000:
			sprintf(instrucao,"bgt %d",instruction->i);
			if(ShiftBit(arq->Reg[31],5,1) == 0 && (ShiftBit(arq->Reg[31],4,1) == ShiftBit(arq->Reg[31],3,1))){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110001:
			sprintf(instrucao,"biv %d",instruction->i);
			if(ShiftBit(arq->Reg[31],2,1)){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110010:
			sprintf(instrucao,"ble %d",instruction->i);
			if(ShiftBit(arq->Reg[29],5,1) == 1 || ShiftBit(arq->Reg[29],4,1) != ShiftBit(arq->Reg[29],3,1)){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110011:
			sprintf(instrucao,"blt %d",instruction->i);
			if(ShiftBit(arq->Reg[31],4,1) != ShiftBit(arq->Reg[31],3,1)){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110100:
			sprintf(instrucao,"bne %d",instruction->i);
			if(ShiftBit(arq->Reg[31],5,1) == 0){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110101:
			sprintf(instrucao,"bni %d",instruction->i);
			if(ShiftBit(arq->Reg[31],2,1) == 0){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110110:
			sprintf(instrucao,"bnz %d",instruction->i);
			if(ShiftBit(arq->Reg[31],5,1) == 0){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b111000:
			sprintf(instrucao,"bzd %d",instruction->i);
			if(ShiftBit(arq->Reg[31],6,1)){
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (instruction->i << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (instruction->i << 2);
			} else {
				printf("0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
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
	for(int i = 0; i < 9100; i = i + 1) {
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
			newIU->l = (newArq->Reg[28] & (0x7FF));
			executionInstructionTypeU(newArq,newIU,i);
			newArq->Reg[29] = newArq->Reg[29] + 4;
		}
		else if(optype == 'F'){
			newIF->opcode = opcode;
			newIF->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
			newIF->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
			newIF->i = (newArq->Reg[28] & (0xFFFF));
			executionInstructionTypeF(newArq,newIF,i);
			newArq->Reg[29] = newArq->Reg[29] + 4;
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

		i++;
	}
	printf("[END OF SIMULATION]\n");
	//showMemory(newArq);
	//showReg(newArq);
}

/*void Tests(){
	Equal_Decimal(ShiftBit(0x00000000F0000000,31,1),0b1,"Test 1 - Passed");
	//Equal_Decimal(changeSR(1,1,0,1,0,1),0b1101001,"Test 2 - Passed");
}*/

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
	//Tests();
	return 0;

	
	//return 0;
}
