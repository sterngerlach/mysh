
/* print-args.c */

#include <stdio.h>

int main(int argc, char** argv)
{
    while (*argv)
        printf("%s\n", *argv++);

    return 0;
}

