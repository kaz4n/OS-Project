#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello from my custom program!\n");
    printf("I received %d argument(s):\n", argc - 1);
    for (int i = 1; i < argc; i++) {
        printf("  arg[%d] = %s\n", i, argv[i]);
    }
    return 0;
}