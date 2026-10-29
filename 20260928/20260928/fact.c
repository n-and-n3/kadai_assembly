#include <stdio.h>

/* Calculate n! recursively. This int version is for 0 <= n <= 12. */
int fact(int n)
{
    if (n <= 1) {
        return 1;
    }

    return n * fact(n - 1);
}

int main(void)
{
    int n = 5;

    printf("fact (%d) = %d\n", n, fact(n));
    return 0;
}

