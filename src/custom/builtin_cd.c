#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "../shell.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void handle_cd(Command *cmd)
{
    static char previous_dir[PATH_MAX] = "";
    char current_dir[PATH_MAX];
    const char *path;

    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("cd");
        return;
    }

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
    else if (strcmp(cmd->argv[1], "-") == 0)
    {
        if (previous_dir[0] == '\0')
        {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        path = previous_dir;
    }
    else
    {
        path = cmd->argv[1];
    }

    if (chdir(path) != 0)
    {
        perror("cd");
        return;
    }

    strncpy(previous_dir, current_dir, sizeof(previous_dir) - 1);
    previous_dir[sizeof(previous_dir) - 1] = '\0';

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-") == 0)
    {
        char new_dir[PATH_MAX];
        if (getcwd(new_dir, sizeof(new_dir)) != NULL)
        {
            printf("%s\n", new_dir);
        }
    }
}
