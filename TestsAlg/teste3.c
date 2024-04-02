#include <stdio.h>
#include <stdint.h>

// Função para calcular a operação desejada com dois valores float em hexadecimal
int calcular_operacao(float valor_hex_x, float valor_hex_y) {
    // Converter os valores hexadecimais para números de ponto flutuante

    // Extrair os expoentes
    uint32_t expoente_x = ((*(uint32_t*)&valor_hex_x) >> 23) & 0xFF;
    uint32_t expoente_y = ((*(uint32_t*)&valor_hex_y) >> 23) & 0xFF;
    if(expoente_x != 0) expoente_x = expoente_x - 127;
    if(expoente_y != 0) expoente_y = expoente_y - 127;
    // Calcular a operação desejada
    int resultado = abs(expoente_x - expoente_y) + 1;

    return resultado;
}

int main() {
    // Variáveis para armazenar os valores hexadecimais como inteiros
    float valor_int_x, valor_int_y;

    // Solicitar ao usuário que insira os valores hexadecimais
    printf("Insira o valor hexadecimal do primeiro float: ");
    scanf("%x", &valor_int_x);
    printf("Insira o valor hexadecimal do segundo float: ");
    scanf("%x", &valor_int_y);

    // Chamar a função para calcular a operação desejada
    int resultado = calcular_operacao(valor_int_x,valor_int_y);
    printf("|expoente(X) - expoente(Y)| + 1 = %d\n", resultado);

    return 0;
}

