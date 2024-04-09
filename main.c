#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct FPU{
	float  x;
	uint32_t x_int;
	float  y;
	uint32_t y_int;
	uint32_t y_intAnt;
	uint32_t  z;
	uint32_t z_int;
	uint32_t OPST;
 	uint32_t cicle;
	uint32_t OPSTnew;
	uint32_t OPSTlast;
	uint8_t error;
}FPU;

typedef struct Terminal{
	int32_t term;
	uint32_t index;
	uint32_t size;
	int32_t* buffer;
}Terminal;

typedef struct Watchdog{
	int32_t setWatchdog;
	int32_t watchCount;
	int8_t isActivate;
}Watchdog;

typedef struct ArqResources{
	uint32_t* Mem;
	uint32_t Reg[32];
	struct Watchdog* wdog;
	struct Terminal* term;
	struct FPU* fpu;
	uint32_t lastR27;
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


void changerValue(uint32_t* value,uint8_t pos,uint8_t v){
	*value = (*value & ~((1 << pos))) | (v << pos);
}

uint32_t decimal_para_ieee754(float num) {
    uint32_t *ponteiro = (int32_t*)&num;
    uint32_t bits = *ponteiro;
    
    // Extrair o bit de sinal
    uint32_t sinal = (bits >> 31) & 1;
    
    // Extrair o expoente
    uint32_t expoente = (bits >> 23) & 0xFF;
    
    // Extrair a mantissa
    uint32_t mantissa = bits & 0x7FFFFF; // Usar máscara corrigida
	//printf("expoente = %d\n",expoente);
    uint32_t result = bits;
	//printf("result = 0x%08X\n",bits);
    return bits;
}

ArqResources* inicialization(){
	ArqResources* newArq = (ArqResources*)malloc(sizeof(ArqResources));
	Watchdog* newW = (Watchdog*)malloc(sizeof(Watchdog));
	Terminal* newT = (Terminal*)malloc(sizeof(Terminal));
	FPU* newF = (FPU*)malloc(sizeof(FPU));

  newF->x = 0;
  newF->x_int = 0;
  newF->y = 0;
  newF->z = 0;
  newF->y_int = 0;
  newF->OPST = 0;
  newF->cicle = 0;

	newT->term = 0;
	newT->index = 0;
	newT->size = 100;
	newT->buffer = (int32_t*)malloc(sizeof(int32_t)*100);

	newW->watchCount = 0;
	newW->setWatchdog = 0;
	newW->isActivate = 0;
	
	newArq->term = newT;
	newArq->fpu = newF;
	newArq->wdog = newW;
	newArq->Mem = (uint32_t*)calloc(32,10024);
	return newArq;
}

char* whyIsReg(uint8_t pos,int8_t op){
	char* res = (char*)malloc(sizeof(4));
	if(pos >= 26 && pos <= 31){
		switch(pos){
			case 26:
				return op == 1? "cr" : "CR";
				break;
			case 27:
				return op == 1? "ipc": "IPC" ;
				break;
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


void showMemory(ArqResources* arq,int ini,int end) {
    printf("--------------------------------------------\n");
    printf("|                 Memória                  |\n");
    printf("--------------------------------------------\n");
    for (int i = ini; i < end; i = i + 1) {
        // Impressao lado a lado
        printf("0x%08X: 0x%08X (0x%02X 0x%02X 0x%02X 0x%02X)\n", i << 2, arq->Mem[i], ((uint8_t*)(arq->Mem))[(i << 2) + 3], ((uint8_t*)(arq->Mem))[(i << 2) + 2], ((uint8_t*)(arq->Mem))[(i << 2) + 1], ((uint8_t*)(arq->Mem))[(i << 2) + 0]);
    }
    printf("--------------------------------------------\n");
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
	uint32_t a_ZN,a_ZD,a_SN,a_OV,a_IV,a_CY,molde,a_IE;
	a_ZN = ShiftBit(arq->Reg[31],6,1);
	a_ZD = ShiftBit(arq->Reg[31],5,1);
	a_SN = ShiftBit(arq->Reg[31],4,1);
	a_OV = ShiftBit(arq->Reg[31],3,1);
	a_IV = ShiftBit(arq->Reg[31],2,1);
	a_IE = ShiftBit(arq->Reg[31],1,1);
	a_CY = ShiftBit(arq->Reg[31],0,1);

	if(a_ZN != ZN) a_ZN = ZN;
	if(a_ZD != ZD) a_ZD = ZD;
	if(a_SN != SN) a_SN = SN;
	if(a_OV != OV) a_OV = OV;
	if(a_IV != IV) a_IV = IV;
	if(a_CY != CY) a_CY = CY;

	molde = (a_ZN << 6) + (a_ZD << 5) + (a_SN << 4) + (a_OV << 3) + (a_IV << 2) + (a_IE << 1) + (a_CY);
	arq->Reg[31] = molde;
}

char opcodeType(uint8_t op){
	if(op <= 0b001011)
		return 'U';
	else if(op >= 0b010010 && op <= 0b011111 ||op == 0b100000 || op == 0b100001)
		return 'F';
	else if((op >= 0b101010 && op <= 0b111001) || op == 0b111111)
		return 'S';
	else
		return 'E';
}

int calcular_ciclos(uint32_t valor_hex_x,uint32_t valor_hex_y) {
	printf("para calcular expoente x = 0x%08X\n",valor_hex_x);
	printf("para calcular expoente y = 0x%08X\n",valor_hex_y);
    // Extrair os expoentes
    uint32_t expoente_x = ((*(uint32_t*)&valor_hex_x) >> 23) & 0xFF;
    uint32_t expoente_y = ((*(uint32_t*)&valor_hex_y) >> 23) & 0xFF;
    if(expoente_x != 0) expoente_x = expoente_x - 127;
    if(expoente_y != 0) expoente_y = expoente_y - 127;

	printf(" expoente x = 0x%08X\n",expoente_x);
	printf(" expoente y = 0x%08X\n",expoente_y);
    // Calcular a operação desejada
    int resultado = abs(expoente_x - expoente_y) + 1;
	printf("resultado = %d\n",resultado);
   printf("ciclos = %d\n",resultado);
    return resultado;
}

uint32_t convertToIEEE754(float  value) {
    uint32_t *ptr = (int32_t*)&value;
    uint32_t binaryValue = *ptr;
    return binaryValue;
}

float convertFromIEEE754(uint32_t binaryValue) {
    uint32_t *ptr = &binaryValue;
    float *float_ptr = (float*)ptr;
    return *float_ptr;
}

void operationFPU(ArqResources* arq,typeF* instruction){
	arq->fpu->OPST = 0xFF & arq->Reg[instruction->z]; 
	arq->fpu->OPSTnew = 0xFF & arq->Reg[instruction->z]; 
	uint8_t error = 0;
	if(ShiftBit(arq->fpu->OPST,0,5) == 0b00001){
		arq->fpu->z = (uint32_t)(arq->fpu->x + arq->fpu->y);
		arq->fpu->cicle = calcular_ciclos(convertToIEEE754(arq->fpu->x_int),convertToIEEE754(arq->fpu->y_int));		
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00010){
		arq->fpu->z = (uint32_t)(arq->fpu->x - arq->fpu->y);
		arq->fpu->cicle = calcular_ciclos(convertToIEEE754(arq->fpu->x_int),convertToIEEE754(arq->fpu->y_int));		
		//printf("0x%08X: ciclos = %d\n",arq->Reg[29],arq->fpu->cicle);
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00011){
		arq->fpu->z = (uint32_t)(arq->fpu->x * arq->fpu->y);
		arq->fpu->cicle = calcular_ciclos(convertToIEEE754(arq->fpu->x_int),convertToIEEE754(arq->fpu->y_int));		
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00100){
		//printf("passou aqui\n");
		if(arq->fpu->y != 0){
			float res = (float)convertFromIEEE754(arq->fpu->x_int) / (float)convertFromIEEE754(arq->fpu->y_int); 
			arq->fpu->z = decimal_para_ieee754(res);
			arq->fpu->cicle = calcular_ciclos(convertToIEEE754(arq->fpu->x_int),convertToIEEE754(arq->fpu->y_int));		;
		} else {
			arq->fpu->cicle = ShiftBit(arq->fpu->x_int,24,8) +  ShiftBit(arq->fpu->y_intAnt,24,8) + 1;
			error = 1;
			arq->fpu->error = 1;
		}
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00101){
		arq->fpu->x = arq->fpu->z;
		arq->fpu->x_int = convertToIEEE754(arq->fpu->z);
		arq->fpu->cicle = 1;
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00110){
		
		arq->fpu->y = arq->fpu->z;
		arq->fpu->y_intAnt = arq->fpu->y_int;
		arq->fpu->y_int = convertToIEEE754(arq->fpu->z);
		arq->fpu->cicle = 1;
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00111){
		float temp = convertFromIEEE754(arq->fpu->z);
		arq->fpu->z_int = ceil(temp);
		arq->fpu->cicle = 1;
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b01000){
		float temp = convertFromIEEE754(arq->fpu->z);
		arq->fpu->z_int = floor(temp);
		arq->fpu->cicle = 1;
	}
	else if(ShiftBit(arq->fpu->OPST,0,5) == 0b01001){
		float temp = convertFromIEEE754(arq->fpu->z);
		arq->fpu->z_int = round(temp);
		arq->fpu->cicle = 1;
	}
    else if(ShiftBit(arq->fpu->OPST,0,5) > 0b01001 || ShiftBit(arq->fpu->OPST,0,5) < 0b00001){
    	arq->fpu->cicle = 1;
		error = 1;
		arq->fpu->error = 1;
		//printf("passou aqui\n");
    }
    //fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal),whyIsReg(instruction->z,2),arq->fpu->OPST);
	if(error){arq->fpu->OPSTlast = arq->fpu->OPST;arq->fpu->OPST = arq->fpu->OPST | 0b100000;}
	arq->lastR27 =  arq->Reg[27];
	//printf("R27 = 0x%08X\n",arq->lastR27);
	arq->Reg[27] = arq->Reg[29] + 4;
	//arq->Reg[29] = arq->Reg[29] + 4;
}

void executionInstructionTypeU(ArqResources* arq,typeU* instruction,FILE* output){
	switch(instruction->opcode){
		char instrucao[30];
		long long unsigned int cyv;
		uint8_t n_ZD;
		uint8_t n_ZN;
		uint8_t n_SN;
		uint8_t n_OV;
		uint8_t n_CY;
		int8_t num;
		case 0b000000:
			uint32_t xyl = arq->Reg[28] & 0x1FFFFF;
			sprintf(instrucao, "mov %s,%d", whyIsReg(instruction->z,1), xyl);
			arq->Reg[instruction->z] = xyl;
			fprintf(output,"0x%08X:\t%-25s\t%s=0x%08X\n", arq->Reg[29],instrucao, whyIsReg(instruction->z,2), arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b000001:
			uint32_t x4 = ShiftBit(arq->Reg[28] & 0x1FFFFF,20,1);
			int32_t xyl_1 = ((int32_t)(arq->Reg[28] & 0x1FFFFF));
			int32_t x4xyl = x4 == 1 ? ((0x7FFU) << 21) + xyl_1 : (arq->Reg[28] & 0x1FFFFF);
			sprintf(instrucao, "movs %s,%d", whyIsReg(instruction->z,1), x4xyl);
			arq->Reg[instruction->z] = x4xyl;
			fprintf(output,"0x%08X:\t%-25s\t%s=0x%08X\n", arq->Reg[29],instrucao, whyIsReg(instruction->z,2), arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b000010:
      //printf("passou\n");
			if(instruction->x == 0) arq->Reg[0] = 0;
			if(instruction->y == 0) arq->Reg[0] = 0;
			cyv = ((int64_t)(arq->Reg[instruction->x]) + (int64_t)(arq->Reg[instruction->y]));
			arq->Reg[instruction->z] = (uint64_t)arq->Reg[instruction->x] + (uint64_t)arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],32,1) == ShiftBit(arq->Reg[instruction->y],32,1)) && ShiftBit(arq->Reg[instruction->z],32,1) != ShiftBit(arq->Reg[instruction->x],32,1);
			n_CY = ShiftBit64(cyv,33,1) == 1;
			sprintf(instrucao,"add %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			fprintf(output,"0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),cyv,arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
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

			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b000100:
			uint32_t id_op = ShiftBit(instruction->l,8,3);
			if(id_op == 0b000){
				cyv = (int64_t)(arq->Reg[instruction->x])*(int64_t)(arq->Reg[instruction->y]);
				uint32_t L41 = (cyv >> 32);
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
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			else if(id_op == 0b001){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int64_t)(arq->Reg[instruction->y])) << (ShiftBit(instruction->l,0,5) +1);
				arq->Reg[instruction->z] = cyv >> 32;
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_CY = arq->Reg[instruction->z]  != 0;
				n_ZN = cyv == 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				sprintf(instrucao,"sll %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s<<%d=0x%016llX,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5) +1,cyv,arq->Reg[31]);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			else if(id_op == 0b010){
				long long unsigned int cyv2 = (int64_t)(arq->Reg[instruction->x])*(int32_t)(arq->Reg[instruction->y]);
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
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			else if(id_op == 0b011){
				cyv = (((int64_t)(arq->Reg[instruction->z])<<32) + (int32_t)(arq->Reg[instruction->y])) << (ShiftBit(instruction->l,0,5) +1);
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				arq->Reg[instruction->z] = ShiftBit64(cyv,32,32);
				if(instruction->x == 0) arq->Reg[instruction->x] = 0;
				if(instruction->z == 0) arq->Reg[instruction->z] = 0;
				n_ZN = cyv == 0;
				n_OV = arq->Reg[instruction->z] != 0;
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				sprintf(instrucao,"sla %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s<<%d=0x%016llX,SR=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5) +1,(long long unsigned int)((arq->Reg[instruction->z] << 31) + (int32_t)arq->Reg[instruction->x]) ,arq->Reg[31]);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			else if(id_op == 0b100){
				if(instruction->y == 0) arq->Reg[0] = 0;
				uint32_t oldl4_0 = arq->Reg[ShiftBit(instruction->l,0,5)];
				sprintf(instrucao,"div %s,%s,%s,%s",whyIsReg(ShiftBit(instruction->l,0,5),1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				if(arq->Reg[instruction->y] != 0 ){
					arq->Reg[ShiftBit(instruction->l,0,5)] = arq->Reg[instruction->x] % arq->Reg[instruction->y];
					arq->Reg[instruction->z] = arq->Reg[instruction->x] / arq->Reg[instruction->y];
					oldl4_0 = arq->Reg[ShiftBit(instruction->l,0,5)];
					changeSR(arq->Reg[instruction->z] == 0,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),oldl4_0 != 0,arq);
					fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),oldl4_0,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
					arq->Reg[29] = arq->Reg[29] + 4;
				}else {
					if(ShiftBit(arq->Reg[31],1,1) == 0b0){
						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
						fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),oldl4_0,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
						//fprintf(output,"[SOFTWARE INTERRUPTION]\n");
						arq->Reg[29] = arq->Reg[29] + 4;
					} else if(ShiftBit(arq->Reg[31],1,1) == 0b1){
						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
						arq->Reg[26] = 0;
						
						fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),oldl4_0,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
						fprintf(output,"[SOFTWARE INTERRUPTION]\n");
						arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[26];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[27];
						arq->Reg[27] = arq->Reg[29];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Reg[29] = 0x00000008;
					}

				}


			}else if(id_op == 0b101){
				cyv = (((long long unsigned int)(arq->Reg[instruction->z]) << 32) + (long long unsigned int)(arq->Reg[instruction->y])) >> ( ShiftBit(instruction->l,0,5) + 1);
				arq->Reg[instruction->z] = ShiftBit(cyv,31,32);
				arq->Reg[instruction->x] = ShiftBit(cyv,0,32);
				n_CY = arq->Reg[instruction->z] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"srl %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),n_CY,arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s>>%d=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5)+1,cyv,arq->Reg[31]);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			else if(id_op == 0b110){
				if(instruction->y == 0) arq->Reg[0] = 0;
				sprintf(instrucao,"divs %s,%s,%s,%s",whyIsReg(ShiftBit(instruction->l,0,5),1),whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
				if(arq->Reg[instruction->y] != 0){
					arq->Reg[ShiftBit(instruction->l,0,5)] = (int32_t)arq->Reg[instruction->x] % (int32_t)arq->Reg[instruction->y];
					arq->Reg[instruction->z] =  (int32_t)arq->Reg[instruction->x] /  (int32_t)arq->Reg[instruction->y];
					n_ZN = arq->Reg[instruction->z] == 0;
					n_ZD = arq->Reg[instruction->y] == 0;
					n_OV = arq->Reg[ShiftBit(instruction->l,0,5)] != 0;
					changeSR(n_ZN,n_ZD,ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
					fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[ShiftBit(instruction->l,0,5)],whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
					arq->Reg[29] = arq->Reg[29] + 4;
				} else {
					if(ShiftBit(arq->Reg[31],1,1) == 0b0){
						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
					} else if(ShiftBit(arq->Reg[31],1,1) == 0b1){

						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
						arq->Reg[26] = 0;
						fprintf(output,"0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(ShiftBit(instruction->l,0,5),2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[ShiftBit(instruction->l,0,5)],whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
						fprintf(output,"[SOFTWARE INTERRUPTION]\n");
						arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[26];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[27];
						arq->Reg[27] = arq->Reg[29];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Reg[29] = 0x00000008;
					}
				}
				
			}
			else if(id_op == 0b111){
				if(instruction->z == 0) arq->Reg[0] = 0;
				if(instruction->y == 0) arq->Reg[0] = 0;
				if(instruction->z == 0){
					cyv = (int64_t)(arq->Reg[instruction->y]) >> (ShiftBit(instruction->l,0,5) + 1);
				} else {
					//printf("p1\n");
					cyv = ((int64_t)(arq->Reg[instruction->z] << 32) + (int32_t)(arq->Reg[instruction->y])) >> (ShiftBit(instruction->l,0,5) + 1);
				}

				arq->Reg[instruction->z] = cyv >> 32;
				arq->Reg[instruction->x] = ShiftBit64(cyv,0,32);
				if(instruction->z == 0) arq->Reg[0] = 0;
				if(instruction->x == 0) arq->Reg[0] = 0;
				n_OV = arq->Reg[instruction->z] != 0;
				n_ZN = cyv == 0;
				sprintf(instrucao,"sra %s,%s,%s,%d",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1),ShiftBit(instruction->l,0,5));
				changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),n_OV,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"0x%08X:\t%-25s\t%s:%s=%s:%s>>%d=0x%016llX,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->z,2),whyIsReg(instruction->y,2),ShiftBit(instruction->l,0,5)+1,cyv,arq->Reg[31]);
				arq->Reg[29] = arq->Reg[29] + 4;
			} else {
				changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
				fprintf(output,"[SOFTWARE INTERRUPTION]\n");
				changerValue(&arq->Reg[31],2,1);
				//arq->Reg[27] = arq->Reg[29];
				arq->Reg[26] = (arq->Reg[28] & (0x7F << 25));
				arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Mem[arq->Reg[30]] = arq->Reg[26];
				arq->Reg[26] = arq->Reg[29];
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Mem[arq->Reg[30]] = arq->Reg[27];
				arq->Reg[27] = arq->Reg[29];
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Reg[29] = 0x00000008;
			}
			break;
		case 0b000101:
			if(instruction->x == 0 ) arq->Reg[instruction->x] = 0;
			if(instruction->y == 0 ) arq->Reg[instruction->y] = 0;
			uint64_t cyv2 = (int32_t)(arq->Reg[instruction->x]) - (int32_t)(arq->Reg[instruction->y]);
			n_ZN = cyv2 == 0;
			n_SN = ShiftBit64(cyv2,31,1) == 1;
			n_OV = (ShiftBit(arq->Reg[instruction->x],31,1) != ShiftBit(arq->Reg[instruction->y],31,1)) && (ShiftBit64(cyv2,31,1) != ShiftBit(arq->Reg[instruction->x],31,1));
			n_CY = (cyv2 & 0x1FFFFFFFF) >> 32 == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,n_OV,ShiftBit(arq->Reg[31],2,1),n_CY,arq);
			sprintf(instrucao,"cmp %s,%s",whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\tSR=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b000110:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] & arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"and %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b000111:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] | arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"or %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b001000:
			arq->Reg[instruction->z] = ~arq->Reg[instruction->x];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"not %s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b001001:
			arq->Reg[instruction->z] = arq->Reg[instruction->x] ^ arq->Reg[instruction->y];
			n_ZN = arq->Reg[instruction->z] == 0;
			n_SN = ShiftBit(arq->Reg[instruction->z],31,1) == 1;
			changeSR(n_ZN,ShiftBit(arq->Reg[31],5,1),n_SN,ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
			sprintf(instrucao,"xor %s,%s,%s",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),whyIsReg(instruction->y,1));
			fprintf(output,"0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),whyIsReg(instruction->y,2),arq->Reg[instruction->z],arq->Reg[31]);
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b001010:
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
			    for (int i = 0; i < num; ++i) {
			        arq->Mem[arq->Reg[30] >> 2] = arq->Reg[pushlist[i]];
					arq->Reg[30] = arq->Reg[30] - 4;
			    }
				
				//if(arq->Reg[29] == 0x000000C4) printf("Reg30 = 0x%08X\n",arq->Reg[30]);
			}
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		case 0b001011:
			uint32_t poplist[] = {ShiftBit(instruction->l,6,5),ShiftBit(instruction->l,0,5),instruction->x,instruction->y,instruction->z};
			num = verifyList(poplist);
			if(num == 0){
				sprintf(instrucao,"pop -");
				fprintf(output,"0x%08X:\t%-25s\t{}=MEM[0x%08X]{}\n",arq->Reg[29],instrucao,arq->Reg[30]);
			} else {
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
   				}
			    for (int i = 1; i < num; ++i) {
			    	arq->Reg[30] = arq->Reg[30] + 4;
			        if (i >= 1) strcat(instrucaoValues,",");
			        arq->Reg[poplist[i]] = arq->Mem[arq->Reg[30] >> 2] ;
			        strcat(instrucaoValues,whyIsRegHex(arq->Reg[poplist[i]]));
			        arq->Mem[arq->Reg[30] >> 2] = 0;

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
			}
			arq->Reg[29] = arq->Reg[29] + 4;
			break;
		default:
			changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
			fprintf(output,"[SOFTWARE INTERRUPTION]\n");
			//arq->Reg[27] = arq->Reg[29];
			arq->Reg[26] = (arq->Reg[28] & (0x7F << 25));
			arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
			arq->Reg[30] = arq->Reg[30] - 4;
			
			arq->Mem[arq->Reg[30]] = arq->Reg[26];
			arq->Reg[26] = arq->Reg[29];
			arq->Reg[30] = arq->Reg[30] - 4;
	
			arq->Mem[arq->Reg[30]] = arq->Reg[27];
			arq->Reg[27] = arq->Reg[29];
			arq->Reg[30] = arq->Reg[30] - 4;
			arq->Reg[29] = 0x00000004;
			break;
	}
}

void executionInstructionTypeF(ArqResources* arq,typeF* instruction,FILE* output){
	char instrucao[30];
	switch(instruction->opcode){
		uint32_t IplusSinal;
		

		case 0b011010:
			IplusSinal = (ShiftBit(instruction->i,15,1)) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao, "l32 %s,[%s%s%d]", whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal);
      if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808880){
        arq->Reg[instruction->z] = arq->fpu->x_int;
        fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x80808880]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->z] );
			  arq->Reg[29] = arq->Reg[29] + 4; 
      } else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808884){
        arq->Reg[instruction->z] = arq->fpu->y_int;
        fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x80808884]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			  arq->Reg[29] = arq->Reg[29] + 4; 
      } else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808888){
        arq->Reg[instruction->z] = (arq->fpu->z < 9999 && ShiftBit(arq->fpu->OPSTnew,0,5) < 7 || ShiftBit(arq->fpu->OPSTnew,0,5) == 4)?((ShiftBit(arq->fpu->OPSTnew,0,5) == 4)? arq->fpu->z:(convertToIEEE754((float)arq->fpu->z)) ):( (ShiftBit(arq->fpu->OPSTnew,0,6) == 0x20 && arq->fpu->OPSTlast < 0b1001)? arq->fpu->z:arq->fpu->z_int);
        fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x80808888]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			  arq->Reg[29] = arq->Reg[29] + 4; 
      } else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x8888888B){
         arq->Reg[instruction->z] = arq->term->term;
        fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x8888888B]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			  arq->Reg[29] = arq->Reg[29] + 4; 
      } else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x8080888C){
        arq->Reg[instruction->z] = arq->fpu->OPSTnew == 0x20? arq->fpu->OPSTnew:arq->fpu->OPST & 0b011111;
        fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x8080888C]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
        arq->Reg[29] = arq->Reg[29] + 4;
      }else {
			  arq->Reg[instruction->z] = arq->Mem[(int32_t)(arq->Reg[instruction->x]) + IplusSinal];
			  fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%08X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),((int32_t)(arq->Reg[instruction->x]) + IplusSinal) << 2,arq->Reg[instruction->z]);
			  arq->Reg[29] = arq->Reg[29]  + 4;
      }
			break;
		case 0b011000:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"l8 %s,[%s%s%d]",whyIsReg(instruction->z,1),whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal);
			if((arq->Reg[instruction->x] + IplusSinal) % 4 == 0){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),24,8);
				fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->x] + IplusSinal,arq->Reg[instruction->z]);
			}
			if((arq->Reg[instruction->x] + IplusSinal) % 4 == 1){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),16,8);
				fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->x] + IplusSinal,arq->Reg[instruction->z]);
			}
			if((arq->Reg[instruction->x] + IplusSinal) % 4 == 2){
				arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),8,8);
				fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->x] + IplusSinal,arq->Reg[instruction->z]);
			}
			if((arq->Reg[instruction->x] + IplusSinal) % 4 == 3){
		        if(((arq->Reg[instruction->x] + IplusSinal)) == 0x8080888F){
					
		          arq->Reg[instruction->z] = arq->fpu->OPSTnew == 0x00000020?arq->fpu->OPSTnew:ShiftBit(arq->fpu->OPST,0,8);
				  //printf("arq----------------------------------------> 0x%08X\n",arq->Reg[instruction->z]);
		          fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),0x8080888F,arq->Reg[instruction->z]);
		        } else {
				  arq->Reg[instruction->z] = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),0,8);
				  fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),arq->Reg[instruction->x] + IplusSinal,arq->Reg[instruction->z]);
		        }
			}
			
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
			fprintf(output,"0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%04X\n", arq->Reg[29],instrucao,whyIsReg(instruction->z,2),(arq->Reg[instruction->x] + IplusSinal) << 1,arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011011:
			uint8_t  b1;
			uint8_t  b2;
			uint8_t  b3 ;
			uint8_t  b4 ;
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			if(instruction->x == 0) arq->Reg[instruction->x] = 0;
			sprintf(instrucao,"s8 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));
		  if(arq->Reg[instruction->x] + IplusSinal == 0x8888888B){
				b1 = ShiftBit(arq->term->term,0,8);
			 	b2 = ShiftBit(arq->term->term,8,8);
			 	b3 = ShiftBit(arq->term->term,16,8);
			 	b4 = ShiftBit(arq->term->term,24,8);
				uint8_t b1_newb = ShiftBit((arq->Reg[instruction->z]),0,8);
				b1 = b1_newb;
				arq->term->term = (b4 << 24) + (b3 << 16) + (b2 << 8) + b1;
				arq->term->buffer[(arq->term->index)] = b1;
				arq->term->index++;
				if(arq->term->index == 100){
					arq->term->buffer = realloc(arq->term->buffer,sizeof(int32_t)*arq->term->size + sizeof(int32_t)*100);
					arq->term->size = arq->term->size +100;
				}
				fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal),whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
				arq->Reg[29] = arq->Reg[29]  + 4;
			} else if(arq->Reg[instruction->x] + IplusSinal == 0x8080888F){
				arq->fpu->OPST = 0xFF & arq->Reg[instruction->z]; 
				arq->fpu->OPSTnew = 0xFF & arq->Reg[instruction->z]; 
				uint8_t error = 0;
				if(ShiftBit(arq->fpu->OPST,0,5) == 0b00001){
					arq->fpu->z = (uint32_t)(arq->fpu->x + arq->fpu->y);
					arq->fpu->cicle = calcular_ciclos((uint32_t)arq->fpu->x,(uint32_t)arq->fpu->y);		
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00010){
					arq->fpu->z = (uint32_t)(arq->fpu->x - arq->fpu->y);
					arq->fpu->cicle = calcular_ciclos(arq->fpu->x_int,arq->fpu->y_int);
					//printf("0x%08X: ciclos = %d\n",arq->Reg[29],arq->fpu->cicle);
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00011){
					arq->fpu->z = (uint32_t)(arq->fpu->x * arq->fpu->y);
					arq->fpu->cicle = calcular_ciclos(arq->fpu->x_int,convertToIEEE754(arq->fpu->y_int));
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00100){
					if(arq->fpu->y != 0){
						float res = (float)convertFromIEEE754(arq->fpu->x_int) / (float)convertFromIEEE754(arq->fpu->y_int); 
						arq->fpu->z = decimal_para_ieee754(res);
						arq->fpu->cicle = calcular_ciclos(arq->fpu->x_int,arq->fpu->y_int);
					} else {
						arq->fpu->cicle = ShiftBit(arq->fpu->x_int,24,8) +  ShiftBit(arq->fpu->y_intAnt,24,8);
						error = 1;
					}
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00101){
					arq->fpu->x = arq->fpu->z;
					arq->fpu->x_int = convertToIEEE754(arq->fpu->z);
					arq->fpu->cicle = 1;
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00110){
					
					arq->fpu->y = arq->fpu->z;
					arq->fpu->y_intAnt = arq->fpu->y_int;
					arq->fpu->y_int = convertToIEEE754(arq->fpu->z);
					arq->fpu->cicle = 1;
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b00111){
					float temp = convertFromIEEE754(arq->fpu->z);
					arq->fpu->z_int = ceil(temp);
					arq->fpu->cicle = 1;
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b01000){
					float temp = convertFromIEEE754(arq->fpu->z);
					arq->fpu->z_int = floor(temp);
					arq->fpu->cicle = 1;
				}
				else if(ShiftBit(arq->fpu->OPST,0,5) == 0b01001){
					float temp = convertFromIEEE754(arq->fpu->z);
					arq->fpu->z_int = round(temp);
					arq->fpu->cicle = 1;
				}
        		else if(ShiftBit(arq->fpu->OPST,0,5) > 0b01001 || ShiftBit(arq->fpu->OPST,0,5) < 0b00001){
           			arq->fpu->cicle = 1;
					error = 1;
					//printf("passou aqui\n");
        		}
        		fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal),whyIsReg(instruction->z,2),arq->fpu->OPST);
				if(error){arq->fpu->OPSTlast = arq->fpu->OPST; arq->fpu->OPST = 0x00000020;arq->fpu->OPSTnew = 0x00000020;}
				arq->lastR27 =  arq->Reg[27];
				//printf("R27 = 0x%08X\n",arq->lastR27);
				arq->Reg[27] = arq->Reg[29] + 4;
				arq->Reg[29] = arq->Reg[29] + 4;
			
				//printf("0x%08X\n",arq->Reg[29]);
      		} else {
				b1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),0,8);
			 	b2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),8,8);
			 	b3 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),16,8);
			 	b4 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]),24,8);
				uint8_t  b1_newb;
        		if((arq->Reg[instruction->x] + IplusSinal) % 4 == 0){
					 b1_newb = ShiftBit((arq->Reg[instruction->z]),24,8);
					 b4 = b1_newb;
				}
				if((arq->Reg[instruction->x] + IplusSinal) % 4 == 1){
					 b1_newb = ShiftBit((arq->Reg[instruction->z]),16,8);
					 b3 = b1_newb;
				}
				if((arq->Reg[instruction->x] + IplusSinal) % 4 == 2){
					if((arq->Reg[instruction->x] + IplusSinal) == 0x27A){
						b1_newb = ShiftBit((arq->Reg[instruction->z]),0,8);
					} else {
						b1_newb = ShiftBit((arq->Reg[instruction->z]),8,8);
					}
					
					 b2 = b1_newb;
				}
				if((arq->Reg[instruction->x] + IplusSinal) % 4 == 3){
					b1_newb = ShiftBit((arq->Reg[instruction->z]),0,8);
					 b1 = b1_newb;
				}
				
				arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2] = (b4 << 24) + (b3 << 16) + (b2 << 8) + b1;
				fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal),whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
				arq->Reg[29] = arq->Reg[29]  + 4;
			}
			break;
		case 0b011100:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"s16 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));
			uint16_t b1_1 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),0,16);
			uint16_t  b2_2 = ShiftBit((arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1]),16,16);
			uint16_t b1_1_newb = ShiftBit((arq->Reg[instruction->z]),0,16);
			b1_1 = b1_1_newb;
			arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 1] = (b2_2 << 16) + b1_1;
			fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%04X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 1,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011101:
			IplusSinal = (ShiftBit(instruction->i,15,1) << 16) == 1? (0xFFFF << 16) +instruction->i : instruction->i;
			sprintf(instrucao,"s32 [%s%s%d],%s",whyIsReg(instruction->x,1),(IplusSinal >= 0) ? ("+") : (""),IplusSinal,whyIsReg(instruction->z,1));

			if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808080){
				arq->wdog->watchCount = ShiftBit(arq->Reg[instruction->z],0,30);
				arq->wdog->setWatchdog =  ShiftBit(arq->Reg[instruction->z],31,1);
				fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
				arq->Reg[29] = arq->Reg[29]  + 4;
			}else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808880){
		        arq->fpu->x = (float)arq->Reg[instruction->z];
				arq->fpu->x_int = arq->Reg[instruction->z];
		        fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
		        arq->Reg[29] = arq->Reg[29] + 4;
		      }else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808884){
		        arq->fpu->y = (float)arq->Reg[instruction->z];
				arq->fpu->y_intAnt = arq->fpu->y_int;
				arq->fpu->y_int = arq->Reg[instruction->z];
		        fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
		        arq->Reg[29] = arq->Reg[29] + 4;
		      }else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x80808888){
		        arq->fpu->z = arq->Reg[instruction->z];
		        fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
		        arq->Reg[29] = arq->Reg[29] + 4; 
		      }else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x8888888B){
		        //ToDo     
		      }else if(((arq->Reg[instruction->x] + IplusSinal) << 2) == 0x8080888C){
		        arq->fpu->OPST = (float)arq->Reg[instruction->z];
				arq->fpu->OPSTnew = (float)arq->Reg[instruction->z];
		        fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Reg[instruction->z]);
				operationFPU(arq,instruction);
		        arq->Reg[29] = arq->Reg[29] + 4;
		      } else {
				arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2] = arq->Reg[instruction->z];
				fprintf(output,"0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", arq->Reg[29],instrucao,(arq->Reg[instruction->x] + IplusSinal) << 2,whyIsReg(instruction->z,2),arq->Mem[(arq->Reg[instruction->x] + IplusSinal) >> 2]);
				arq->Reg[29] = arq->Reg[29]  + 4;
			}

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
			uint8_t CY_2 = (((long long unsigned int)(arq->Reg[instruction->z]) & (0b1 << 31)) >> 32) == 1;
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
			uint8_t ZN_4;
			if(!ZD_4){
				arq->Reg[instruction->z] = (int32_t)arq->Reg[instruction->x] /(int32_t)IplusSinal;
				ZN_4 = arq->Reg[instruction->z] == 0;
				uint8_t OV_4 = 0;
				changeSR( ZN_4,ZD_4,ShiftBit(arq->Reg[31],3,1),OV_4,ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
				fprintf(output,"0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
				arq->Reg[29] = arq->Reg[29]  + 4;
			} else {
					uint8_t OV_4 = 0;
					ZN_4 = ShiftBit(arq->Reg[31],6,1);
					if(ShiftBit(arq->Reg[31],1,1) == 0b0){
						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
					} else if(ShiftBit(arq->Reg[31],1,1) == 0b1){
						changeSR(ShiftBit(arq->Reg[31],6,1),1,ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),ShiftBit(arq->Reg[31],2,1),ShiftBit(arq->Reg[31],0,1),arq);
						arq->Reg[26] = 0;
						fprintf(output,"0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsReg(instruction->x,2),IplusSinal,arq->Reg[instruction->z],arq->Reg[31]);
						fprintf(output,"[SOFTWARE INTERRUPTION]\n");
						arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[26];
						arq->Reg[26] = arq->Reg[29];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Mem[arq->Reg[30]] = arq->Reg[27];
						arq->Reg[27] = arq->Reg[29];
						arq->Reg[30] = arq->Reg[30] - 4;
						arq->Reg[29] = 0x00000008;
					}
			}
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
			if(instruction->x == 0 ) arq->Reg[instruction->x] = 0;
			int64_t sinal = ((instruction->i & 0x7FFF)>> 15 == 1) && (((instruction->i & 0xFFFF) >> 16) != 1)? 0xFFFF << 15 : 0;
			uint64_t CMPI = (int64_t)arq->Reg[instruction->x] - (int64_t)(sinal+instruction->i);
			uint8_t RegX31 = ShiftBit(arq->Reg[31],31,1);
			uint8_t CMPI31 = ShiftBit64(CMPI,31,1);
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
		case 0b100000:
			sprintf(instrucao,"reti");
			arq->Reg[30] = arq->Reg[30] + 4;
			arq->Reg[27] = arq->Mem[arq->Reg[30]];
			uint32_t ipc = arq->Reg[30];
			arq->Reg[30] = arq->Reg[30] + 4;
			arq->Reg[26] = arq->Mem[arq->Reg[30]];
			uint32_t cr = arq->Reg[30];
			arq->Reg[30] = arq->Reg[30] + 4;
			uint32_t oldPC2 = arq->Reg[29];
			arq->Reg[29] = arq->Mem[arq->Reg[30]];
			fprintf(output,"0x%08X:\t%-25s\tIPC=MEM[0x%08X]=0x%08X,CR=MEM[0x%08X]=0x%08X,PC=MEM[0x%08X]=0x%08X\n",oldPC2,instrucao,ipc,arq->Reg[27],cr,arq->Reg[26],arq->Reg[30],arq->Reg[29]);
			//printf("------------- R[27] = %08X\n",arq->Reg[27]);
			break;
		case 0b100001:
			if(instruction->i == 0){
				sprintf(instrucao,"cbr %s[%d]",whyIsReg(instruction->z,1),instruction->x);
				changerValue(&arq->Reg[instruction->z],instruction->x,0);
				fprintf(output,"0x%08X:\t%-25s\t%s=%s\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsRegHex(arq->Reg[instruction->z]));
			}else{
				sprintf(instrucao,"sbr %s[%d]",whyIsReg(instruction->z,1),instruction->x);
				changerValue(&arq->Reg[instruction->z],instruction->x,1);
				fprintf(output,"0x%08X:\t%-25s\t%s=%s\n",arq->Reg[29],instrucao,whyIsReg(instruction->z,2),whyIsRegHex(arq->Reg[instruction->z]));
			}
			arq->Reg[29] = arq->Reg[29]  + 4;
			break;
		case 0b011110:
			int64_t sinal2 = (((instruction->i & (0b1 << 15))>>15)) == 1? 0xFFFF << 15 : 0;
			int32_t value = instruction->i + sinal2;
			uint32_t oldpc = arq->Reg[29];
			sprintf(instrucao,"call [%s%s%d]",whyIsReg(instruction->x ,1),value >=0?("+"):(""),value);
			arq->Mem[arq->Reg[30] >> 2] = arq->Reg[29] + 4;
			arq->Reg[29] = (arq->Reg[instruction->x] + value) << 2;
			
			arq->Reg[30] = arq->Reg[30] - 4;
			fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n",oldpc,instrucao,arq->Reg[29],arq->Reg[30],arq->Mem[arq->Reg[30] >> 2]);
			break;
		default:
			changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
			fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
			fprintf(output,"[SOFTWARE INTERRUPTION]\n");
			//arq->Reg[27] = arq->Reg[29];
			arq->Reg[26] = (arq->Reg[28] & (0x7F << 25));
			arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
			arq->Reg[30] = arq->Reg[30] - 4;
			arq->Mem[arq->Reg[30]] = arq->Reg[26];
			arq->Reg[26] = arq->Reg[29];
			arq->Reg[30] = arq->Reg[30] - 4;
			arq->Mem[arq->Reg[30]] = arq->Reg[27];
			arq->Reg[30] = arq->Reg[30] - 4;
			arq->Reg[29] = 0x00000004;
			break;
	}
}

void executionInstructionTypeS(ArqResources* arq,typeS* instruction,FILE* output){
	char instrucao[30];
	switch(instruction->opcode){
		int IplusSinal;
		case 0b110111:
			IplusSinal = (ShiftBit(instruction->i,25,1)) == 1? (0x3F << 26)+ instruction->i : instruction->i;
			sprintf(instrucao, "bun %d", IplusSinal);
			if(IplusSinal == 0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			} else {
				if(IplusSinal == -1){
					arq->Reg[29] = arq->Reg[29];
					fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29]);
				} else {
					fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n", arq->Reg[29],instrucao,arq->Reg[29] + 4*IplusSinal + 4);
					arq->Reg[29] = arq->Reg[29] + 4*IplusSinal + 4;
				}
			}
			break;
		case 0b111111:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao, "int %d", IplusSinal);
			if(IplusSinal != 0){
				arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Mem[arq->Reg[30]] = arq->Reg[26];
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Mem[arq->Reg[30]] = arq->Reg[27];
				arq->Reg[30] = arq->Reg[30] - 4;
				arq->Reg[26] = instruction->i;
				arq->Reg[27] = arq->Reg[29];
				arq->Reg[29] = 0x0000000C;
				fprintf(output,"0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08X\n", arq->Reg[27],instrucao,arq->Reg[26],arq->Reg[29]);
				fprintf(output,"[SOFTWARE INTERRUPTION]\n");
			} else {
				uint32_t oldpc = arq->Reg[29];
				arq->Reg[29] = 0;
				arq->Reg[26] = 0;
				fprintf(output,"0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08X\n", oldpc,instrucao,arq->Reg[26],arq->Reg[29]);
			}
			break;
		case 0b101010:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao,"bat %d",IplusSinal);
			if((arq->Reg[31] & 0x00000040) == 0b0 && (arq->Reg[31] & 0x00000001) == 0b0){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}

			break;
		case 0b101100:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao,"bbe %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],6,1) == 1 || ShiftBit(arq->Reg[31],0,1) == 1){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}

			break;
		case 0b101101:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		      if(IplusSinal >> 25 == 1){
		        IplusSinal = IplusSinal | 0xFC000000; 
		      }
      //printf("Iplus = %d\n",IplusSinal);
			sprintf(instrucao,"beq %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],6,1) == 1){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}

			break;
		case 0b101111:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao,"bgt %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],6,1) == 0 && (ShiftBit(arq->Reg[31],4,1) == ShiftBit(arq->Reg[31],3,1))){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			break;
		case 0b110001:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao,"ble %d",IplusSinal);

			if(ShiftBit(arq->Reg[29],6,1) == 1 || ShiftBit(arq->Reg[29],4,1) != ShiftBit(arq->Reg[29],3,1)){
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29]+4 + (IplusSinal << 2));
				arq->Reg[29] = arq->Reg[29] + 4 + (IplusSinal << 2);
			} else {
				fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X\n",arq->Reg[29],instrucao,arq->Reg[29] + 4);
				arq->Reg[29] = arq->Reg[29] + 4;
			}
			break;
		case 0b110011:
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
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
			IplusSinal = arq->Reg[28] & 0x03FFFFFF;
		    if(IplusSinal >> 25 == 1){
		      IplusSinal = IplusSinal | 0xFC000000; 
		    }
			sprintf(instrucao,"bzd %d",IplusSinal);
			if(ShiftBit(arq->Reg[31],5,1)){
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
			sprintf(instrucao,"call %ld",value);
			fprintf(output,"0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n",oldpc,instrucao,arq->Reg[29],arq->Reg[30],arq->Mem[arq->Reg[30] >> 2]);
			arq->Reg[30] = arq->Reg[30] - 4;
			break;
		default:
		changeSR(ShiftBit(arq->Reg[31],6,1),ShiftBit(arq->Reg[31],5,1),ShiftBit(arq->Reg[31],4,1),ShiftBit(arq->Reg[31],3,1),1,ShiftBit(arq->Reg[31],0,1),arq);
		fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",arq->Reg[29]);
		fprintf(output,"[SOFTWARE INTERRUPTION]\n");
		//arq->Reg[27] = arq->Reg[29];
		arq->Reg[26] = (arq->Reg[28] & (0x7F << 25));
		arq->Mem[arq->Reg[30]] = arq->Reg[29] + 4;
		arq->Reg[30] = arq->Reg[30] - 4;
		arq->Mem[arq->Reg[30]] = arq->Reg[26];
		arq->Reg[26] = arq->Reg[29];
		arq->Reg[30] = arq->Reg[30] - 4;
		arq->Mem[arq->Reg[30]] = arq->Reg[27];
		arq->Reg[30] = arq->Reg[30] - 4;
		arq->Reg[29] = 0x00000004;
		break;
	}
}

