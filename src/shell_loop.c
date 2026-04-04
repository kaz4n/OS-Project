#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

void start_shell_loop(void)
{
    char *input_line;

    while (1)
    {
        print_shell_name();
        input_line = read_line_from_user();

        if (!input_line)
        {
            printf("\nExiting...\n");
            exit(0);
        }

        remove_newline(input_line);

        if (input_line[0] == '\0' || is_empty_line(input_line))
        {
            free(input_line);
            continue;
        }

        process_input(input_line);
        free(input_line);
    }
}

void print_shell_name(void)
{
    printf("myshell> ");
}

char *read_line_from_user(void)
{
    char *line = (char *)malloc(MAX_LINE);

    if (!line)
    {
        return NULL;
    }

    if (!fgets(line, MAX_LINE, stdin))
    {
        free(line);
        return NULL;
    }

    return line;
}

void remove_newline(char *line)
{
    int i = 0;

    while (line[i] != '\0')
    {
        if (line[i] == '\n')
        {
            line[i] = '\0';
            break;
        }
        i++;
    }
}

int is_empty_line(char *line)
{
    int i = 0;

    while (line[i] != '\0')
    {
        if (line[i] != ' ' && line[i] != '\t')
        {
            return 0;
        }
        i++;
    }

    return 1;
}

void process_input(char *input_line)
{
    Pipeline pipeline;

    if (!is_valid_pipe_syntax(input_line))
    {
        printf("Syntax error: invalid pipe usage\n");
        return;
    }

    if (!parse_input(input_line, &pipeline))
    {
        printf("Parse error\n");
        return;
    }

    execute_pipeline(&pipeline);
}