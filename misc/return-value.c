
/* return-value.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
    int ret;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Return value>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ret = atoi(argv[1]);
    printf("pid: %d, ppid: %d, pgid: %d, return value: %d\n",
           getpid(), getppid(), getpgrp(), ret);

    return ret;
}

