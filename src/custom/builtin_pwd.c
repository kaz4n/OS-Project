#include <stdio.h>
#include <unistd.h>

#include "../shell.h"

void builtin_pwd(void)
{
    char cwd[1024];
    const char *session_cwd = shell_session_cwd();

    if (session_cwd != NULL)
    {
        printf("%s\n", session_cwd);
        return;
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s\n", cwd);
    else
        perror("pwd");
}
