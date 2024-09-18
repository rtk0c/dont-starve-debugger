#include "arena.h"
#include "str.h"

#include <asm-generic/ioctls.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

    ioctl(sockfd, FIONBIO);

    return sockfd;
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

    // And then set it to nonblocking, so we can use it with epoll
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    return fd;
}

static int setup_epoll() {
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("Failed to create epoll");
        exit(EXIT_FAILURE);
    }

    return epollfd;
}

static void add_epoll_fd(int epollfd, int flags, int watched_fd) {
    struct epoll_event ev;
    ev.events = flags;
    ev.data.fd = watched_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, watched_fd, &ev) == -1) {
        perror("Failed to add epoll watched fd");
        exit(EXIT_FAILURE);
    }
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

    int epollfd = setup_epoll();
    add_epoll_fd(epollfd, EPOLLIN, sockfd);
    add_epoll_fd(epollfd, EPOLLIN, game2dbg_fd);
    // TODO do not assume writes will finish in one go
    // add_epoll_fd(epollfd, EPOLLOUT, dbg2game_fd);

#define MAX_EVENTS 8

    char dbg2game_buffer[2048];
    char game2dbg_buffer[2048];

    for (;;) {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("Failed to wait on epoll");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            int thefd = events[i].data.fd;
            if (thefd == sockfd) {
                int n = recv(sockfd, dbg2game_buffer, sizeof(dbg2game_buffer), 0);
                if (n == 0)
                    goto exit;
                else if (n == -1)
                    // handle_recv_error(sockfd, error);
                    ;

                int res = write(dbg2game_fd, dbg2game_buffer, n);
                if (res == -1) {
                }
            } else if (thefd == game2dbg_fd) {
                int n = read(game2dbg_fd, game2dbg_buffer, sizeof(game2dbg_buffer));
                if (n == 0)
                    goto exit;
                else if (n == -1)
                    // handle_recv_error(sockfd, error);
                    ;
                
                int res = send(sockfd, game2dbg_buffer, n, 0);
                if (res == -1) {
                }
            } else if (thefd == dbg2game_fd) {
                // TODO write extra data
            }
        }
    }

exit:
    // Let's not free `a`. We're exiting anyways.
    // Or any of the other resources
    return 0;
}
