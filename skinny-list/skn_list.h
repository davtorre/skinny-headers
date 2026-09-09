/*  skn_list.h — header-only, non-owning tagged linked list ("list viewer")
 *
 *  MODULE
 *    A lightweight, non-owning linked list of tagged nodes. Each node
 *    stores a type tag and a void* pointing at an external, caller-owned
 *    variable (int / float / double / char* string) — the data itself is
 *    never copied. Build a row of nodes once, then in a loop mutate the
 *    referenced variables and reprint the row without ever rebuilding the
 *    list. Intended for incremental/streaming tabular output — e.g. a live
 *    log row whose fields are pointers into your working variables.
 *
 *    Nodes do NOT own the data they point to: lstv_free_list only frees
 *    the nodes themselves, never the pointed-to variables.
 *
 *  USAGE
 *    #define SKN_LIST_IMPLEMENTATION
 *    #include "skn_list.h"
 *
 *  EXAMPLE
 *    int   v_int   = 0;
 *    float v_float = 0.0f;
 *
 *    SknListNode *row = NULL;
 *    lstv_push_front(&row, lstv_create_node(LSTV_FLOAT, &v_float));
 *    lstv_push_front(&row, lstv_create_node(LSTV_INT,   &v_int));
 *
 *    for (v_int = 0; v_int < 3; v_int++) {
 *        v_float = v_int * 0.5f;
 *        lstv_print_list(stdout, row, ", ", 1);
 *    }
 *    lstv_free_list(row);
 */

#ifndef SKN_LIST_H
#define SKN_LIST_H

#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Public types
 * ---------------------------------------------------------------------- */

typedef enum {
    LSTV_INT,
    LSTV_FLOAT,
    LSTV_DOUBLE,
    LSTV_CHAR
} SknListType;

typedef struct SknListNode {
    SknListType         type;
    void               *data;  /* address of the caller's variable; not owned, not copied */
    struct SknListNode *next;
} SknListNode;

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/* Create a node of the given type pointing at addr (not copied — addr must
 * outlive the node). Returns NULL on allocation failure. */
SknListNode *lstv_create_node(SknListType type, void *addr);

/* Push node onto the front of *head.
 * If node is NULL (e.g. lstv_create_node failed), this is a safe no-op:
 * *head is left unchanged and -1 is returned. Returns 0 on success. */
int lstv_push_front(SknListNode **head, SknListNode *node);

/* Free every node in the list. Only the nodes are freed — the variables
 * they point to are untouched, since the list never owned them. Safe to
 * call with NULL. */
void lstv_free_list(SknListNode *head);

/* Print the current values of every node in list to fp, in order,
 * separated by sep (may be NULL for no separator). If new_line is
 * non-zero, a trailing newline is printed after the last value.
 * Returns 1 on success, -1 if a node has a NULL data pointer or an
 * unrecognized type. */
int lstv_print_list(FILE *fp, SknListNode *list, const char *sep, int new_line);

/* -------------------------------------------------------------------------
 * Implementation
 * ---------------------------------------------------------------------- */

#ifdef SKN_LIST_IMPLEMENTATION

/* Print the current value of a single node to fp, formatted by its type.
 * Returns 1 on success, -1 if data is NULL or type is unrecognized. */
static int list__print_data(FILE *fp, SknListNode *node, int new_line)
{
    if (!node) return -1;
    if (!node->data) {
        fprintf(stderr, "list__print_data: data not set for node at %p\n", (void *)node);
        return -1;
    }

    switch (node->type) {
        case LSTV_INT:
            fprintf(fp, "%d", *(int *)node->data);
            break;
        case LSTV_FLOAT:
            fprintf(fp, "%f", *(float *)node->data);
            break;
        case LSTV_DOUBLE:
            fprintf(fp, "%lf", *(double *)node->data);
            break;
        case LSTV_CHAR:
            fprintf(fp, "%s", (char *)node->data); /* treated as a string */
            break;
        default:
            fprintf(stderr, "list__print_data: type %d not implemented yet.\n", node->type);
            return -1;
    }

    if (new_line) fprintf(fp, "\n");
    return 1;
}

SknListNode *lstv_create_node(SknListType type, void *addr)
{
    SknListNode *node = (SknListNode *)malloc(sizeof(SknListNode));
    if (!node) return NULL;
    node->type = type;
    node->data = addr;
    node->next = NULL;
    return node;
}

int lstv_push_front(SknListNode **head, SknListNode *node)
{
    if (!node) return -1;
    node->next = *head;
    *head = node;
    return 0;
}

void lstv_free_list(SknListNode *head)
{
    while (head) {
        SknListNode *tmp = head;
        head = head->next;
        free(tmp); /* only the node is freed, NOT the data it points to */
    }
}

int lstv_print_list(FILE *fp, SknListNode *list, const char *sep, int new_line)
{
    int first = 1;
    while (list) {
        if (!first && sep) {
            fprintf(fp, "%s", sep);
        }
        first = 0;

        int ret = list__print_data(fp, list, 0);
        if (ret != 1) {
            return -1;
        }

        list = list->next;
    }

    if (new_line) fprintf(fp, "\n");
    return 1;
}

#endif /* SKN_LIST_IMPLEMENTATION */
#endif /* SKN_LIST_H */
