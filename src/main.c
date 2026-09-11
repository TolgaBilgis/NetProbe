#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "stats.h"

#define DEFAULT_PORT 9000
#define LISTEN_BACKLOG 16
#define IO_BUFFER_SIZE 4096
#define LATENCY_SAMPLES 20

typedef enum {
    MODE_NONE = 0,
    MODE_SERVER,
    MODE_CLIENT
} netprobe_mode;

typedef enum {
    COMMAND_LATENCY = 1,
    COMMAND_THROUGHPUT = 2
} netprobe_command;

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

static int open_client_socket(const char *host, int port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *entry;
    char service[6];
    int fd = -1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(service, sizeof(service), "%d", port);
    rc = getaddrinfo(host, service, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (entry = result; entry != NULL; entry = entry->ai_next) {
        fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
        if (fd < 0) {
            continue;
        }

        if (connect(fd, entry->ai_addr, entry->ai_addrlen) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);

    if (fd < 0) {
        fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
    }

    return fd;
}

static int send_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *data = buffer;
    size_t sent = 0;

    while (sent < length) {
        ssize_t rc = send(fd, data + sent, length - sent, 0);

        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (rc == 0) {
            return -1;
        }

        sent += (size_t)rc;
    }

    return 0;
}

static int recv_all(int fd, void *buffer, size_t length)
{
    unsigned char *data = buffer;
    size_t received = 0;

    while (received < length) {
        ssize_t rc = recv(fd, data + received, length - received, 0);

        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (rc == 0) {
            return -1;
        }

        received += (size_t)rc;
    }

    return 0;
}

static int echo_client(int fd)
{
    unsigned char buffer[IO_BUFFER_SIZE];

    for (;;) {
        ssize_t received = recv(fd, buffer, sizeof(buffer), 0);

        if (received == 0) {
            return 0;
        }

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            return -1;
        }

        if (send_all(fd, buffer, (size_t)received) != 0) {
            perror("send");
            return -1;
        }
    }
}

static int drain_client(int fd)
{
    unsigned char buffer[IO_BUFFER_SIZE];

    for (;;) {
        ssize_t received = recv(fd, buffer, sizeof(buffer), 0);

        if (received == 0) {
            return 0;
        }

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            return -1;
        }
    }
}

static int handle_client(int fd)
{
    unsigned char command;

    if (recv_all(fd, &command, sizeof(command)) != 0) {
        fprintf(stderr, "Connection closed before benchmark command\n");
        return -1;
    }

    switch ((netprobe_command)command) {
    case COMMAND_LATENCY:
        return echo_client(fd);
    case COMMAND_THROUGHPUT:
        return drain_client(fd);
    default:
        fprintf(stderr, "Unknown benchmark command: %u\n", command);
        return -1;
    }
}

static double elapsed_ms(const struct timespec *start, const struct timespec *end)
{
    double seconds = (double)(end->tv_sec - start->tv_sec);
    double nanoseconds = (double)(end->tv_nsec - start->tv_nsec);

    return seconds * 1000.0 + nanoseconds / 1000000.0;
}

static int measure_latency(int fd)
{
    unsigned char command = COMMAND_LATENCY;
    unsigned char probe = 0;
    unsigned char response;
    double samples[LATENCY_SAMPLES];
    latency_stats stats;
    int i;

    if (send_all(fd, &command, sizeof(command)) != 0) {
        perror("send");
        return -1;
    }

    for (i = 0; i < LATENCY_SAMPLES; i++) {
        struct timespec start;
        struct timespec end;

        if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
            perror("clock_gettime");
            return -1;
        }

        if (send_all(fd, &probe, sizeof(probe)) != 0) {
            perror("send");
            return -1;
        }

        if (recv_all(fd, &response, sizeof(response)) != 0) {
            fprintf(stderr, "Connection closed during latency test\n");
            return -1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
            perror("clock_gettime");
            return -1;
        }

        samples[i] = elapsed_ms(&start, &end);
        probe++;
    }

    if (calculate_latency_stats(samples, LATENCY_SAMPLES, &stats) != 0) {
        fprintf(stderr, "Unable to calculate latency statistics\n");
        return -1;
    }

    printf("Latency\n");
    printf("  min  %.3f ms\n", stats.min_ms);
    printf("  avg  %.3f ms\n", stats.avg_ms);
    printf("  p95  %.3f ms\n", stats.p95_ms);
    printf("  p99  %.3f ms\n", stats.p99_ms);
    printf("  max  %.3f ms\n", stats.max_ms);

    return 0;
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

        handle_client(client_fd);
        close(client_fd);
    }
}

static int run_client(const char *host, int port)
{
    int fd = open_client_socket(host, port);

    if (fd < 0) {
        return -1;
    }

    printf("Connected to %s:%d\n", host, port);

    if (measure_latency(fd) != 0) {
        close(fd);
        return -1;
    }

    close(fd);
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
        if (run_server(config.port) != 0) {
            return 1;
        }
    } else {
        if (run_client(config.host, config.port) != 0) {
            return 1;
        }
    }

    return 0;
}
