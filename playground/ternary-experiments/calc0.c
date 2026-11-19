#include <stdio.h>
#include <stdbool.h>


// グローバル変数
bool hasError = false;
char last_op = '+';
char op = '+';
int memory = 0; // メモリーの値
int res = 0; // レジスタの値
int cur = 0; // 今入力されている値
int states = 0; // 0:数字入力状態, 1:演算子入力状態

// 関数定義
bool is_memory(char x){
    return x == 'C' || x == 'R' || x == 'P' || x == 'M';
}

bool is_number(char x){
    return x == 'S' || ('0' <= x && x <= '9');
}

bool is_operand(char x){
    return x == '+' || x == '-' || x == '*' || x == '/';
}

bool is_equal(char x){
    return x == '=';
}

void reset(){
    cur = 0;
    op = '+';
}


int add(int x, int y){
    int ret;
    bool overflow = __builtin_add_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int sub(int x, int y){
    int ret;
    bool overflow = __builtin_sub_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int mul(int x, int y){
    int ret;
    bool overflow = __builtin_mul_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int div(int x, int y){
    int ret = (y != 0 ? x/y : 0);
    bool overflow = (y == 0);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int eval(char cmd, int x, int y){
    switch (cmd)
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
        hasError = true;
        return 0;
    }
}

void op_memory(char cmd){
    switch (cmd)
    {
    case 'R':
        res = memory;
        break;
    case 'C':
        memory = 0;
        break;
    case 'P':
        memory = add(memory,eval(last_op,res,cur));
        last_op = op;
        reset();
        res=0;
        break;
    case 'M':
        memory = sub(memory,eval(last_op,res,cur));
        last_op = op;
        reset();
        res=0;
        break;
    }

}


int concat(int x, char c){
    if (c == 'S'){
        return mul(x,-1);
    } else {
        int d = c - '0';
        if (x >= 0){
            return add(mul(x,10),d);
        } else {
            return sub(mul(x,10),d);
        }
    }
}

void display(){
    if (hasError){
        printf("E\n");
    } else {
        printf("%d\n", res);
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


    while (true) {
        if (*p == '\0'){break;}
        char cmd = *p++;
        if (states == 0){
            if(is_operand(cmd)){
                op = cmd;
                states = 1;
            } else if (is_number(cmd)){
                cur = concat(cur,cmd);
            } else if (is_memory(cmd)){
                op_memory(cmd);
            } else if (is_equal(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
            }
        } else {
            if(is_operand(cmd)){
                op = cmd;
            } else if (is_number(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
                cur = concat(cur,cmd);
                states = 0;
            } else if (is_memory(cmd)){
                op_memory(cmd);
            } else if (is_equal(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
            }
        }
    }

    // 最後に内部状態（？、レジスタの状態？）を出力
    display();
}
