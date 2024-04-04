#include <stdio.h>
#include <stdint.h>

// Função para converter um número decimal para IEEE 754
void decimal_para_ieee754(float num) {
    // Ponteiro para acessar a representação binária do número
    uint32_t *ponteiro = (int32_t*)&num;
    
    // Pegar os bits individuais do número
    uint32_t bits = *ponteiro;
    
    // Extrair o bit de sinal
    uint32_t sinal = (bits >> 31) & 1;
    
    // Extrair o expoente
    uint32_t expoente = (bits >> 23) & 0xFF;
    
    // Extrair a mantissa
    uint32_t mantissa = bits & 0x7FFFFF; // Usar máscara corrigida
    uint32_t result = bits;

}

int main() {
    // Converter 0.111111 para IEEE 754
    float numero = 0.11111111;
    
    // Mostrar a representação IEEE 754
    printf("Representação IEEE 754 do número %f:\n", numero);
    decimal_para_ieee754(numero);
    
    return 0;
}
