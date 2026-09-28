/* 
 * SCALC: Stack based Calculator using RPN.
 */

/*
 * Copyright (C) 2026 Carlos G. S. 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Contact: Instagram DMs @cgs.dev
 */

/* cc scalc.c -o scalc */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#define MAX_STACK_SIZE 1024
#define MAX_TOKEN_SIZE 512

typedef char Bool;
#define True 1
#define False 0


typedef enum Status {
    SUCCESS,
    QUIT,
    FAILED_MALLOC,
    BAD_PTR,
    STACK_OVERFLOW,
    UNEXPECTED_SYMBOL,
    UNKNOWN_SYMBOL,
    DIVISION_BY_ZERO
} Status;


typedef enum Op {
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_MOD,
    OP_POW,
    OP_FACTORIAL,
    OP_SQRT,
    OP_LN,
    OP_LOG,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_ATAN,
    _OP_COUNT
} Op;

static const size_t op_input_amount[_OP_COUNT] = {
    0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1
};

typedef enum Constant {
    CONST_NULL,
    CONST_PI,
    CONST_E,
    CONST_INF,
    _CONST_COUNT
} Constant;

static const double constants[_CONST_COUNT] = {
    0.0f,
    3.14159265358979323846f,
    2.71828182845904523536f,
    INFINITY
};

typedef enum Command {
    CMD_NONE,
    CMD_SWAP,
    CMD_LS,
    CMD_PRINT,
    CMD_PRINT_AND_POP,
    CMD_DUP,
    CMD_CLEAR_SCREEN,
    CMD_CLEAR_STACK,
    CMD_PRINT_OPS,
    CMD_PRINT_HELP,
    CMD_EVAL_TOP,
    CMD_QUIT,
    _CMD_COUNT
} Command;

typedef struct Symbol {
    enum {
        S_UNDEFINED,
        S_NUMBER,
        S_OP
    } kind;
    union {
        double num;
        Op op;
    } as;
} Symbol;

typedef struct Stack {
    Symbol data[MAX_STACK_SIZE];
    size_t top;
} Stack;


static Stack stack;


void        print_help(void);
void        print_usage(const char *program);
void        print_ops(void);

Bool        is_number(const char *s);
char       *read_line(size_t *out_length);
char       *file_to_str(const char *filename);
char       *trim_str(char *str);
double      factorial(double n);

const char *err_to_string(Status err);

Op       str_to_op(const char *s);
Constant str_to_const(const char *s);
Command  str_to_cmd(const char *s);

size_t op_get_input_amount(Op op);


Symbol symbol_print(Symbol s);
Symbol symbol_print_top(Bool pop);
void   stack_print(void);

Status symbol_push(Symbol s);
Symbol symbol_pop(void);
Symbol symbol_get(void);

Status cmd_eval(Command cmd);

Status stack_eval_top(double *res, Bool pop);

Status eval_string(const char *str);

void print_usage(const char *programm) {
    printf("usage:\n");
    printf("%s [-h] [--file file.scalc] \n", programm);
    printf("  -h: print this message\n");
    printf("  --file: evaluate a file\n");
    printf("  no options: read and evaluate from stdin\n");
}

void print_help(void) {
    printf("\x1b[0;39m");
    printf("SCALC: Stack Calculator.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  c: clear the whole stack\n");
    printf("  ls: print the whole stack\n");
    printf("  p: print the Top of the Stack\n");
    printf("  P: pop and print the Top of the Stack\n");
    printf("  d: duplicate the first Element of the Stack\n");
    printf("  s: swap the first two Elements of the Stack\n");
    printf("  e: evaluate the Top of the Stack\n");
    printf("  q: quit\n");
    printf("  help: print this message\n");
    printf("\n");
    printf("Push a number to the stack by typing it into the prompt.\n");
    printf("You can perform Operations with elements of the\ntop of the Stack by using Operators.\n");
    printf("Operators:\n");
    print_ops();
    printf("\n");
    printf("Constants:\n");
    printf("'pi': pi\n");
    printf("'euler': euler's number\n");
    printf("'inf': infinity\n");
}

