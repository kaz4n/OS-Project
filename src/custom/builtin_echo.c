#include <stdio.h>

#include "../shell.h"

void builtin_echo(Command *cmd)
{
    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        printf("%s", cmd->argv[i]);
        if (cmd->argv[i + 1] != NULL)
            printf(" ");
    }
    printf("\n");
}
