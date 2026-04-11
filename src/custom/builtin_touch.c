#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>

#include "../shell.h"

void builtin_touch(Command *cmd)
{
    if (cmd->argv[1] == NULL)
    {
        fprintf(stderr, "touch: missing file operand\n");
        return;
    }

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        const char *path = cmd->argv[i];

        if (access(path, F_OK) == 0)
        {
            struct utimbuf times;
            time_t now = time(NULL);
            times.actime = now;
            times.modtime = now;

            if (utime(path, &times) != 0)
            {
                perror(path);
            }
        }
        else
        {
            FILE *fp = fopen(path, "w");
            if (!fp)
            {
                perror(path);
                continue;
            }
            fclose(fp);
        }
    }
}