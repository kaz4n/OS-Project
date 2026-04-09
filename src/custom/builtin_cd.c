#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../shell.h"

void handle_cd(Command *cmd)
{
    const char *path;

    if (cmd->argv[1] == NULL)
    {
        path = getenv("HOME");
        if (path == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }
    else if (cmd->argv[2] != NULL)
    {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    else
    {
        path = cmd->argv[1];
    }

    if (chdir(path) != 0)
        perror("cd");
}
