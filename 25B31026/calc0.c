#include <stdio.h>
#include <stdbool.h>

// 入出力周りのテンプレート
void printc(char tmp){
    printf("%c\n", tmp);
}
void prints(char* tmp){
    printf("%s\n", tmp);
}
void printi(int tmp){
    printf("%d\n", tmp);
}

// グローバル変数
bool hasError = false;

// 関数定義
int add(int x, int y){
    int ret;
    bool carry = __builtin_add_overflow(x,y,&ret);
    if (carry){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int sub(int x, int y){
    int ret;
    bool carry = __builtin_sub_overflow(x,y,&ret);
    if (carry){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int mul(int x, int y){
    int ret;
    bool carry = __builtin_mul_overflow(x,y,&ret);
    if (carry){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int div(int x, int y){
    int ret = x/y;
    bool carry = (y == 0);
    if (carry){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int eval(char op, int x, int y){
    switch (op)
    {
    case '+':
        return add(x,y);
    case '-':
        return sub(x,y);
    case '*':
        return mul(x,y);
    case '/':
        return div(x,y);
    default:
        return 0;
    }
}
int concat(int x, char c){
    int d = c - '0';
    if (x >= 0){
        return add(mul(x,10),d);
    } else {
        return sub(mul(x,10),d);
    }
}



// ほんへ
int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("don't match arguments.\n");
        return 1;
    }
    char *p = argv[1];

    char last_op = '+';

    int memory = 0; // メモリーの値
    int res = 0; // レジスタの値
    int current = 0; // 今入力されている値

    while (true) {
        if (*p == NULL){break;}
        char cmd = *p++;

        switch (cmd)
        {
        case 'C':  // Memory Clear
            memory = 0;
            break;
        case 'R':  // Recall Memory
            res = memory;
            break;
        case 'P':  // Memory Plus
            memory += eval(last_op,res,current);
            res = 0;
            last_op = '+';
            current = 0;
            break;
        case 'M':  // Memory Minus
            memory -= eval(last_op,res,current);
            res = 0;
            last_op = '+';

            current = 0;
            break;
        case 'S':  // Sign Inv
            current *= -1;
            break;
        case '+':
        case '-':
        case '*':
        case '/':
            res = eval(last_op,res,current);
            current = 0;
            last_op = cmd;
            break;
        case '=': // eval
            res = eval(last_op,res,current);
            break;
        default: // digit
            current = concat(current, cmd);
            break;
        }
    }

    // 最後に内部状態（？、レジスタの状態？）を出力
    if (hasError){
        printf("%s\n", "E");
    } else {
        printf("%d\n", res);
    }
}
