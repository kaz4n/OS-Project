#include <stdio.h>

#include "../shell.h"

void builtin_clear(void)
{
    /* ANSI escape: move cursor home, then clear screen */
    printf("\033[H\033[J");
    fflush(stdout);
}
