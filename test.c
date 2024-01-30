#include <stdio.h>
#include "test.h"


void Equal_Decimal(int actual,int expected,char messager[64]){
	if(actual == expected)
		printf("%s\n",messager);
	else
		printf("[ERRO - expected %d , but the result is %d]\n",expected,actual);
}

