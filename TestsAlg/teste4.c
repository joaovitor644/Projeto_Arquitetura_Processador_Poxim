#include <stdio.h>
#include <stdint.h>

uint32_t hexadecimal_division(uint32_t dividend, uint32_t divisor) {
    uint32_t quotient = 0;
    uint32_t remainder = 0;

    for (int i = 31; i >= 0; i--) {
        remainder <<= 1; // Shift left the remainder
        remainder |= (dividend >> i) & 1; // Add next bit of dividend to remainder
        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1 << i); // Set corresponding bit in quotient
        }
    }

    return quotient;
}

int main() {
    // Exemplo de utilização
    uint32_t dividend = 0x40000000;
    uint32_t divisor = 0x41900000;

    uint32_t resultado = hexadecimal_division(dividend, divisor);

    printf("Resultado da divisão em hexadecimal: 0x%X\n", resultado);

    return 0;
}
