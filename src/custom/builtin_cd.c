#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "../shell.h"

static int resolve_target_path(const ShellSession *session, const char *path, char *resolved, size_t resolved_size)
{
    char candidate[PATH_MAX];

    if (path == NULL || resolved == NULL || resolved_size == 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (path[0] == '/')
    {
        strncpy(candidate, path, sizeof(candidate) - 1);
        candidate[sizeof(candidate) - 1] = '\0';
    }
    else if (session != NULL && session->cwd[0] != '\0')
    {
        if (snprintf(candidate, sizeof(candidate), "%s/%s", session->cwd, path) >= (int)sizeof(candidate))
        {
            errno = ENAMETOOLONG;
            return -1;
        }
    }
    else
    {
        strncpy(candidate, path, sizeof(candidate) - 1);
        candidate[sizeof(candidate) - 1] = '\0';
    }

    if (realpath(candidate, resolved) == NULL)
    {
        return -1;
    }

    return 0;
}

void handle_cd_session(Command *cmd, ShellSession *session)
{
    const char *path;
    char new_dir[PATH_MAX];

    if (session == NULL)
    {
        handle_cd(cmd);
        return;
    }

    if (cmd->argv[1] == NULL)
    {
        path = getenv("HOME");
        if (path == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }
    else if (cmd->argv[2] != NULL)
    {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    else if (strcmp(cmd->argv[1], "-") == 0)
    {
        if (session->previous_cwd[0] == '\0')
        {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        path = session->previous_cwd;
    }
    else
    {
        path = cmd->argv[1];
    }

    if (resolve_target_path(session, path, new_dir, sizeof(new_dir)) != 0)
    {
        perror("cd");
        return;
    }

    strncpy(session->previous_cwd, session->cwd, sizeof(session->previous_cwd) - 1);
    session->previous_cwd[sizeof(session->previous_cwd) - 1] = '\0';
    strncpy(session->cwd, new_dir, sizeof(session->cwd) - 1);
    session->cwd[sizeof(session->cwd) - 1] = '\0';

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-") == 0)
    {
        printf("%s\n", session->cwd);
    }
}

void handle_cd(Command *cmd)
{
    ShellSession *session = shell_session_current();

    if (session != NULL)
    {
        handle_cd_session(cmd, session);
        return;
    }

    static char previous_dir[PATH_MAX] = "";
    char current_dir[PATH_MAX];
    const char *path;

    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("cd");
        return;
    }

    if (cmd->argv[1] == NULL)
    {
        path = getenv("HOME");
        if (path == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }
    else if (cmd->argv[2] != NULL)
    {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    else if (strcmp(cmd->argv[1], "-") == 0)
    {
        if (previous_dir[0] == '\0')
        {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        path = previous_dir;
    }
    else
    {
        path = cmd->argv[1];
    }

    if (chdir(path) != 0)
    {
        perror("cd");
        return;
    }

    strncpy(previous_dir, current_dir, sizeof(previous_dir) - 1);
    previous_dir[sizeof(previous_dir) - 1] = '\0';

    if (cmd->argv[1] != NULL && strcmp(cmd->argv[1], "-") == 0)
    {
        char new_dir[PATH_MAX];
        if (getcwd(new_dir, sizeof(new_dir)) != NULL)
        {
            printf("%s\n", new_dir);
        }
    }
}
