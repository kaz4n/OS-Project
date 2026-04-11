#include <stdio.h>

#include "../shell.h"

void builtin_mv(Command *cmd)
{
    if (cmd->argv[1] == NULL || cmd->argv[2] == NULL || cmd->argv[3] != NULL)
    {
        fprintf(stderr, "mv: usage: mv_new source destination\n");
        return;
    }

    if (rename(cmd->argv[1], cmd->argv[2]) != 0)
    {
        perror("mv");
    }
}