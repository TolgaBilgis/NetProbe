#include <stdio.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s server [options]\n", program);
    fprintf(stderr, "  %s client <host> [options]\n", program);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    printf("NetProbe: command '%s' is not implemented yet.\n", argv[1]);
    return 0;
}