void processFile(FILE* input,FILE* output){
	ArqResources* newArq = inicialization();
	int flag = 0;
	uint32_t i = 0;
	while(!feof(input)){
		fscanf(input,"0x%8X\n",&newArq->Mem[i]);
    //printf("0x%08X\n",newArq->Mem[i]);
		i++;
	}
	i = 0;
	uint8_t exec = 1;
	fprintf(output,"[START OF SIMULATION]\n");
	while(exec){
		//printf("R[27] = 0x%08X\n",newArq->Reg[27]);
		//printf("0x%08X: opstnew = 0x%08X - opst = 0x%08X\n",newArq->Reg[29],newArq->fpu->OPSTnew,newArq->fpu->OPST);
		//printf("fpu z 0x%08X\n",newArq->fpu->z);
		//printf("0x%08X\n",newArq->Reg[29]);
		if(newArq->wdog->isActivate == 1 && ShiftBit(newArq->Reg[31],1,1) == 1){
			newArq->wdog->isActivate = 0;
			
			newArq->Mem[newArq->Reg[30]] = newArq->Reg[29];
			newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Mem[newArq->Reg[30]] = newArq->Reg[26];
			newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Mem[newArq->Reg[30]] = newArq->Reg[27];
			//printf("int 1 -> Mem[0x%08X] 0x%08X\n",newArq->Reg[30],newArq->Reg[27]);
			newArq->Reg[27] = newArq->Reg[29];
			newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Reg[26] = 0xE1AC04DA;
			newArq->Reg[29] = 0x00000010;
			fprintf(output,"[HARDWARE INTERRUPTION 1]\n");
			//printf("interruption 1 -------------------------------\n");
		} else {
			if(newArq->wdog->watchCount == 0 && newArq->wdog->setWatchdog == 1){ newArq->wdog->setWatchdog = 0; newArq->wdog->isActivate = 1;}
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
				executionInstructionTypeU(newArq,newIU,output);

			}
			else if(optype == 'F'){
				newIF->opcode = opcode;
				newIF->x = (newArq->Reg[28] & (0b11111 << 16)) >> 16;
				newIF->z = (newArq->Reg[28] & (0b11111 << 21)) >> 21;
				newIF->i = (newArq->Reg[28] & (0xFFFF));
				executionInstructionTypeF(newArq,newIF,output);
				//printf("pc = %08X\n",newArq->Reg[29]);
			}
			else if(optype == 'S'){
				newIS->opcode = opcode;
				newIS->i = (newArq->Reg[28] & (0b11111111111111111111111111));
				executionInstructionTypeS(newArq,newIS,output);
				if(opcode == 0b111111 && newIS->i == 0){
					exec = 0;
					break;
				}
			}else if(optype != 'U' && optype != 'F' && optype != 'S')	{
				changeSR(ShiftBit(newArq->Reg[31],6,1),ShiftBit(newArq->Reg[31],5,1),ShiftBit(newArq->Reg[31],4,1),ShiftBit(newArq->Reg[31],3,1),1,ShiftBit(newArq->Reg[31],0,1),newArq);
				fprintf(output,"[INVALID INSTRUCTION @ 0x%08X]\n",newArq->Reg[29]);
				fprintf(output,"[SOFTWARE INTERRUPTION]\n");
				
				
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[29] + 4;
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[26];
				newArq->Reg[26] = newArq->Reg[29];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[27];
				newArq->Reg[27] = newArq->Reg[29];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Reg[29] = 0x00000004;
			}
			i++;
			//if(i == 2822) break;
		}
		if( ShiftBit(newArq->Reg[31],1,1) == 0b1 && newArq->fpu->cicle == 0 && ShiftBit(newArq->fpu->OPST,5,1) == 0b1 && (ShiftBit(newArq->fpu->OPST,0,5) > 0b01001 || ShiftBit(newArq->fpu->OPST,0,5) < 0b00001 || ShiftBit(newArq->fpu->OPST,0,5) == 0b100 || newArq->fpu->error == 1 )){
			
      			newArq->Mem[newArq->Reg[30]] = newArq->Reg[29];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[26];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->lastR27;
				//printf("int 2 -> Mem[0x%08X] 0x%08X\n",newArq->Reg[30],newArq->lastR27);
				
				newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Reg[26] = 0x01EEE754;
			newArq->Reg[29] = 0x00000014;
			fprintf(output,"[HARDWARE INTERRUPTION 2]\n");
			//newArq->Reg[27] = newArq->Reg[29] - 4;
			printf("interruption 2 -------------------------------\n");
      		newArq->fpu->OPST = newArq->fpu->OPST & 0b000000; 
			newArq->fpu->OPSTnew = 0x20;
			
		} else if((ShiftBit(newArq->fpu->OPST,0,5) >= 0b1 && ShiftBit(newArq->fpu->OPST,0,5) <= 0b100 && ShiftBit(newArq->fpu->OPST,5,1) == 0b0 && ShiftBit(newArq->Reg[31],1,1) == 0b1 && newArq->fpu->cicle == 0 )){
      			newArq->Mem[newArq->Reg[30]] = newArq->Reg[29];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[26];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->lastR27;
				//printf("int 3 -> Mem[0x%08X] 0x%08X\n",newArq->Reg[30],newArq->Reg[27]);
		
				
				newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Reg[26] = 0x01EEE754;
			newArq->Reg[29] = 0x00000018;
			fprintf(output,"[HARDWARE INTERRUPTION 3]\n");
		//	newArq->Reg[27] = newArq->Reg[29] - 4;
			//printf("interruption 3 -------------------------------\n");
			//printf("op = 0x%08X\n",ShiftBit(newArq->fpu->OPST,0,5));
      		newArq->fpu->OPST = newArq->fpu->OPST & 0b100000; 
			
		} else if(ShiftBit(newArq->fpu->OPST,0,5) >= 4 && ShiftBit(newArq->fpu->OPST,5,1) == 0b0 && newArq->fpu->cicle == 0  && ShiftBit(newArq->Reg[31],1,1) == 0b1 && newArq->Reg[29] != 0x0000001C){
			
      		newArq->Mem[newArq->Reg[30]] = newArq->Reg[29];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->Reg[26];
				newArq->Reg[30] = newArq->Reg[30] - 4;
				newArq->Mem[newArq->Reg[30]] = newArq->lastR27;
				//printf("int 4 -> Mem[0x%08X] 0x%08X\n",newArq->Reg[30],newArq->Reg[27]);
				
				newArq->Reg[30] = newArq->Reg[30] - 4;
			newArq->Reg[26] = 0x01EEE754;
			newArq->Reg[29] = 0x0000001C;
			fprintf(output,"[HARDWARE INTERRUPTION 4]\n");
			//newArq->Reg[27] = newArq->Reg[29] - 4;
			//printf("interruption 4 -------------------------------\n");
      		newArq->fpu->OPST = newArq->fpu->OPST & 0b100000;
			
		} 
		if(newArq->fpu->cicle > 0){ newArq->fpu->cicle--; newArq->Reg[27] = newArq->Reg[29]; }
		if(newArq->wdog->setWatchdog == 1 && newArq->wdog->watchCount > 0) newArq->wdog->watchCount--;
}
if(newArq->term->index > 0){
	fprintf(output,"[TERMINAL]\n");
	for(int i = 0;i < newArq->term->index;i++){
		fprintf(output,"%c",newArq->term->buffer[i]);
	}
	fprintf(output,"\n");
}
fprintf(output,"[END OF SIMULATION]");
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
