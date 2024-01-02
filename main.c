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

char* whyIsReg(uint8_t pos,int8_t op){
	char* res = (char*)malloc(sizeof(4));
	if(pos >= 28 && pos <= 31){
		switch(pos){
			case 28:
				return op == 1? "ir": "IR" ;
				break;
			case 29:
				return op == 1? "pc": "PC" ;
				break;
			case 30:
				return op == 1? "sp": "SP" ;
				break;
			case 31:
				return op == 1? "sr": "SR" ;
				break;
		}
	} else {
		
		if(!pos)
			sprintf(res,op == 1? "r0" : "R0");
		else 
			sprintf(res,op == 1? "r%d" : "R%d",pos);
		return res;
	}
}

uint8_t verifyList(uint32_t* list){
	uint8_t i;
	for(i = 0; i< 5 ;i++)
		if(list[i] == 0)
			break;
	return i;
}

const char* whyIsRegHex(uint32_t reg) {
    static char buffer[10];
    sprintf(buffer, "0x%08X", reg);
    return buffer;
}

uint32_t ShiftBit(uint32_t bits, uint8_t shift,uint8_t quantbits){
	uint32_t molde = 0;
	for (int i = 0; i < quantbits; i++) {
        molde |= (1u << i);
    }
	return (bits & (molde << shift)) >> shift;
}
uint32_t ShiftBit64(uint64_t bits, uint8_t shift,uint8_t quantbits){
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
	if(op <= 0b001011 && op >= 0b0000 )
		return 'U';
	else if((op <= 0b011101 && op >= 0b011000) || (op <= 0b010111 && op >= 0b010010) || op == 0b011110 || op == 0b011111 )
		return 'F';
	else if(op >= 0b101010 && op <= 0b111111 || op == 0b111001)
		return 'S';
	else
		return 'E';
}



