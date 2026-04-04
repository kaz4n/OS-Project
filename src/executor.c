#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "shell.h"

void execute_pipeline(Pipeline *pipeline)
{
#ifdef _WIN32
    (void)pipeline;
    printf("Pipeline execution is implemented for POSIX systems (Linux/macOS).\n");
    printf("Use WSL/Linux to run fork/pipe/execvp.\n");
    return;
#else
    int n = pipeline->num_commands;
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < n - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            return;
        }
    }

    for (int i = 0; i < n; i++)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            perror("fork");
            return;
        }

        if (pids[i] == 0)
        {
            if (i > 0)
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("dup2 stdin");
                    exit(1);
                }
            }

            if (i < n - 1)
            {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                {
                    perror("dup2 stdout");
                    exit(1);
                }
            }

            for (int k = 0; k < n - 1; k++)
            {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            execvp(pipeline->commands[i].argv[0], pipeline->commands[i].argv);
            perror(pipeline->commands[i].argv[0]);
            exit(127);
        }
    }

    for (int i = 0; i < n - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < n; i++)
    {
        waitpid(pids[i], NULL, 0);
    }
#endif
}