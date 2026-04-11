#include <stdio.h>
#include <unistd.h>

#include "../shell.h"

void builtin_rmdir(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "rmdir: missing operand\n");
        return;
    }

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        if (rmdir(cmd->argv[i]) != 0)
        {
            perror(cmd->argv[i]);
        }
    }
}