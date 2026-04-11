#include <stdio.h>
#ifdef _WIN32
#include <stdlib.h>
#else
#include <sys/utsname.h>
#endif

#include "../shell.h"

void builtin_uname(void)
{
#ifdef _WIN32
    const char *os = getenv("OS");
    const char *name = getenv("COMPUTERNAME");
    printf("%s %s\n", os ? os : "Windows", name ? name : "unknown-host");
#else
    struct utsname info;

    if (uname(&info) != 0)
    {
        perror("uname");
        return;
    }

    printf("%s %s %s %s %s\n",
           info.sysname,
           info.nodename,
           info.release,
           info.version,
           info.machine);
#endif
}
