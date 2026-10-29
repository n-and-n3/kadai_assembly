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

    int  memory = 0, display = 0, current = 0;

    while (*p++ != NULL) {
        char cmd = *p;
        switch (cmd)
        {
        case 'C':  // Memory Clear
            memory = 0;
            break;
        case 'R':  // Recall Memory
            current = memory;
            break;
        case 'P':
            memory += current;
            current = 0;
            break;
        case 'M':
            memory -= current;
            current = 0;
            break;
        case 'S':
            current *= -1;
            break;
        case '+':
        case '-':
        case '*':
        case '/':
            temporary = eval(last_op,temporary,current);
            current = 0;
            last_op = cmd;
            break;
        case '=':
            temporary = eval(last_op,temporary,current);
            printf("%d\n", temporary);
            break;
        default:
            temporary = 10*temporary + (cmd-'0');
            break;
        }
    }

}
