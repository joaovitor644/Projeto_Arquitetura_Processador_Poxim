#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void processFile(FILE* input,FILE* output){

}

int main(int argc, char* argv[]){
	
	char* inputFile = argv[1];
	char* outputFile = argv[2];

	FILE* input = fopen(inputFile,"r");
	if(!input){
		return 1;
	}

	FILE* input = fopen(inputFile, "w");
	if(!output){
		fclose(input);
		return 1;
	}

	processFile(input,output);

	fclose(input);
	fclose(output);

	return 0;
}
