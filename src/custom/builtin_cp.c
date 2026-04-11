#include <stdio.h>

#include "../shell.h"

void builtin_cp(Command *cmd)
{
    if (cmd->argv[1] == NULL || cmd->argv[2] == NULL || cmd->argv[3] != NULL)
    {
        fprintf(stderr, "cp: usage: cp_new source destination\n");
        return;
    }

    FILE *src = fopen(cmd->argv[1], "rb");
    if (!src)
    {
        perror(cmd->argv[1]);
        return;
    }

    FILE *dst = fopen(cmd->argv[2], "wb");
    if (!dst)
    {
        perror(cmd->argv[2]);
        fclose(src);
        return;
    }

    unsigned char buffer[4096];
    size_t nread;
    while ((nread = fread(buffer, 1, sizeof(buffer), src)) > 0)
    {
        if (fwrite(buffer, 1, nread, dst) != nread)
        {
            perror("cp");
            break;
        }
    }

    fclose(src);
    fclose(dst);
}