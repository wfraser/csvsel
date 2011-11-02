#include <stdio.h>
#include <string.h>
#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"

void print_val(val r)
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
    if (r.is_special) {
        printf("special: ");
        switch (r.special) {
        case SPECIAL_NUMCOLS:
            printf("<number of columns>");
            break;
        case SPECIAL_ROWNUM:
            printf("<row number>");
            break;
        default:
            printf("<unknown special value!!>");
            break;
        }
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
    if (NULL == c) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    switch (c->oper) {
    case OPER_SIMPLE:
        print_indent(indent);
        print_val(c->simple.left);
        printf(" ");

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
        print_val(c->simple.right);
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
