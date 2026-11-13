#include <stdio.h>

int add5(int x){
    return x+5;
}

int a = 42;
int x = 26;

int main(void)
{
    int b = add5(a);
    int y = add5(x);
    printf("%d\n",b);
    printf("%d\n",y);
    return 0;
}

