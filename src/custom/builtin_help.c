#include <stdio.h>

#include "../shell.h"

void builtin_help(void)
{
    const char *builtins[] = {
        "cd_new",
        "pwd_new",
        "ls_new",
        "mkdir_new",
        "rm_new",
        "echo_new",
        "whoami_new",
        "clear_new",
        "exit_new",
        "help_new",
        "date_new",
        "uname_new",
        "touch_new",
        "cat_new",
        "head_new",
        "tail_new",
        "cp_new",
        "mv_new",
        "rmdir_new",
        "wc_new"
    };

    int count = (int)(sizeof(builtins) / sizeof(builtins[0]));

    printf("Built-in commands (%d):\n", count);
    for (int i = 0; i < count; i++)
    {
        printf("  %s\n", builtins[i]);
    }
}