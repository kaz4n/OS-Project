#include <stdio.h>
#include <ctype.h>

#include "../shell.h"

static void wc_one_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        perror(path);
        return;
    }

    long lines = 0;
    long words = 0;
    long bytes = 0;
    int in_word = 0;
    int ch;

    while ((ch = fgetc(fp)) != EOF)
    {
        bytes++;
        if (ch == '\n')
        {
            lines++;
        }

        if (isspace((unsigned char)ch))
        {
            in_word = 0;
        }
        else if (!in_word)
        {
            words++;
            in_word = 1;
        }
    }

    printf("%ld %ld %ld %s\n", lines, words, bytes, path);
    fclose(fp);
}

void builtin_wc(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "wc: missing file operand\n");
        return;
    }

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        wc_one_file(cmd->argv[i]);
    }
}