void print_ops(void) {
    printf("'+' / 'add': add the top two Numbers of the Stack\n");
    printf("'-' / 'sub': subtract the top most Number from the Stack from the second Number in the Stack\n");
    printf("'*' / 'mul': multiply the top two Numbers of the Stack\n");
    printf("'/' / 'div': divide the top most Number from the Stack by the Second number in the Stack\n");
    printf("'%%' / 'mod': get the Modulo from the top two Numbers of the Stack\n");
    printf("'^' / 'pow': raise the second Number of the Stack to the first Number of the Stack\n");

    printf("'v/' / 'sqrt': get the square Root of the top most Number of the Stack\n");
    printf("'ln': get the natural Logarithm of the top of the Stack\n");
    printf("'log': get the base 10 Logarithm of the top of the Stack\n");
    printf("'sin': get the sine function's result of the top of the Stack\n");
    printf("'cos': get the cosine function's result of the top of the Stack\n");
    printf("'tan': get the tangent of the top of the Stack\n");
    printf("'atan': get the arc tangent of the top of the Stack\n");
}

char *file_to_str(const char *filename) {
    FILE *file;
    size_t file_size;
    char *buf;

    file = fopen(filename, "rb");

    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);

    file_size = ftell(file);

    fseek(file, 0, SEEK_SET);

    buf = (char *) malloc((file_size + 1) * sizeof(char));

    if (!buf) {
        fclose(file);
        return NULL;
    }

    fread(buf, 1, file_size, file);
    buf[file_size] = '\0';

    fclose(file);

    return buf;
}

char *trim_str(char *str) {
    char *start;
    char *end;

    if (str == NULL) return NULL;
 
    start = str;

    while (isspace((unsigned char)*start)) {
        start++;
    }
 
    if (*start == '\0') {
        *str = '\0'; 
        return str;
    }
 
    end = start;
    while (*end != '\0') {
        end++;
    }
    end--; 
 
    while (end >= start && isspace((unsigned char)*end)) {
        end--;
    }
    end++;  
    *end = '\0';  
 
    return start;
}

char *read_line(size_t *out_length) {
    size_t length;
    size_t capacity;
    char *buf;
    int c;

    length = 0;
    capacity = 1024;
    buf = (char *) malloc(capacity*sizeof(char));

    if (buf == NULL)
        return NULL;


    while ((c = getc(stdin)) != '\n') {
        if (length + 1 >= capacity) {
            char *tmp;
            capacity *= 2;

            tmp = realloc(buf, capacity*sizeof(char));
            if (tmp == NULL) {
                free(buf);
                return NULL;
            }

            buf = tmp;
        }

        buf[length++] = (char)c;
    }

    buf[length] = '\0';

    if (out_length != NULL)
        *out_length = length;

    return buf;
}

const char *err_to_string(Status err) {
    switch (err) {
        case SUCCESS: 
            return "No Error";
        case FAILED_MALLOC: 
            return "Failed Malloc (buy more ram lol)";
        case QUIT:
            return "Quit";
        case BAD_PTR:
            return "Null Pointer Exception";
        case STACK_OVERFLOW:
            return "Stack Overflow";
        case UNKNOWN_SYMBOL:
            return "Unknown Symbol";
        case UNEXPECTED_SYMBOL:
            return "Unexpected Symbol";
        case DIVISION_BY_ZERO:
            return "Division by Zero";
    }
}

Bool is_number(const char *s) {
    size_t i, n, point_counter;
    
    point_counter = 0;
    n = strlen(s);

    i = 0;

    if (s[0] == '-') {
        i = 1;
    }

    if (n <= 1 && i > 0) {
        return False;
    }

    for (; i < n; ++i) {
        if (s[i] == '.') {
            point_counter++;
        }

        if (point_counter > 1) {
            return False;
        }

        if (!isdigit(s[i]) && s[i] != '.') {
            return False;
        }
    }

    return True;
}

