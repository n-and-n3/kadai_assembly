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
    EQUAL,
    END,
} TokenKind;


typedef struct {
    TokenKind kind;
    char* p;
    int len;
} Token;

// 変数名は入力文字列中の位置と長さで保持する。重複は登録しない。
static Token *variables;
static size_t variable_count;
static size_t variable_capacity;

static void remember_variable(Token token) {
    for (size_t i = 0; i < variable_count; i++) {
        if (variables[i].len == token.len &&
            strncmp(variables[i].p, token.p, token.len) == 0) {
            return;
        }
    }

    if (variable_count == variable_capacity) {
        size_t new_capacity = variable_capacity == 0 ? 8 : variable_capacity * 2;
        Token *new_variables = realloc(variables, new_capacity * sizeof *variables);
        if (new_variables == NULL) {
            perror("realloc");
            exit(1);
        }
        variables = new_variables;
        variable_capacity = new_capacity;
    }
    variables[variable_count++] = token;
}

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

    if (**p == '=') {
        Token token = {.kind = EQUAL, .p = *p, .len = 1};
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
        if (token.len == 3 && strncmp(token.p, "int", 3) == 0) {
            Token var_token = next_token(p);
            if (var_token.kind != IDENTIFIER) {
                fprintf(stderr, "Expected identifier after 'int'\n");
                exit(1);
            }
            remember_variable(var_token);
        } else if (token.len == 5 && strncmp(token.p, "print", 5) == 0) {
            parse_expression(p);
            printf("    popq %%rsi\n");
            printf("    leaq .LC0(%%rip), %%rdi\n");
            printf("    movq $0, %%rax\n");
            printf("    call printf\n");
        } else {
            Token eq_token = next_token(p);
            if (eq_token.kind != EQUAL || eq_token.p[0] != '=') {
                fprintf(stderr, "Expected '=' after identifier\n");
                exit(1);
            }
            parse_expression(p);
            printf("    popq %%rax\n");
            printf("    movq %%rax, %.*s(%%rip)\n", token.len, token.p);
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
            parse_term(p);
            printf("    popq %%rdi\n");
            printf("    popq %%rax\n");
            if (op == '+') {
                printf("    addq %%rdi, %%rax\n");
            } else {
                printf("    subq %%rdi, %%rax\n");
            }
            printf("    pushq %%rax\n");
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
            parse_factor(p);
            printf("    popq %%rdi\n");
            printf("    popq %%rax\n");
            if (op == '*') {
                printf("    imulq %%rdi, %%rax\n");
            } else if (op == '/') {
                printf("    cqto\n");
                printf("    idivq %%rdi\n");
            } else {
                printf("    cqto\n");
                printf("    idivq %%rdi\n");
                printf("    movq %%rdx, %%rax\n");
            }
            printf("    pushq %%rax\n");
        } else {
            *p = tmp;
            break;
        }
    }
}

void parse_factor(char** p) {
    Token token = next_token(p);
    if (token.kind == NUMBER) {
        printf("    movabsq $%.*s, %%rax\n", token.len, token.p);
        printf("    pushq %%rax\n");
    } else if (token.kind == IDENTIFIER) {
        printf("    movq %.*s(%%rip), %%rax\n", token.len, token.p);
        printf("    pushq %%rax\n");
    } else if (token.kind == PAREN && token.p[0] == '(') {
        parse_expression(p);
        Token closing_paren = next_token(p);
        if (closing_paren.kind != PAREN || closing_paren.p[0] != ')') {
            fprintf(stderr, "Expected ')' after expression\n");
            exit(1);
        }
    } else if (token.kind == OPERATOR && token.p[0] == '-') {
        parse_factor(p);
        printf("    popq %%rax\n");
        printf("    negq %%rax\n");
        printf("    pushq %%rax\n");
    } else {
        fprintf(stderr, "Unexpected token in factor\n");
        exit(1);
    }
}

void emit_header() {
    printf(".section .rodata\n");
    printf(".LC0:\n");
    printf("    .string \"%%ld\\n\"\n");
    printf(".text\n");
    printf(".globl main\n");
    printf("main:\n");
    printf("    pushq %%rbp\n");
    printf("    movq %%rsp, %%rbp\n");
}

void emit_footer() {
    printf("    movq $0, %%rax\n");
    printf("    popq %%rbp\n");
    printf("    ret\n");
    printf("\n");
    if (variable_count != 0) {
        printf(".section .bss\n");
        for (size_t i = 0; i < variable_count; i++) {
            printf("    .balign 8\n");
            printf("%.*s:\n", variables[i].len, variables[i].p);
            printf("    .zero 8\n");
        }
    }
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
