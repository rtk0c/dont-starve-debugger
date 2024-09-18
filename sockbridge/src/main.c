#include "arena.h"
#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

static void ensure_fifo_exists(const char* path, const char* fifo_usage) {
    struct stat sb;
    int stat_res = stat(path, &sb);

    // If the file does not exist, (try to) create the FIFO
    if (stat_res != 0 && errno == ENOENT) {
        if (mkfifo(path, 0600) == 0) {
            printf("Created FIFO '%s' for %s data\n", path, fifo_usage);
            return;
        } else {
            perror("Failed to create FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // If the file exists and is FIFO, great
    if (S_ISFIFO(sb.st_mode)) {
        printf("Existing FIFO at '%s', ok\n", path);
        return;
    }

    // Otherwise, cannot continue
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void parse_addr(char* addr, char** port) {
    int i = 0;
    while (addr[i] != ':')
        ++i;

    addr[i] = '\0';
    *port = addr + i + 1;
    // char* end;
    // *port = strtol(addr + i + 1, &end, 10);
}

static int socket_to_upstream_debugger(const char* upstream_host, const char* upstream_port) {
    struct addrinfo hints = {};
    struct addrinfo* addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(upstream_host, upstream_port, &hints, &addrs) != 0) {
        perror("Failed to parse upstream address");
        exit(EXIT_FAILURE);
    }
    if (addrs == NULL) {
        fprintf(stderr, "Resolved upstream address to nothing\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    if (sockfd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, addrs->ai_addr, addrs->ai_addrlen) < 0) {
        perror("Failed to connect to upstream debugger");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

typedef struct {
    const char* fifo_path;

    int result_fd;
} FifoOpener;

static void* fifo_opener_thrd_func(void* data_) {
    FifoOpener* data = (FifoOpener*)data_;

    int fd = open(data->fifo_path, 0);
    if (fd == -1) {

    }

    return NULL;
}

static int open_game2dbg_fifo(const char* path) {
    // This FIFO would be used to read on our side, O_NONBLOCK will just get us a fd happy
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Failed to open game -> debugger FIFO");
        exit(EXIT_FAILURE);
    }

    return fd;
}

static int open_dbg2game_fifo(const char* path) {
    // This FIFO would be used to write, O_NONBLOCK will just fail, need to open blockingly to wait until DST conencts
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open debugger -> game FIFO");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int main(int argc, char** argv) {
    String dst_data_dir = str_from_cstring(argv[1]);
    String upstream = str_from_cstring(argv[2]);

    Arena* a = make_arena();

    // Prepare path to /path/to/dst/debugger_bound
    char* game2dbg_path = ARENA_DO_STRING_CONCAT(a, dst_data_dir);
    ARENA_DO_LITERAL_CONCAT(a, "/debugger_bound");
    ARENA_DO_NULL_TERMINATE(a);
    ensure_fifo_exists(game2dbg_path, "game -> debugger");

    // Prepare path to /path/to/dst/debuggee_bound
    char* dbg2game_path = ARENA_DO_STRING_CONCAT(a, dst_data_dir);
    ARENA_DO_LITERAL_CONCAT(a, "/debuggee_bound");
    ARENA_DO_NULL_TERMINATE(a);
    ensure_fifo_exists(dbg2game_path, "debugger -> game");

    char* upstream_host = upstream.data;
    char* upstream_port;
    parse_addr(upstream_host, &upstream_port);

    printf("Connecting to downstream game...\n");
    int game2dbg_fd = open_game2dbg_fifo(game2dbg_path);
    int dbg2game_fd = open_dbg2game_fifo(dbg2game_path);
    printf("Connected.\n");
  
    printf("Connecting to upstream %s:%s...\n", upstream_host, upstream_port);
    int sockfd = socket_to_upstream_debugger(upstream_host, upstream_port);
    printf("Connected.\n");

    // TODO pipe data

    // Let's not free `a`. We're exiting anyways.
    return 0;
}
