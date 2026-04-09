#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../shell.h"

void builtin_rm_recursive(const char *path)
{
    struct stat st;

    if (lstat(path, &st) != 0)
    {
        perror(path);
        return;
    }

    if (S_ISDIR(st.st_mode))
    {
        DIR *dir = opendir(path);
        if (!dir)
        {
            perror(path);
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            char child[1024];
            snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            builtin_rm_recursive(child);
        }

        closedir(dir);

        if (rmdir(path) != 0)
            perror(path);
    }
    else
    {
        if (unlink(path) != 0)
            perror(path);
    }
}

void builtin_rm(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "rm: missing operand\n");
        return;
    }

    int recursive = 0;
    int start = 1;

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-r") == 0)
    {
        recursive = 1;
        start = 2;
    }

    for (int i = start; cmd->argv[i] != NULL; i++)
    {
        if (recursive)
            builtin_rm_recursive(cmd->argv[i]);
        else if (unlink(cmd->argv[i]) != 0)
            perror(cmd->argv[i]);
    }
}