double factorial(double n) {
    double res;
    size_t i;

    res = 1;


    for (i = (size_t)n; i > 0; --i) {
        res *= i;
    }

    return res;
}
Op str_to_op(const char *s) {
    if (!(strcmp("+", s) && strcmp("add", s))) {
        return OP_ADD;
    } else if (!(strcmp("-", s) && strcmp("sub", s))) {
        return OP_SUB;
    } else if (!(strcmp("*", s) && strcmp("mul", s))) {
        return OP_MUL;
    } else if (!(strcmp("/", s) && strcmp("div", s))) {
        return OP_DIV;
    } else if (!(strcmp("mod", s) && strcmp("%", s))) {
        return OP_MOD;
    } else if (!(strcmp("pow", s) && strcmp("^", s))) {
        return OP_POW;
    } else if (!(strcmp("sqrt", s) && strcmp("v/", s))) {
        return OP_SQRT;
    } else if (!strcmp("ln", s)) {
        return OP_LN;
    } else if (!strcmp("log", s)) {
        return OP_LOG;
    } else if (!strcmp("sin", s)) {
        return OP_SIN;
    } else if (!strcmp("cos", s)) {
        return OP_COS;
    } else if (!strcmp("tan", s)) {
        return OP_TAN;
    } else if (!strcmp("atan", s)) {
        return OP_ATAN;
    } else if (!(strcmp("factorial", s) && strcmp("!", s))) {
        return OP_FACTORIAL;
    }

    return OP_NONE;
}

Constant str_to_const(const char *s) {
    if (!strcmp("pi", s)) {
        return CONST_PI;
    } else if (!strcmp("euler", s)) {
        return CONST_E;
    } else if (!strcmp("inf", s)) {
        return CONST_INF;
    }

    return CONST_NULL;
}
Command str_to_cmd(const char *s) {
    if (!strcmp(s, "ls")) {
        return CMD_LS;
    } else if (!strcmp(s, "p")) {
        return CMD_PRINT;
    } else if (!(strcmp(s, "s") && strcmp(s, "swap"))) {
        return CMD_SWAP;
    } else if (!strcmp(s, "P")) {
        return CMD_PRINT_AND_POP;
    } else if (!(strcmp(s, "dup") && strcmp(s, "d"))) {
        return CMD_DUP;
    } else if (!strcmp(s, "cls")) {
        return CMD_CLEAR_SCREEN;
    } else if (!strcmp(s, "c")) {
        return CMD_CLEAR_STACK;
    } else if (!strcmp(s, "ops")) {
        return CMD_PRINT_OPS;
    } else if (!(strcmp(s, "h") && strcmp(s, "help"))) {
        return CMD_PRINT_HELP;
    } else if (!(strcmp(s, "q") && strcmp(s, "quit"))) {
        return CMD_QUIT;
    } else if (!(strcmp(s, "e") && strcmp(s, "eval"))) {
        return CMD_EVAL_TOP;
    }

    return CMD_NONE;
}

Status symbol_push(Symbol s) {
    if (stack.top + 1 == MAX_STACK_SIZE) {
        return STACK_OVERFLOW;
    }

    stack.data[stack.top++] = s;
    return SUCCESS;
}

Symbol symbol_pop(void) {
    if (stack.top < 1) {
        return stack.data[stack.top];
    }
    return stack.data[--stack.top];
}

Symbol symbol_get(void) {
    if (stack.top < 1) {
        return stack.data[stack.top];
    }
    return stack.data[stack.top - 1];
}

Symbol symbol_print(Symbol s) {
    printf("\x1b[0;39m");
    switch (s.kind) {
        case S_UNDEFINED:
            printf("Undefined symbol\n");
            break;
        case S_OP:
            printf("Symbol: ");
            switch (s.as.op) {
                case OP_NONE:
                    printf("NULL\n");
                    break;
                case OP_ADD:
                    printf("+\n");
                    break;
                case OP_SUB:
                    printf("-\n");
                    break;
                case OP_MUL:
                    printf("*\n");
                    break;
                case OP_DIV:
                    printf("/\n");
                    break;
                case OP_MOD:
                    printf("mod\n");
                    break;
                case OP_POW:
                    printf("pow\n");
                    break;
                case OP_FACTORIAL:
                    printf("factorial\n");
                    break;
                case OP_SQRT:
                    printf("mod\n");
                    break;
                case OP_LN:
                    printf("ln\n");
                    break;
                case OP_LOG:
                    printf("log\n");
                    break;
                case OP_SIN:
                    printf("sin\n");
                    break;
                case OP_COS:
                    printf("cos\n");
                    break;
                case OP_TAN:
                    printf("tan\n");
                    break;
                case OP_ATAN:
                    printf("atan\n");
                    break;
                default: break;
            }
            printf("\n");
            break;
        case S_NUMBER: 
            printf("%lf\n", s.as.num);
            break;
    }
    return s;
}

