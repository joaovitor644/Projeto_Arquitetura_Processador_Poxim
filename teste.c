#include <stdio.h>
#include <stdint.h>

// Função principal para converter um valor flutuante para IEEE-754 em hexadecimal
void convertToIEEE754(float value) {
    // Ponteiro para acessar o valor binário da variável float
    uint32_t *ptr = (uint32_t *)&value;

    // Obtendo o valor binário
    uint32_t binaryValue = *ptr;

    // Mostrando o resultado em hexadecimal
    printf("Formato IEEE-754 em hexadecimal para o valor %.2f:\n", value);
    printf("%08X\n", binaryValue);
}

int main() {
    float floatValue;
    printf("Digite o valor flutuante: ");
    scanf("%f", &floatValue);

    convertToIEEE754(floatValue);

    return 0;
}