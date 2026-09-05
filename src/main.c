#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT 9000

typedef enum {
    MODE_NONE = 0,
    MODE_SERVER,
    MODE_CLIENT
} netprobe_mode;

typedef struct {
    netprobe_mode mode;
    const char *host;
    int port;
} netprobe_config;

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s server [--port PORT]\n", program);
    fprintf(stderr, "  %s client <host> [--port PORT]\n", program);
}

static int parse_port(const char *value, int *port)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 1 || parsed > 65535) {
        return -1;
    }

    *port = (int)parsed;
    return 0;
}

static int parse_args(int argc, char **argv, netprobe_config *config)
{
    int i;

    config->mode = MODE_NONE;
    config->host = NULL;
    config->port = DEFAULT_PORT;

    if (argc < 2) {
        return -1;
    }

    if (strcmp(argv[1], "server") == 0) {
        config->mode = MODE_SERVER;
        i = 2;
    } else if (strcmp(argv[1], "client") == 0) {
        if (argc < 3) {
            return -1;
        }
        config->mode = MODE_CLIENT;
        config->host = argv[2];
        i = 3;
    } else {
        return -1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            if (parse_port(argv[i + 1], &config->port) != 0) {
                fprintf(stderr, "Invalid port: %s\n", argv[i + 1]);
                return -1;
            }
            i += 2;
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    netprobe_config config;

    if (parse_args(argc, argv, &config) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    if (config.mode == MODE_SERVER) {
        printf("Server mode on port %d\n", config.port);
    } else {
        printf("Client mode targeting %s:%d\n", config.host, config.port);
    }

    return 0;
}
