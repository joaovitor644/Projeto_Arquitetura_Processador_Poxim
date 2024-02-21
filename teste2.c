#include <stdio.h>
#include <stdint.h>

// Função para extrair o expoente de um valor hexadecimal de um float de acordo com o padrão IEEE 754
int expoente_ieee754(float valor_int) {
    // Converter o valor hexadecimal para um número de ponto flutuante

    // Extrair o expoente
  uint32_t expoente = ((*(uint32_t*)&valor_int) >> 23) & 0xFF;

    // Subtrair o bias do IEEE 754 (127) para obter o valor real do expoente
  return expoente == 0? expoente : expoente-127;
}

int main() {
    // Variável para armazenar o valor hexadecimal como inteiro
    float valor_int;

    // Solicitar ao usuário que insira o valor hexadecimal
    printf("Insira o valor hexadecimal de um float: ");
    scanf("%x", &valor_int);

    // Chamar a função para calcular o expoente
    int expoente = expoente_ieee754(valor_int);
    printf("Expoente do valor: %d\n", expoente);

    return 0;
}

