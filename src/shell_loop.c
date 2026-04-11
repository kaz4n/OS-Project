#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
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
    char cwd[1024];
    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strcpy(hostname, "unknown");
    }
    hostname[sizeof(hostname) - 1] = '\0';

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s:%s> ", hostname, cwd);
    }
    else
    {
        printf("%s> ", hostname);
    }
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

// Old Code
//  void process_input(char *input_line)
//  {
//      Pipeline pipeline;

//     if (!is_valid_pipe_syntax(input_line))
//     {
//         printf("Syntax error: invalid pipe usage\n");
//         return;
//     }

//     if (!parse_input(input_line, &pipeline))
//     {
//         printf("Parse error\n");
//         return;
//     }

//     execute_pipeline(&pipeline);
// }

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

    /* Handle builtins — must run in parent process, not a fork */
    // if (pipeline.num_commands == 1)
    // {
    //     char *cmd = pipeline.commands[0].argv[0];

    //     if (strcmp(cmd, "cd") == 0)
    //     {
    //         handle_cd(&pipeline.commands[0]);
    //         return;
    //     }
    //     if (strcmp(cmd, "exit") == 0)
    //     {
    //         printf("Goodbye!\n");
    //         exit(0);
    //     }
    // }

        if (pipeline.num_commands == 1)
    {
        char *cmd = pipeline.commands[0].argv[0];
 
        if (strcmp(cmd, "cd") == 0)
        {
            handle_cd(&pipeline.commands[0]);
            return;
        }

        if (strcmp(cmd, "cd_new")     == 0) { handle_cd(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "pwd_new")    == 0) { builtin_pwd();                         return; }
        if (strcmp(cmd, "ls_new") == 0) { builtin_ls(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "mkdir_new")  == 0) { builtin_mkdir(&pipeline.commands[0]);  return; }
        if (strcmp(cmd, "rm_new")     == 0) { builtin_rm(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "echo_new")   == 0) { builtin_echo(&pipeline.commands[0]);   return; }
        if (strcmp(cmd, "whoami_new") == 0) { builtin_whoami();                      return; }
        if (strcmp(cmd, "clear_new")  == 0) { builtin_clear();                       return; }
        if (strcmp(cmd, "help_new")   == 0) { builtin_help();                        return; }
        if (strcmp(cmd, "date_new")   == 0) { builtin_date();                        return; }
        if (strcmp(cmd, "uname_new")  == 0) { builtin_uname();                       return; }
        if (strcmp(cmd, "touch_new")  == 0) { builtin_touch(&pipeline.commands[0]);  return; }
        if (strcmp(cmd, "cat_new")    == 0) { builtin_cat(&pipeline.commands[0]);    return; }
        if (strcmp(cmd, "head_new")   == 0) { builtin_head(&pipeline.commands[0]);   return; }
        if (strcmp(cmd, "tail_new")   == 0) { builtin_tail(&pipeline.commands[0]);   return; }
        if (strcmp(cmd, "cp_new")     == 0) { builtin_cp(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "mv_new")     == 0) { builtin_mv(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "rmdir_new")  == 0) { builtin_rmdir(&pipeline.commands[0]);  return; }
        if (strcmp(cmd, "wc_new")     == 0) { builtin_wc(&pipeline.commands[0]);     return; }
        if (strcmp(cmd, "exit_new")   == 0) { printf("Goodbye!\n"); exit(0); }
    }
    execute_pipeline(&pipeline);
}


