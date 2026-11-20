#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    SEMICOLON,
    PAREN,
    EQUAL,
    RELATIONAL,
    END,
} TokenKind;


typedef struct {
    TokenKind kind;
    char* p;
    int len;
} Token;

int label_count = 0;

// 動的配列の扱い面倒くさすぎ！！！！！
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
    while (isspace((unsigned char)**p)) {
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

    if (**p == '(' || **p == ')' || **p == '{' || **p == '}') {
        Token token = {.kind = PAREN, .p = *p, .len = 1};
        (*p)++;
        return token;
    }

    if (**p == '=') {
        if (*(*p + 1) == '=') {
            Token token = {.kind = RELATIONAL, .p = *p, .len = 2};
            (*p) += 2;
            return token;
        }
        Token token = {.kind = EQUAL, .p = *p, .len = 1};
        (*p)++;
        return token;
    }

    if (**p == '!' && *(*p + 1) == '=') {
        Token token = {.kind = RELATIONAL, .p = *p, .len = 2};
        (*p) += 2;
        return token;
    }

    if (**p == '<' || **p == '>') {
        if (*(*p + 1) == '=') {
            Token token = {.kind = RELATIONAL, .p = *p, .len = 2};
            (*p) += 2;
            return token;
        }
        Token token = {.kind = RELATIONAL, .p = *p, .len = 1};
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
void parse_relational(char **p);


void test_tokenize_program(char* p) {
    while (*p) {
        Token token = next_token(&p);
        printf("Token: kind=%d, p=%.*s, len=%d\n", token.kind, token.len, token.p, token.len);
    }
}

void parse_program(char** p) {
    while (true) {
        char *start = *p;
        if (next_token(p).kind == END) {
            break;
        }
        *p = start;
        parse_statement(p);
        Token token = next_token(p);
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
            printf("    leaq .Lprint(%%rip), %%rdi\n");
            printf("    movq $0, %%rax\n");
            printf("    call printf\n");
        } else if (token.len == 6 && strncmp(token.p, "printc", 6) == 0) {
            parse_expression(p);
            printf("    popq %%rsi\n");
            printf("    leaq .Lprint_char(%%rip), %%rdi\n");
            printf("    movq $0, %%rax\n");
            printf("    call printf\n");
        } else if (token.len == 5 && strncmp(token.p, "input", 5) == 0) {
            token = next_token(p);
            if (token.kind != IDENTIFIER) {
                fprintf(stderr, "Expected identifier after 'input'\n");
                exit(1);
            }
            remember_variable(token);
            printf("    leaq .Lscan(%%rip), %%rdi\n");
            printf("    leaq %.*s(%%rip), %%rsi\n", token.len, token.p);
            printf("    movq $0, %%rax\n");
            printf("    call scanf\n");
        } else if (token.len == 6 && strncmp(token.p, "inputc", 6) == 0) {
            token = next_token(p);
            if (token.kind != IDENTIFIER) {
                fprintf(stderr, "Expected identifier after 'input'\n");
                exit(1);
            }
            remember_variable(token);
            printf("    leaq .Lscan_char(%%rip), %%rdi\n");
            printf("    leaq %.*s(%%rip), %%rsi\n", token.len, token.p);
            printf("    movq $0, %%rax\n");
            printf("    call scanf\n");
        } else if (token.len == 2 && strncmp(token.p, "if", 2) == 0) {
            Token open_paren = next_token(p);
            if (open_paren.kind != PAREN || open_paren.p[0] != '(') {
                fprintf(stderr, "Expected '(' after 'if'\n");
                exit(1);
            }
            parse_expression(p);
            Token close_paren = next_token(p);
            if (close_paren.kind != PAREN || close_paren.p[0] != ')') {
                fprintf(stderr, "Expected ')' after expression\n");
                exit(1);
            }
            Token open_brace = next_token(p);
            if (open_brace.kind != PAREN || open_brace.p[0] != '{') {
                fprintf(stderr, "Expected '{' after ')'\n");
                exit(1);
            }
            printf("    popq %%rax\n");
            printf("    testq %%rax, %%rax\n");
            printf("    jz .L_iflabel_%d\n", label_count);
            parse_program(p);
            printf(".L_iflabel_%d:\n", label_count);
            label_count++;
            Token close_brace = next_token(p);
            if (close_brace.kind != PAREN || close_brace.p[0] != '}') {
                fprintf(stderr, "Expected '}' after if block\n");
                exit(1);
            }
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
    parse_relational(p);
    while (true) {
        char* tmp = *p;
        Token token = next_token(p);
        if (token.kind == RELATIONAL) {
            Token op = token;
            parse_relational(p);
            printf("    popq %%rdi\n");
            printf("    popq %%rax\n");
            if (op.len == 2 && op.p[0] == '=' && op.p[1] == '=') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    sete %%al\n");
                printf("    movzbq %%al, %%rax\n");
            } else if (op.len == 2 && op.p[0] == '!' && op.p[1] == '=') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    setne %%al\n");
                printf("    movzbq %%al, %%rax\n");
            } else if (op.len == 1 && op.p[0] == '<') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    setl %%al\n");
                printf("    movzbq %%al, %%rax\n");
            } else if (op.len == 1 && op.p[0] == '>') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    setg %%al\n");
                printf("    movzbq %%al, %%rax\n");
            } else if (op.len == 2 && op.p[0] == '<' && op.p[1] == '=') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    setle %%al\n");
                printf("    movzbq %%al, %%rax\n");
            } else if (op.len == 2 && op.p[0] == '>' && op.p[1] == '=') {
                printf("    cmpq %%rdi, %%rax\n");
                printf("    setge %%al\n");
                printf("    movzbq %%al, %%rax\n");
            }
            printf("    pushq %%rax\n");
        } else {
            *p = tmp;
            break;
        }
    }
}

void parse_relational(char** p) {
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
    printf(".Lprint:\n");
    printf("    .string \"%%ld\\n\"\n");
    printf(".Lprint_char:\n");
    printf("    .string \"%%c\"\n");
    printf(".Lscan:\n");
    printf("    .string \"%%ld\"\n");
    printf(".Lscan_char:\n");
    printf("    .string \" %%c\"\n");
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
    parse_program(&p);
    emit_footer();
    return 0;
}
