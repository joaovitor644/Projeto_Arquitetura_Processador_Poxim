#include <stdio.h>
#include <stdint.h>
#include <math.h>


int main(){
    
    float x = 1;//0.111111;
    printf("sem floor = %f\n",x);
    x = floor(x);
    printf("com floor = %f\n",x);

    return 0;

}