void executionInstructionTypeU(ArqResources* arq,typeU* instruction,uint32_t i,FILE* output){
	switch(instruction->opcode){
		char instrucao[30];
		uint64_t cyv;
		uint8_t n_ZD;
		uint8_t n_ZN;
		uint8_t n_SN;
		uint8_t n_OV;
		uint8_t n_IV;
		uint8_t n_CY;
		char* res1;
		char* res2;
		char* res3;
		int8_t num;
		case 0b000000:
			uint32_t xyl = arq->Reg[28] & 0x1FFFFF;
			sprintf(instrucao, "mov %s,%d", whyIsReg(instruction->z,1), xyl);
			arq->Reg[instruction->z] = xyl;
			fprintf(output,"0x%08X:\t%-25s\t%s=0x%08X\n", arq->Reg[29],instrucao, whyIsReg(instruction->z,2), arq->Reg[instruction->z]);
			break;
		case 0b000001:
			uint32_t x4 = ShiftBit(arq->Reg[28] & 0x1FFFFF,20,1);
			int32_t xyl_1 = ((int32_t)(arq->Reg[28] & 0x1FFFFF));
			int32_t x4xyl = x4 == 1 ? (0x7FF << 21) + xyl_1 : (arq->Reg[28] & 0x1FFFFF);
			sprintf(instrucao, "movs %s,%d", whyIsReg(instruction->z,1), x4xyl);
			arq->Reg[instruction->z] = x4xyl;
			fprintf(output,"0x%08X:\t%-25s\t%s=0x%08X\n", arq->Reg[29],instrucao, whyIsReg(instruction->z,2), arq->Reg[instruction->z]);
			break;
		case 0b000010:
			cyv = ((int64_t)(arq->Reg[instruction->x]) + (int64_t)(arq->Reg[instruction->y]));
			arq->Reg[instruction->z] = (uint64_t)arq->Reg[instruction->x] + (uint64_t)arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],32,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],32,1) == ShiftBit(arq->Reg[instruction->y],32,1)) && ShiftBit(arq->Reg[instruction->z],32,1) != ShiftBit(arq->Reg[instruction->x],32,1);
			n_CY = ShiftBit(cyv,33,1) == 1;
			sprintf(instrucao,"add %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000011:
			cyv = (int64_t)(arq->Reg[instruction->x]) - (int64_t)(arq->Reg[instruction->y]);
			arq->Reg[instruction->z] = (int64_t)arq->Reg[instruction->x] - (int64_t)arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(arq->Reg[instruction->y],31,1)) && ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(arq->Reg[instruction->x],31,1);
			n_CY = (cyv & (0x1FFFFFFFF)) >> 32 == 1;
			sprintf(instrucao,"sub %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000100:
			uint32_t id_op = ShiftBit(instruction->l,8,3);
			if(id_op == 0b000){
				cyv = (int64_t)(arq->Reg[instruction->x])*(int64_t)(arq->Reg[instruction->y]);
				//printf("cyv = 0x%016llX\n",cyv);
				uint32_t L41 = (cyv >> 32);
				//printf("l4 = 0x%08X\n",L41);
				uint32_t BitsMsig = ShiftBit(cyv,0,32);
				uint32_t l4_0 = ShiftBit(instruction->l,0,5);
				arq->Reg[l4_0] = L41;
				arq->Reg[instruction->z] = BitsMsig;
				if(l4_0 == 0) arq->Reg[l4_0] = 0; 
				if(instruction->z == 0) arq->Reg[instruction->z] = 0; 
				n_CY = arq->Reg[l4_0] != 0;
				n_ZN = cyv == 0;

				sprintf(instrucao,"mul %s,%s,%s,%s",whyIsReg(l4_0,1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(l4_0,2),whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),cyv,arq->Reg[31]);
			}
			if(id_op == 0b001){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int64_t)(arq->Reg[instruction->y])) << (ShiftBit(instruction->l,0,5) +1);
				arq->Reg[instruction->z] = cyv >> 32;
				//printf("cyv >> 32 = 0x%08X\n",arq->Reg[instruction->z] );
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_CY = arq->Reg[instruction->z]  != 0;
				n_ZN = cyv == 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				sprintf(instrucao,"sll %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s<<%d=0x%016llX,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5) +1,cyv,arq->Reg[31]);
				
			}
			if(id_op == 0b010){
				uint64_t cyv2 = (int64_t)(arq->Reg[instruction->x])*(int32_t)(arq->Reg[instruction->y]);
				int32_t L4 = ShiftBit(cyv2,31,32);
				int32_t BitsMsig = ShiftBit(cyv2,0,32);
				int32_t l4_0 = ShiftBit(instruction->l,0,5);
				arq->Reg[l4_0] = L4;
				arq->Reg[instruction->z] = BitsMsig;
				n_OV = arq->Reg[l4_0] != 0;
				n_ZN = cyv2 == 0;
				sprintf(instrucao,"muls %s,%s,%s,%s",whyIsReg(l4_0,1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s*%s=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(l4_0,2),whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),cyv2,arq->Reg[31]);
			}
			if(id_op == 0b011){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int32_t)(arq->Reg[instruction->y])) << (ShiftBit(instruction->l,0,5) +1);
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				arq->Reg[instruction->z] = ShiftBit64(cyv,32,32);
				if(instruction->x == 0) arq->Reg[instruction->x] = 0;
				if(instruction->z == 0) arq->Reg[instruction->z] = 0;
				n_ZN = cyv == 0;
				n_OV = arq->Reg[instruction->z] != 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				sprintf(instrucao,"sla %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));

				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s<<%d=0x%016llX,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5) +1,((arq->Reg[instruction->z] << 32) + (int32_t)arq->Reg[instruction->x]) ,arq->Reg[31]);
			}
			if(id_op == 0b100){
				if(instruction->y == 0) arq->Reg[0] = 0;
				uint32_t oldl4_0 = arq->Reg[ShiftBit(instruction->l,0,5)];
				sprintf(instrucao,"div %s,%s,%s,%s",whyIsReg(ShiftBit(instruction->l,0,5),1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				if(arq->Reg[instruction->y] != 0 ){
					arq->Reg[ShiftBit(instruction->l,0,5)] = arq->Reg[instruction->x] % arq->Reg[instruction->y];
					arq->Reg[instruction->z] = arq->Reg[instruction->x] / arq->Reg[instruction->y];
					oldl4_0 = arq->Reg[ShiftBit(instruction->l,0,5)];
					changeSR(arq->Reg[instruction->z] == 0,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),oldl4_0 != 0,arq);
					
				}else {
					changeSR(ShiftBit(arq->Reg[31],6,1),arq->Reg[instruction->y] == 0,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				}
				if(ShiftBit(instruction->l,0,5) == 0)  arq->Reg[0] = 0;
				fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),oldl4_0,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			}
			if(id_op == 0b101){
				cyv = ((int64_t)(arq->Reg[instruction->z] << 32) + (int64_t)(arq->Reg[instruction->y])) >> ShiftBit(instruction->l,0,5) + 1; 
				arq->Reg[instruction->z] = ShiftBit(cyv,31,32);
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_CY = arq->Reg[instruction->z] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"srl %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s>>%d=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5)+1,cyv,arq->Reg[31]);
			}
			if(id_op == 0b110){
				sprintf(instrucao,"divs %s,%s,%s,%s",whyIsReg(ShiftBit(instruction->l,0,5),1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				if(arq->Reg[instruction->y] != 0){
					arq->Reg[ShiftBit(instruction->l,0,5)] = (int32_t)arq->Reg[instruction->x] % (int32_t)arq->Reg[instruction->y];
					arq->Reg[instruction->z] =  (int32_t)arq->Reg[instruction->x] /  (int32_t)arq->Reg[instruction->y];
				}
				n_ZN = arq->Reg[instruction->z] == 0;
				n_ZD = arq->Reg[instruction->y] == 0;
				n_OV = arq->Reg[ShiftBit(instruction->l,0,5)] != 0;
				changeSR(n_ZN,n_ZD,ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[ShiftBit(instruction->l,0,5)],whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			}
			if(id_op == 0b111){
				cyv = ((int64_t)(arq->Reg[instruction->z] << 32) + (int32_t)(arq->Reg[instruction->y])) >> ShiftBit(instruction->l,0,5) + 1; 
				arq->Reg[instruction->z] = cyv >> 32;
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_OV = arq->Reg[instruction->z] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"sra %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s>>%d=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5)+1,cyv,arq->Reg[31]);
			}
			break;
		case 0b000101:
			int32_t cyv2 = (int32_t)(arq->Reg[instruction->x]) - (int32_t)(arq->Reg[instruction->y]);
			n_ZN = cyv2 == 0;
			n_SN = ShiftBit(cyv2,31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(arq->Reg[instruction->y],31,1)) && (ShiftBit(cyv2,31,1) != ShiftBit(arq->Reg[instruction->x],31,1));
			n_CY = ShiftBit(cyv2,32,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			sprintf(instrucao,"cmp %s,%s",whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\tSR=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[31]);
			break;
		case 0b000110:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] & arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"and %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b000111:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] | arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"or %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b01000:
			arq->Reg[instruction->z] = ~arq->Reg[instruction->x];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"not %s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b001001:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] ^ arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"xor %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			break;
		case 0b001010:
			if(arq->Reg[29] == 0x0000015C) printf("{%d,%d,%d,%d,%d}\n",ShiftBit(instruction->l,6,5),ShiftBit(instruction->l,0,5),instruction->x,instruction->y,instruction->z);
			
			uint32_t pushlist[] = {ShiftBit(instruction->l,6,5),ShiftBit(instruction->l,0,5),instruction->x,instruction->y,instruction->z};
			num = verifyList(pushlist);
			if(num == 0){
				sprintf(instrucao,"push -");
				fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]{}={}\n",arq->Reg[29],instrucao,arq->Reg[30]);
			} else {
				char instrucao2[22];
				char instrucao3[22];
				char finalInstrucao[52];
				char instrucaoValues[56];
				if (num >= 1) {
        			sprintf(instrucaoValues, "%s", whyIsRegHex(arq->Reg[pushlist[0]]));    			
   				}
			    for (int i = 1; i < num; ++i) {
			        strcat(instrucaoValues, ",");
			        strcat(instrucaoValues, whyIsRegHex(arq->Reg[pushlist[i]]));
			    }
				sprintf(instrucao,"push ");
				if (num >= 1) {
        			sprintf(instrucao2, "%s", whyIsReg(pushlist[0],1));
        			sprintf(instrucao3,"%s",whyIsReg(pushlist[0],2));
    			}
    			for (int i = 1; i < num; ++i) {
        			strcat(instrucao2, ",");
        			strcat(instrucao3, ",");
        			strcat(instrucao2, whyIsReg(pushlist[i],1));
        			strcat(instrucao3, whyIsReg(pushlist[i],2));
    			}
    			sprintf(finalInstrucao,"%s%s",instrucao,instrucao2);
    			fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]{%s}={%s}\n",arq->Reg[29],finalInstrucao,arq->Reg[30],instrucaoValues,instrucao3);
    			//printf("PUSH--------------------------\n");
			    for (int i = 0; i < num; ++i) {
			        arq->Mem[arq->Reg[30] >> 2] = arq->Reg[pushlist[i]];
			      //  printf("0x%08X -> [0x%08X] \n",arq->Reg[30],arq->Mem[arq->Reg[30] >> 2]);
			        arq->Reg[30] = arq->Reg[30] - 4;
			    }
			  //  printf("0x%08X:%-25s\tMEM[0x%08X]{%s}={%s}\n",arq->Reg[29],finalInstrucao,arq->Reg[30],instrucaoValues,instrucao3);
			  //  printf("--------------------------\n");
			}
			//showMemory(arq);
			break;
		case 0b001011:
			uint32_t poplist[] = {ShiftBit(instruction->l,6,5),ShiftBit(instruction->l,0,5),instruction->x,instruction->y,instruction->z};
			num = verifyList(poplist);
			if(num == 0){
				sprintf(instrucao,"pop -");
				fprintf(output,"0x%08X:\t%-25s\t{}=MEM[0x%08X]{}\n",arq->Reg[29],instrucao,arq->Reg[30]);
			} else {
				// printf("POP------------------------------\n");
				char instrucao2[22];
				char instrucao3[22];
				char finalInstrucao[52];
				char instrucaoValues[56] = {""};
				uint32_t old = arq->Reg[30];
				if (num >= 1) {
					arq->Reg[30] = arq->Reg[30] + 4;
        			arq->Reg[poplist[0]] = arq->Mem[arq->Reg[30] >> 2] ;
        			sprintf(instrucaoValues, "%s", whyIsRegHex(arq->Reg[poplist[0]]));
        			arq->Mem[arq->Reg[30] >> 2] = 0;
        			//printf("0x%08X -> [0x%08X] zero\n",arq->Reg[30],arq->Reg[poplist[0]]);
   				}
			    for (int i = 1; i < num; ++i) {
			    	arq->Reg[30] = arq->Reg[30] + 4;
			        if (i >= 1) strcat(instrucaoValues,",");
			        arq->Reg[poplist[i]] = arq->Mem[arq->Reg[30] >> 2] ;
			        strcat(instrucaoValues,whyIsRegHex(arq->Reg[poplist[i]]));
			        arq->Mem[arq->Reg[30] >> 2] = 0;
			       
			        //printf("0x%08X -> [0x%08X]\n",arq->Reg[30],arq->Reg[poplist[i]]);
			    }
				sprintf(instrucao,"pop ");
				if (num >= 1) {
        			sprintf(instrucao2, "%s", whyIsReg(poplist[0],1));
        			sprintf(instrucao3,"%s",whyIsReg(poplist[0],2));
    			}
    			for (int i = 1; i < num; ++i) {
        			strcat(instrucao2, ",");
        			strcat(instrucao3, ",");
        			strcat(instrucao2, whyIsReg(poplist[i],1));
        			strcat(instrucao3, whyIsReg(poplist[i],2));
    			}
    			sprintf(finalInstrucao,"%s%s",instrucao,instrucao2);
			    fprintf(output,"0x%08X:\t%-25s\t{%s}=MEM[0x%08X]{%s}\n",arq->Reg[29],finalInstrucao,instrucao3,old,instrucaoValues);
			   //printf("---------------------------------\n");
			}
			//showMemory(arq);
			break;
		default:
			changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
			printf("[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
			break;
	}
}

void executionInstructionTypeF(ArqResources* arq,typeF* instruction,uint32_t i,FILE* output){
	char instrucao[30];
	switch(instruction->opcode){
		uint32_t IplusSinal; 
		case 0b011010:
			IplusSinal = (ShiftBit(instruction->i,15,1)) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao, "l32 %s,[%s%s%d]", whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal);
			arq->Reg[instruction->z] = arq->Mem[(int32_t)(arq->Reg[instruction->x]) + IplusSinal];
			fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),((int32_t)(arq->Reg[instruction->x]) + IplusSinal) << 2,arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011000:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"l8 %s,[%s%s%d]",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal);
			if(instruction->z % 4 == 0){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),8,8);
			}
			if(instruction->z % 4 == 1){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),0,8);
			}
			if(instruction->z % 4 == 2){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),24,8);
			}
			if(instruction->z % 4 == 3){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),16,8);
			}
			fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->x] + IplusSinal,arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011001:
			IplusSinal = (ShiftBit(instruction->i,15,1)) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao, "l16 %s,[%s%s%d]", whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal);
			if(instruction->z % 4 == 0){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),0,16);
			} else {
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),16,16);
			}
			//arq->Reg[instruction->z] = ((((uint16_t*)(arq->Mem))[(arq->Reg[instruction->x] + IplusSinal )]));//ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),16,16);
			fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%04X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),(arq->Reg[instruction->x] + IplusSinal) << 1,arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011011:
			uint8_t  b1;
			uint8_t  b2;
			uint8_t  b3 ;
			uint8_t  b4 ;
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"s8 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));
			if(instruction->x == 0) arq->Reg[instruction->x] = 0;
		      b1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),0,8);
			  b2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),8,8);
			  b3 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),16,8);
			  b4 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),24,8);
			uint8_t b1_newb = ShiftBit((arq->Reg[instruction->z]),0,8);
			b1 = b1_newb;
			arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2] = (b4 << 24) + (b3 << 16) + (b2 << 8) + b1;
			fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal),whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011100:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"s16 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));
			uint16_t b1_1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),0,16);
			uint16_t  b2_2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),16,16);
			uint16_t b1_1_newb = ShiftBit((arq->Reg[instruction->z]),0,16);
			b1_1 = b1_1_newb;
			printf("%d\n",(arq->Reg[instruction->x] + IplusSinal) >> 2);
			arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1] = (b2_2 << 16) + b1_1;
			fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%04X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 1,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011101:
		//printf("0x%08X\n",arq->Reg[instruction->z]);
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"s32 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));
			arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2] = arq->Reg[instruction->z];
			fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010010:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) + instruction->i : instruction->i;
			sprintf(instrucao,"addi %s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),IplusSinal);
			uint64_t sum = (uint64_t)arq->Reg[instruction->x] + (uint32_t)IplusSinal;
			arq->Reg[instruction->z] = (uint64_t)arq->Reg[instruction->x] + (uint64_t)IplusSinal;
			uint8_t ZN_1 = arq->Reg[instruction->z] == 0;
			uint8_t SN_1 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_1 = (ShiftBit(arq->Reg[instruction->x],31,1) == ShiftBit(instruction->i,15,1)) && (ShiftBit(arq->Reg[instruction->z],31,1) != ShiftBit(instruction->x,31,1));
			uint8_t CY_1 = (sum & (0x1FFFFFFFF) ) >> 32 == 1;
			changeSR(ZN_1,ShiftBit(arq->Reg[31],5,1),SN_1,OV_1,ShiftBit(arq->Reg[31],2,1),CY_1,arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),instruction->i,arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010011:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"subi %s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),IplusSinal);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] - IplusSinal;
			uint8_t ZN_2 = arq->Reg[instruction->z] == 0;
			uint8_t SN_2 = (ShiftBit(arq->Reg[instruction->z],31,1) == 1);
			uint8_t OV_2 = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(instruction->i,15,1)) && ((arq->Reg[instruction->z] & (0b1 << 31)) >> 31)!= ShiftBit(arq->Reg[instruction->x],31,1);
			uint8_t CY_2 = ((arq->Reg[instruction->z] & (0b1 << 32)) >> 32) == 1;
			changeSR(ZN_2,ShiftBit(arq->Reg[31],5,1),SN_2,OV_2,ShiftBit(arq->Reg[31],2,1),CY_2,arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010100:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"muli %s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),IplusSinal);
			arq->Reg[instruction->z] = arq->Reg[instruction->x] * IplusSinal;
			uint8_t ZN_3 = arq->Reg[instruction->z] == 0;
			uint8_t OV_3 = (ShiftBit(arq->Reg[instruction->z],31,1) != 0);
			changeSR(ZN_3,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),OV_3,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s*0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010101:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 15) +instruction->i : instruction->i;
			sprintf(instrucao,"divi %s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),IplusSinal);
			uint8_t ZD_4 = (IplusSinal == 0);
			if(!ZD_4){
				 arq->Reg[instruction->z] = (int32_t)arq->Reg[instruction->x] /(int32_t)IplusSinal;
			}
			uint8_t ZN_4 = arq->Reg[instruction->z] == 0;
			
			uint8_t OV_4 = 0;
			changeSR(ZN_4,ZD_4,ShiftBit(arq->Reg[31],3,1),OV_4,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010110:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"modi %s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),IplusSinal);
			uint8_t ZD_5 = (IplusSinal == 0);
			if(!ZD_5){
				arq->Reg[instruction->z] = (int32_t)arq->Reg[instruction->x] % (int32_t)IplusSinal;
			}
			uint8_t ZN_5 = arq->Reg[instruction->z] == 0;
			
			uint8_t OV_5 = 0;
			changeSR(ZN_5,ZD_5,ShiftBit(arq->Reg[31],4,1),OV_5,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s%%0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b010111:
			sprintf(instrucao,"cmpi %s,%d",whyIsReg(instruction->x,1),instruction->i);
			int64_t sinal = (((instruction->i & (0b1 << 15))>>15)) == 1? 0xFFFF << 15 : 0;
			uint64_t CMPI = (int64_t)arq->Reg[instruction->x] - (int64_t)(sinal+instruction->i);
			uint8_t RegX31 = ShiftBit(arq->Reg[31],31,1);
			uint8_t CMPI31 = ShiftBit(CMPI,31,1);
			uint8_t I15 = ShiftBit(instruction->i,15,1);
			changeSR(CMPI == 0,ShiftBit(arq->Reg[31],5,1),CMPI31 == 1,(RegX31 != CMPI31) && (RegX31 != I15),ShiftBit(arq->Reg[31],2,1),((CMPI & 0x1FFFFFFFF) >> 32)== 1,arq);
			fprintf(output,"0x%08X:\t%-25s\tSR=0x%08x\n",arq->Reg[29],instrucao,arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011111:
			sprintf(instrucao,"ret");
			arq->Reg[30] = arq->Reg[30] + 4;
			uint32_t oldPC = arq->Reg[29]; 
			arq->Reg[29] = arq->Mem[arq->Reg[30] >> 2];
			fprintf(output,"0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n",oldPC,instrucao,arq->Reg[30],arq->Reg[29]);
			break;
		case 0b011110:
			int64_t sinal2 = (((instruction->i & (0b1 << 15))>>15)) == 1? 0xFFFF << 15 : 0;
			int32_t value = instruction->i + sinal2;
			uint32_t oldpc = arq->Reg[29];
			sprintf(instrucao,"call [%s%s%d]",whyIsReg(instruction->x ,1),value >=0?("+"):(""),value);
			arq->Mem[arq->Reg[30] >> 2] = arq->Reg[29] + 4;
			arq->Reg[29] = (arq->Reg[instruction->x] + value) << 2;		
			fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n",oldpc,instrucao,arq->Reg[29],arq->Reg[30],arq->Mem[arq->Reg[30] >> 2]);
			arq->Reg[30] = arq->Reg[30] - 4;
			break;
		default:
			changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
			break;
	}
}

void executionInstructionTypeS(ArqResources* arq,typeS* instruction,uint32_t i,FILE* output){
	char instrucao[30];
	switch(instruction->opcode){
		uint32_t IplusSinal;
		case 0b110111:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao, "bun %d", IplusSinal);
			if(IplusSinal == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4*IplusSinal + 4);
				arq->Reg[29] = arq->Reg[29] + 4*IplusSinal + 4;
			}
			break;
		case 0b111111:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao, "int %d", IplusSinal);
			fprintf(output,"0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08x\n", arq->Reg[29],instrucao,0,0);
			break;
		case 0b101010:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bae %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],0,1) == 0){
				
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			break;
		case 0b101011:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bat %d",IplusSinal);
			if(ShiftBit(arq->Reg[29],6,1) == 0 && ShiftBit(arq->Reg[29],0,1) == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101100:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bbe %d",IplusSinal);
			if(ShiftBit(arq->Reg[29],6,1) == 1 || ShiftBit(arq->Reg[29],0,1) == 1){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101101:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bbt %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],0,1) == 1){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101110:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"beq %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],5,1) == 1){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b101111:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bge %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],4,1) == ShiftBit(arq->Reg[31],3,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110000:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bgt %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],5,1) == 0 && (ShiftBit(arq->Reg[31],4,1) == ShiftBit(arq->Reg[31],3,1))){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110001:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"biv %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],2,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110010:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"ble %d",IplusSinal);
			if(ShiftBit(arq->Reg[29],5,1) == 1 || ShiftBit(arq->Reg[29],4,1) != ShiftBit(arq->Reg[29],3,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110011:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"blt %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],4,1) != ShiftBit(arq->Reg[31],3,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110100:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bne %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],6,1) == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110101:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bni %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],2,1) == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b110110:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bnz %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],5,1) == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b111000:
			IplusSinal = (ShiftBit(instruction->i,25,1) << 6) == 1? (0x3F << 25) +instruction->i : instruction->i;
			sprintf(instrucao,"bzd %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],6,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			
			break;
		case 0b111001:
			int64_t sinal2 = (((instruction->i & (0b1 << 25))>>25)) == 1? 0x3F << 26 : 0;
			int64_t value = (int64_t)instruction->i + sinal2;
			uint32_t oldpc = arq->Reg[29];
			arq->Mem[arq->Reg[30] >> 2] = arq->Reg[29] + 4;
			arq->Reg[29] = arq->Reg[29] + 4 + (value << 2);
			sprintf(instrucao,"call %d",value);
			fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n",oldpc,instrucao,arq->Reg[29],arq->Reg[30],arq->Mem[arq->Reg[30] >> 2]);
			arq->Reg[30] = arq->Reg[30] - 4;
			break;
		default:
			changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
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
	for(int i = 0; i < 170; i = i + 1) {
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
	fprintf(output,"[START OF SIMULATION]\n");
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
			executionInstructionTypeU(newArq,newIU,i,output);

			printf("-------------> 0x%08X U, op = 0x%08X\n",newArq->Mem[87],opcode);
	
			if(ShiftBit(newArq->Reg[31],2,1)) break;
			//if(opcode == 0b001010)
				//printf("(push -> 0x%08X , proximo -> 0x%08X, instrução -> 0x%08X)\n", newArq->Reg[29],newArq->Reg[29]+4,newArq->Reg[28]);
			newArq->Reg[29] = newArq->Reg[29] + 4;
		}
		else if(optype == 'F'){
			newIF->opcode = opcode;
			newIF->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
			newIF->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
			newIF->i = (newArq->Reg[28] & (0xFFFF));
			executionInstructionTypeF(newArq,newIF,i,output);
			printf("-------------> 0x%08X F, op = 0x%08X\n",newArq->Mem[87],opcode);
			if(ShiftBit(newArq->Reg[31],2,1)) break;

		}
		else if(optype == 'S'){
			newIS->opcode = opcode;
			newIS->i = (newArq->Reg[28] & (0b11111111111111111111111111));
			executionInstructionTypeS(newArq,newIS,i,output);
			printf("-------------> 0x%08X S , op = 0x%08X\n",newArq->Mem[87],opcode);
			if(ShiftBit(newArq->Reg[31],2,1)) break;
			if(opcode == 0b111111 && newIS->i == 0){
				exec = 0;
				break;
			}
		}else if(optype != 'U' && optype != 'F' && optype != 'S')	{
			fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",newArq->Reg[29]);
			break;
		}

		i++;
	}
	fprintf(output,"[END OF SIMULATION]");
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
