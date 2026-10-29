#include <stdio.h>

// 入出力周りのテンプレート
char inputc(){
    char tmp;
    scanf("%c",&tmp);
    return tmp;
}
void printc(char tmp){
    printf("%c\n", tmp);
}
void prints(char* tmp){
    printf("%s\n", tmp);
}
void printi(int tmp){
    printf("%d\n", tmp);
}

// 関数定義
int add(int x, int y){
    return x+y;
}
int sub(int x, int y){
    return x+y;
}
int mul(int x, int y){
    return x+y;
}
int div(int x, int y){
    return x+y;
}


// ほんへ
int main (int argc, char *argv [])
{
    if(argc!=2){return 1;}

    char last_op, *p = argv [1];
    int  num1, num2, acc;

    num1    = *p++ - '0';
    last_op = *p++;
    num2    = *p++ - '0';
    switch (last_op) {
    case '+':
        acc = num1 + num2;
        break;
    case '-':
        acc = num1 - num2;
        break;
    }

    printf ("%d\n", acc);
}

