#include <stdio.h>
#include <string.h>
#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"

void print_rval(rval r)
{
    if (r.is_num) {
        printf("number: %ld", r.num);
    }
    if (r.is_str) {
        printf("str: %s", r.str);
    }
    if (r.is_col) {
        printf("column: %zu", r.col);
    }
}

void print_indent(size_t indent)
{
    for (size_t i = 0; i < indent; i++) {
        printf("    ");
    }
}

void print_condition(compound* c, size_t indent)
{
    switch (c->oper) {
    case OPER_SIMPLE:
        print_indent(indent);
        if (c->simple.lval.is_col) {
            printf("column %zu ", c->simple.lval.col);
        }
        else if (c->simple.lval.is_special) {
            switch (c->simple.lval.special) {
            case SPECIAL_NUMCOLS:
                printf("<number of columns> ");
                break;
            case SPECIAL_ROWNUM:
                printf("<row number> ");
                break;
            default:
                printf("<unknown special value!!> ");
                break;
            }
        }
        else {
            printf("<wtf value!!!!!!!> ");
        }

        switch (c->simple.oper) {
        case TOK_EQ:
            printf("=");
            break;
        case TOK_GT:
            printf(">");
            break;
        case TOK_LT:
            printf("<");
            break;
        case TOK_GTE:
            printf(">=");
            break;
        case TOK_LTE:
            printf("<=");
            break;
        }
        printf(" ");
        print_rval(c->simple.rval);
        printf("\n");
        break;
    case OPER_NOT:
        print_indent(indent);
        printf("NOT:\n");
        print_condition(c->left, indent + 1);
        printf("\n");
        break;
    case OPER_AND:
        print_indent(indent);
        printf("AND L:\n");
        print_condition(c->left, indent + 1);
        print_indent(indent);
        printf("AND R:\n");
        print_condition(c->right, indent + 1);
        printf("\n");
        break;
    case OPER_OR:
        print_indent(indent);
        printf("OR L:\n");
        print_condition(c->left, indent + 1);
        print_indent(indent);
        printf("OR R:\n");
        print_condition(c->right, indent + 1);
        printf("\n");
        break;
    }
}
