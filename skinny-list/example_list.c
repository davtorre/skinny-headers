#define SKN_LIST_IMPLEMENTATION
#include "skn_list.h"

int main(void)
{
    FILE *fp = fopen("sentinel.log", "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fprintf(fp, "iter;value_int;value_float;message\n");

    int   v_int   = 0;
    float v_float = 0.0f;
    char  msg[32] = "start";

    /* Build the row once. */
    SknListNode *row = NULL;
    lstv_push_front(&row, lstv_create_node(LSTV_CHAR,   msg));
    lstv_push_front(&row, lstv_create_node(LSTV_FLOAT, &v_float));
    lstv_push_front(&row, lstv_create_node(LSTV_INT,   &v_int));

    for (int i = 0; i < 10; ++i) {
        /* Mutate the referenced variables — the row itself never changes. */
        v_int   = i;
        v_float = i * 0.5f;
        snprintf(msg, sizeof(msg), "step_%d", i);

        fprintf(fp, "%d;", i);
        lstv_print_list(fp, row, ";", 1);
    }

    lstv_free_list(row);
    fclose(fp);
    return 0;
}
