#include <stdio.h>

#include "../shell.h"

void builtin_cat(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "cat: missing file operand\n");
        return;
    }

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        FILE *fp = fopen(cmd->argv[i], "r");
        if (!fp)
        {
            perror(cmd->argv[i]);
            continue;
        }

        int ch;
        while ((ch = fgetc(fp)) != EOF)
        {
            putchar(ch);
        }

        fclose(fp);
    }
}