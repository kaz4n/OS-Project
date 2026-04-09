#include <ctype.h>
#include <string.h>

#include "shell.h"

void trim_inplace(char *s)
{
    char *start = s;

    while (*start && isspace((unsigned char)*start))
    {
        start++;
    }

    if (start != s)
    {
        memmove(s, start, strlen(start) + 1);
    }

    while (*s != '\0')
    {
        s++;
    }

    while (s > start && isspace((unsigned char)*(s - 1)))
    {
        s--;
        *s = '\0';
    }
}

int is_valid_pipe_syntax(const char *line)
{
    int saw_token_in_segment = 0;
    int cmd_count = 1;

    if (line == NULL)
    {
        return 0;
    }

    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == '|')
        {
            if (!saw_token_in_segment)
            {
                return 0;
            }

            saw_token_in_segment = 0;
            cmd_count++;
            if (cmd_count > MAX_CMDS)
            {
                return 0;
            }
        }
        else if (!isspace((unsigned char)line[i]))
        {
            saw_token_in_segment = 1;
        }
    }

    if (!saw_token_in_segment)
    {
        return 0;
    }

    return 1;
}

int parse_input(char *input_line, Pipeline *pipeline)
{
    char *segments[MAX_CMDS];
    int segment_count = 0;
    char *cursor;

    if (input_line == NULL || pipeline == NULL)
    {
        return 0;
    }

    pipeline->num_commands = 0;
    cursor = input_line;

    while (cursor != NULL)
    {
        char *pipe_pos = strchr(cursor, '|');

        if (segment_count >= MAX_CMDS)
        {
            return 0;
        }

        if (pipe_pos != NULL)
        {
            *pipe_pos = '\0';
        }

        trim_inplace(cursor);
        if (cursor[0] == '\0')
        {
            return 0;
        }

        segments[segment_count++] = cursor;
        cursor = (pipe_pos == NULL) ? NULL : pipe_pos + 1;
    }

    for (int i = 0; i < segment_count; i++)
    {
        char *p;
        int argc = 0;
        Command *cmd = &pipeline->commands[pipeline->num_commands];

        p = segments[i];

        while (*p != '\0')
        {
            char *token_start;
            char *write;
            int in_quote = 0;
            char quote_char = '\0';
            int ended_on_space;

            while (*p != '\0' && isspace((unsigned char)*p))
            {
                p++;
            }

            if (*p == '\0')
            {
                break;
            }

            if (argc >= MAX_ARGS - 1)
            {
                return 0;
            }

            token_start = p;
            write = p;

            while (*p != '\0')
            {
                char ch = *p;

                if (!in_quote && isspace((unsigned char)ch))
                {
                    break;
                }

                if (ch == '\\' && p[1] != '\0')
                {
                    p++;
                    *write++ = *p++;
                    continue;
                }

                if (ch == '"' || ch == '\'')
                {
                    if (!in_quote)
                    {
                        in_quote = 1;
                        quote_char = ch;
                        p++;
                        continue;
                    }

                    if (ch == quote_char)
                    {
                        in_quote = 0;
                        quote_char = '\0';
                        p++;
                        continue;
                    }
                }

                *write++ = ch;
                p++;
            }

            if (in_quote)
            {
                return 0;
            }

            ended_on_space = (*p != '\0' && isspace((unsigned char)*p));
            *write = '\0';
            cmd->argv[argc++] = token_start;

            if (ended_on_space)
            {
                p++;
            }

            while (*p != '\0' && isspace((unsigned char)*p))
            {
                p++;
            }
        }

        if (argc == 0)
        {
            return 0;
        }

        cmd->argv[argc] = NULL;
        pipeline->num_commands++;
    }

    return (pipeline->num_commands > 0);
}