Symbol symbol_print_top(Bool pop) {
    if (pop) {
        return symbol_print(symbol_pop());
    } else {
        return symbol_print(stack.data[stack.top-1]);
    }
}

void stack_print(void) {
    size_t i;

    for (i = 0; i < stack.top; ++i) {
        symbol_print(stack.data[i]);
    }
}

size_t op_get_input_amount(Op op) {
    return op_input_amount[op];
}

Status stack_eval_top(double *res, Bool pop) {
    Symbol s;
    Symbol a, b, ns;
    Op op;

    if (pop) {
        s = symbol_pop();
    } else {
        s = stack.data[stack.top-1];
    }

    if (s.kind != S_OP) {
        return UNEXPECTED_SYMBOL;
    }
    

    op = s.as.op;

    ns.kind = S_NUMBER;

    if (pop) {
        a = symbol_pop();
    } else {
        a = stack.data[stack.top-2];
    }

    if (a.kind != S_NUMBER) {
        return UNEXPECTED_SYMBOL;
    }

    if (op_get_input_amount(op) == 2) {
        if (pop) {
            b = symbol_pop();
        } else {
            b = stack.data[stack.top-3];
        }

        if (b.kind != S_NUMBER) {
            return UNEXPECTED_SYMBOL;
        }
    } 

    switch (op) {
        case OP_ADD: {
            ns.as.num = b.as.num + a.as.num;
            break;
        }

        case OP_SUB: {
            ns.as.num = b.as.num - a.as.num;
            break;
        }

        case OP_MUL: {
            ns.as.num = b.as.num * a.as.num;
            break;
        }

        case OP_DIV: {
            if (a.as.num == 0) {
                return DIVISION_BY_ZERO;
            }

            ns.as.num = b.as.num / a.as.num;
            break;
        }

        case OP_MOD: {
            if (a.as.num == 0) {
                return DIVISION_BY_ZERO;
            }

            ns.as.num = (int)b.as.num % (int)a.as.num;
            break;
        }

        case OP_POW: {
            ns.as.num = pow(b.as.num, a.as.num);
            break;
        }

        case OP_FACTORIAL: {
            ns.as.num = factorial(a.as.num);
            break;
        }

        case OP_SQRT: {
            ns.as.num = sqrt(a.as.num);
            break;
        }

        case OP_LN: {
            ns.as.num = log(a.as.num);
            break;
        }

        case OP_LOG: {
            ns.as.num = log10(a.as.num);
            break;
        }

        case OP_SIN: {
            ns.as.num = sin(a.as.num);
            break;
        }

        case OP_COS: {
            ns.as.num = cos(a.as.num);
            break;
        }

        case OP_TAN: {
            ns.as.num = tan(a.as.num);
            break;
        }

        case OP_ATAN: {
            ns.as.num = atan(a.as.num);
            break;
        }
        default: break;
    }

    if (res) {
        *res = ns.as.num;
    }

    symbol_push(ns);
    return SUCCESS;
}

