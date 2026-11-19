#include <cstdio>


int c = 10;
int main() {

    long long* p = (long long*)__builtin_frame_address(0);

    printf("A\n");

    if (c--){
        *(p+1) -= 2;
    }

    // printf("%lld\n", p);

    return (long long)main;
}