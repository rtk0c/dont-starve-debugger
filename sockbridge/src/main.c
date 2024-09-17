#include "arena.h"
#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>  
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

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
        printf("Existing FIFO at '%s', ok\n");
        return;
    }
    
    // Otherwise, cannot continue
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    String dst_data_dir = str_from_cstring(argv[1]);
    String debugger_addr = str_from_cstring(argv[2]);

    Arena* a = make_arena();

    // Prepare path to /path/to/dst/debugger_bound
    char* game2dbg_fifo = ARENA_DO_STRING_CONCAT(a, dst_data_dir);
    ARENA_DO_LITERAL_CONCAT(a, "/debugger_bound");
    ARENA_DO_NULL_TERMINATE(a);
    ensure_fifo_exists(game2dbg_fifo, "game -> debugger");

    // Prepare path to /path/to/dst/debuggee_bound
    char* dbg2game_fifo = ARENA_DO_STRING_CONCAT(a, dst_data_dir);
    ARENA_DO_LITERAL_CONCAT(a, "/debuggee_bound");
    ARENA_DO_NULL_TERMINATE(a);
    ensure_fifo_exists(dbg2game_fifo, "debugger -> game");      

    // Let's not free `a`. We're exiting anyways.
    return 0;
}
