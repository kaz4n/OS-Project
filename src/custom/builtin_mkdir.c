#include <stdio.h>
#include <sys/stat.h>

#include "../shell.h"

void builtin_mkdir(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "mkdir: missing operand\n");
        return;
    }

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        /* 0755 = rwxr-xr-x */
        if (mkdir(cmd->argv[i], 0755) != 0)
            perror(cmd->argv[i]);
    }
}
