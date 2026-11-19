#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    SEMICOLON,
    PAREN,
    END,
} TokenKind;


typedef struct {
    TokenKind kind;
    char* p;
    int len;
} Token;

Token next_token(char** p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }

    if (**p == '\0') {
        return (Token){.kind = END, .p = *p, .len = 0};
    }

    if (**p == '+' || **p == '-' || **p == '*' || **p == '/') {
        Token token = {.kind = OPERATOR, .p = *p, .len = 1};
        (*p)++;
        return token;
    }

    if (**p >= '0' && **p <= '9') {
        char* start = *p;
        while (**p >= '0' && **p <= '9') {
            (*p)++;
        }
        return (Token){.kind = NUMBER, .p = start, .len = *p - start};
    }

    if ((**p >= 'a' && **p <= 'z') || (**p >= 'A' && **p <= 'Z')) {
        char* start = *p;
        while ((**p >= 'a' && **p <= 'z') || (**p >= 'A' && **p <= 'Z')) {
            (*p)++;
        }
        return (Token){.kind = IDENTIFIER, .p = start, .len = *p - start};
    }

    if (**p == ';') {
        Token token = {.kind = SEMICOLON, .p = *p, .len = 1};
        (*p)++;
        return token;
    }

    if (**p == '(' || **p == ')') {
        Token token = {.kind = PAREN, .p = *p, .len = 1};
        (*p)++;
        return token;
    }

    fprintf(stderr, "Unexpected character: %c\n", **p);
    exit(1);

}

void parse_statement(char **p);
void parse_expression(char **p);
void parse_term(char **p);
void parse_factor(char **p);


void test_tokenize_program(char* p) {
    while (*p) {
        Token token = next_token(&p);
        printf("Token: kind=%d, p=%.*s, len=%d\n", token.kind, token.len, token.p, token.len);
    }
}

void parse_program(char* p) {
    while (*p) {
        parse_statement(&p);
        Token token = next_token(&p);
        if (token.kind != SEMICOLON) {
            fprintf(stderr, "Expected ';' after statement\n");
            exit(1);
        }
    }
}

void parse_statement(char** p) {
    Token token = next_token(p);
    if (token.kind == IDENTIFIER) {
        if (token.len == 3 && strncmp(token.p, "let", 3) == 0) {
            Token var_token = next_token(p);
            if (var_token.kind != IDENTIFIER) {
                fprintf(stderr, "Expected identifier after 'let'\n");
                exit(1);
            }
        } else if (token.len == 5 && strncmp(token.p, "print", 5) == 0) {
            parse_expression(p);
            printf("    popq %%rsi\n");
            printf("    leaq .LC0(%%rip), %%rdi\n");
            printf("    movl $0, %%eax\n");
            printf("    call printf\n");
        } else {
            fprintf(stderr, "Unexpected identifier: %.*s\n", token.len, token.p);
            exit(1);
        }
    } else {
        fprintf(stderr, "Unexpected token in statement\n");
        exit(1);
    }
}

void parse_expression(char** p) {
    parse_term(p);
    while (true) {
        char* tmp = *p;
        Token token = next_token(p);
        if (token.kind == OPERATOR && (token.p[0] == '+' || token.p[0] == '-')) {
            char op = token.p[0];
            printf("    popq %%edi\n");
            printf("    popq %%eax\n");
            if (op == '+') {
                printf("    addl %%edi, %%eax\n");
            } else {
                printf("    subl %%edi, %%eax\n");
            }
            printf("    pushq %%rax\n");
            parse_term(p);
        } else {
            *p = tmp;
            break;
        }
    }
}

void parse_term(char** p) {
    parse_factor(p);
    while (true) {
        char* tmp = *p;
        Token token = next_token(p);
        if (token.kind == OPERATOR && (token.p[0] == '*' || token.p[0] == '/' || token.p[0] == '%')) {
            char op = token.p[0];
            printf("    popq %%edi\n");
            printf("    popq %%eax\n");
            if (op == '*') {
                printf("    imull %%edi, %%eax\n");
            } else if (op == '/') {
                printf("    cltd\n");
                printf("    idivl %%edi\n");
            } else {
                printf("    cltd\n");
                printf("    idivl %%edi\n");
                printf("    movl %%edx, %%eax\n");
            }
            printf("    pushq %%rax\n");
            parse_factor(p);
        } else {
            *p = tmp;
            break;
        }
    }
}

void parse_factor(char** p) {
    Token token = next_token(p);
    if (token.kind == NUMBER) {
        printf("    movl $%.*s, %%eax\n", token.len, token.p);
        printf("    pushq %%rax\n");
    } else if (token.kind == IDENTIFIER) {
        printf("    movl %.*s(%%rip), %%eax\n", token.len, token.p);
        printf("    pushq %%rax\n");
    } else if (token.kind == PAREN && token.p[0] == '(') {
        parse_expression(p);
        Token closing_paren = next_token(p);
        if (closing_paren.kind != PAREN || closing_paren.p[0] != ')') {
            fprintf(stderr, "Expected ')' after expression\n");
            exit(1);
        }
    } else if (token.kind == OPERATOR && token.p[0] == '-') {
        printf("    popq %%rax\n");
        printf("    negl %%eax\n");
        printf("    pushq %%rax\n");
        parse_factor(p);
    } else {
        fprintf(stderr, "Unexpected token in factor\n");
        exit(1);
    }
}

void emit_header() {
    printf(".section .rodata\n");
    printf(".LC0:\n");
    printf("    .string \"%%d\\n\"\n");
    printf(".text\n");
    printf(".globl main\n");
    printf("main:\n");
    printf("    pushq %%rbp\n");
    printf("    movq %%rsp, %%rbp\n");
}

void emit_footer() {
    printf("    movl $0, %%eax\n");
    printf("    popq %%rbp\n");
    printf("    ret\n");
    printf("\n");
}

char** stack;

// ほんへ
int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("don't match arguments.\n");
        return 1;
    }
    char *p = argv[1];

    emit_header();
    parse_program(p);
    emit_footer();
    return 0;
}
