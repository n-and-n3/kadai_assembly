#include <stdio.h>

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

// 関数定義
int add(int x, int y){
    return x+y;
}
int sub(int x, int y){
    return x-y;
}
int mul(int x, int y){
    return x*y;
}
int div(int x, int y){
    return x/y;
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

    while (1) {
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
            if (current >= 0){
                current = 10*current + (cmd-'0');
            } else {
                current = 10*current - (cmd-'0');
            }
            break;
        }
    }

    // 最後に内部状態（？、レジスタの状態？）を出力
    printf("%d\n", res);
}
