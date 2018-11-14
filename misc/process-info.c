
/* process-info.c */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
    printf("pid: %d, ppid: %d, pgid: %d\n",
           getpid(), getppid(), getpgrp());
    pause();

    return 0;
}

