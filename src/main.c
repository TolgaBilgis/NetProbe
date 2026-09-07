#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 9000
#define LISTEN_BACKLOG 16

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

static int open_listen_socket(int port)
{
    struct sockaddr_in address;
    int fd;
    int reuse = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static int run_server(int port)
{
    int listen_fd = open_listen_socket(port);

    if (listen_fd < 0) {
        return -1;
    }

    printf("Listening on port %d\n", port);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            close(listen_fd);
            return -1;
        }

        char address[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &peer.sin_addr, address, sizeof(address)) != NULL) {
            printf("Accepted connection from %s:%u\n", address, ntohs(peer.sin_port));
        }

        close(client_fd);
    }
}

int main(int argc, char **argv)
{
    netprobe_config config;

    if (parse_args(argc, argv, &config) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    if (config.mode == MODE_SERVER) {
        if (run_server(config.port) != 0) {
            return 1;
        }
    } else {
        printf("Client mode targeting %s:%d\n", config.host, config.port);
    }

    return 0;
}
