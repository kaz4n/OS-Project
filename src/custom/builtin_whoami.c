#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include "../shell.h"

void builtin_whoami(void)
{
    struct passwd *pw = getpwuid(getuid());

    if (pw)
        printf("%s\n", pw->pw_name);
    else
        perror("whoami");
}
