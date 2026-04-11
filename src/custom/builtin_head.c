#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shell.h"

void builtin_head(Command *cmd)
{
    int line_count = 10;
    int file_index = 1;

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-n") == 0)
    {
        if (cmd->argv[2] == NULL || cmd->argv[3] == NULL)
        {
            fprintf(stderr, "head: usage: head_new [-n N] file\n");
            return;
        }

        line_count = atoi(cmd->argv[2]);
        if (line_count < 0)
        {
            fprintf(stderr, "head: invalid line count\n");
            return;
        }
        file_index = 3;
    }

    FILE *fp = fopen(cmd->argv[file_index], "r");
    if (!fp)
    {
        perror(cmd->argv[file_index]);
        return;
    }

    char buf[1024];
    int printed = 0;
    while (printed < line_count && fgets(buf, sizeof(buf), fp) != NULL)
    {
        fputs(buf, stdout);
        printed++;
    }

    fclose(fp);
}