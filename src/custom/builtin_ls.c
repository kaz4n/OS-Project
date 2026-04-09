#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "../shell.h"

static void print_permissions(mode_t mode)
{
    printf("%c", S_ISDIR(mode)    ? 'd' : '-');
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

void builtin_ls(Command *cmd)
{
    int long_format = 0;
    const char *path = ".";

    for (int i = 1; cmd->argv[i] != NULL; i++)
    {
        if (strcmp(cmd->argv[i], "-l") == 0)
            long_format = 1;
        else
            path = cmd->argv[i];
    }

    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("ls");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
        {
            struct stat st;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            if (stat(full_path, &st) == -1)
            {
                perror("stat");
                continue;
            }

            print_permissions(st.st_mode);

            printf(" %ld", (long)st.st_nlink);

            struct passwd *pw = getpwuid(st.st_uid);
            printf(" %s", pw ? pw->pw_name : "?");

            struct group *gr = getgrgid(st.st_gid);
            printf(" %s", gr ? gr->gr_name : "?");

            printf(" %6lld", (long long)st.st_size);

            char timebuf[20];
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M",
                     localtime(&st.st_mtime));
            printf(" %s", timebuf);

            printf(" %s\n", entry->d_name);
        }
        else
        {
            printf("%s  ", entry->d_name);
        }
    }

    if (!long_format)
        printf("\n");

    closedir(dir);
}
