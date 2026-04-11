#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shell.h"

void builtin_tail(Command *cmd)
{
    int line_count = 10;
    int file_index = 1;

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-n") == 0)
    {
        if (cmd->argv[2] == NULL || cmd->argv[3] == NULL)
        {
            fprintf(stderr, "tail: usage: tail_new [-n N] file\n");
            return;
        }

        line_count = atoi(cmd->argv[2]);
        if (line_count <= 0)
        {
            fprintf(stderr, "tail: invalid line count\n");
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

    char **ring = (char **)calloc((size_t)line_count, sizeof(char *));
    if (!ring)
    {
        fclose(fp);
        perror("tail");
        return;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    int total = 0;

    while ((nread = getline(&line, &cap, fp)) != -1)
    {
        (void)nread;
        int slot = total % line_count;
        free(ring[slot]);
        ring[slot] = strdup(line);
        if (!ring[slot])
        {
            perror("tail");
            free(line);
            for (int i = 0; i < line_count; i++)
            {
                free(ring[i]);
            }
            free(ring);
            fclose(fp);
            return;
        }
        total++;
    }

    int start = (total > line_count) ? (total % line_count) : 0;
    int to_print = (total < line_count) ? total : line_count;

    for (int i = 0; i < to_print; i++)
    {
        int idx = (start + i) % line_count;
        if (ring[idx])
        {
            fputs(ring[idx], stdout);
        }
    }

    free(line);
    for (int i = 0; i < line_count; i++)
    {
        free(ring[i]);
    }
    free(ring);
    fclose(fp);
}