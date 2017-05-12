// Created Time:    2017-05-11 20:26:26
// Modified Time:   2017-05-12 09:31:35

#include <stdio.h>
#include <stdlib.h>
typedef int array[4];

int main(int argc, char *argv[])
{
    unsigned int *p;
    array x;
    *x = 1;
    *(x+1) = 2;
    p = (unsigned int*) malloc(16);

    struct test
    {
        unsigned int a;
        unsigned int b;
        struct test *next;
    } *data;
    
    data = (struct test *)p;
    data->a = 1;
    data->b = 2;
    data->next = data;
    printf("p:%d %d %p\ndata:%d %d %p\n", *p, *(p+1),(unsigned int*) *(p+2), data->a,data->b, data->next);
    printf("x:%d %d %p\n", *x, *(x+1), x);
    return 0;
}