Status cmd_eval(Command cmd) {
    switch (cmd) {
        case CMD_LS: {
            stack_print();

            if (stack.top < 1) {
                printf("Stack empty\n");
            }
        } break;

        case CMD_DUP: {
            Symbol s;

            s = symbol_get();

            if (s.kind != S_UNDEFINED) {
                symbol_push(symbol_get());
            }
        } break;

        case CMD_SWAP: {
            Symbol a, b;

            if (stack.top < 2) {
                printf("Stack to small to swap\n");
                break;
            }

            a = symbol_pop();
            b = symbol_pop();

            symbol_push(a);
            symbol_push(b);
        } break;

        case CMD_PRINT: {
            symbol_print_top(False);
        } break;

        case CMD_PRINT_AND_POP: {
            symbol_print_top(True);
        } break;

        case CMD_PRINT_OPS: {
            print_ops();
        } break;

        case CMD_PRINT_HELP: {
            print_help();
        } break;

        case CMD_EVAL_TOP: {
            stack_eval_top(NULL, False);
        } break;

        case CMD_CLEAR_SCREEN: {
            printf("\x1b[H\x1b[J");
        } break;

        case CMD_CLEAR_STACK: {
            memset(stack.data, 0, MAX_STACK_SIZE);
            stack.top = 0;
        } break;

        case CMD_QUIT: {
            return QUIT;
        }
        default: break;
    }
    return SUCCESS;
}

Status eval_string(const char *sc) {
    char *sc_cpy;
    char *token;

    sc_cpy = (char *) malloc(sizeof(char) * (strlen(sc) + 1));

    if (!sc_cpy) {
        return FAILED_MALLOC;
    }

    strcpy(sc_cpy, sc);
    token = strtok(sc_cpy, " \n\t\v");

    while (token != NULL) {
        if (!token) {
            break;
        }

        if (is_number(token)) {
            Symbol s;

            s.kind = S_NUMBER;
            s.as.num = atof(token);

            symbol_push(s);
        } else if (str_to_cmd(token) != CMD_NONE) {
            Command cmd;
            Status status;

            cmd = str_to_cmd(token);
            status = cmd_eval(cmd);

            if (status != SUCCESS) {
                free(sc_cpy);
                return status;
            }
        } else if (!(strcmp(token, "q") && strcmp(token, "quit") && strcmp(token, "exit"))) {
            free(sc_cpy);
            return QUIT;
        } else if (*token == EOF) {
            free(sc_cpy);
            return QUIT;
        } else if (str_to_const(token) != CONST_NULL) {
            Symbol s;

            s.kind = S_NUMBER;
            s.as.num = constants[str_to_const(token)];

            symbol_push(s);
        } else {
            Symbol s;
            Status status;

            s.kind = S_OP;
            s.as.op = str_to_op(token);

            if (s.as.op == S_UNDEFINED) {
                free(sc_cpy);
                return UNKNOWN_SYMBOL;
            }

            symbol_push(s);
            status = stack_eval_top(NULL, False);

            if (status != SUCCESS) {
                free(sc_cpy);
                return status;
            }
        }

        token = trim_str(strtok(NULL, " \n\t\v"));
    }

    free(sc_cpy);
    return SUCCESS;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Type 'h' for help\n");
        while (1) {
            Status status;
            char *line;

            printf("\x1b[0;32m");
            printf("[%zu]-$ ", stack.top);
            fflush(stdout);

            line = read_line(NULL);
            status = eval_string(line);

            free(line);

            if (status == SUCCESS) {
                continue;
            }

            if (status == QUIT) {
                printf("\x1b[0;31mQuitting...\n");
                break;
            } else if (status == UNKNOWN_SYMBOL || status == UNEXPECTED_SYMBOL || status == DIVISION_BY_ZERO) {
                printf("\x1b[0;31mERROR: %s\n", err_to_string(status));
            } else {
                printf("\x1b[0;31mERROR: %s\n", err_to_string(status));
                return EXIT_FAILURE;
            }

        }
        printf("\x1b[0;39m");
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
    } else if (strcmp(argv[1], "--file") == 0) {
        char *sc;
        Status status;
        if (argc != 3) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        sc = file_to_str(argv[2]);

        if (!sc) {
            fprintf(stderr, "\x1b[0;31mERROR: Unable to open file: %m");
            return EXIT_FAILURE;
        }

        status = eval_string(sc);

        free(sc);

        if (status != SUCCESS && status != QUIT) {
            printf("\x1b[0;31mERROR: %s", err_to_string(status));
            return EXIT_FAILURE;
        }
    }
    printf("\x1b[0;39m");
    return EXIT_SUCCESS;
}
