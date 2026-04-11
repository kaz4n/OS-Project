#include <stdio.h>
#include <time.h>

#include "../shell.h"

void builtin_date(void)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (!tm_info)
    {
        perror("date");
        return;
    }

    char buf[128];
    if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info) == 0)
    {
        fprintf(stderr, "date: failed to format time\n");
        return;
    }

    printf("%s\n", buf);
}