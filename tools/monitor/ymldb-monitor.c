#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// mkfifo()
#include <sys/types.h>
#include <sys/stat.h>

#include "ymldb.h"

int done = 0;

void signal_handler_INT(int param)
{
    done = 1;
}

int main(int argc, char *argv[])
{
    int k;

    // MUST ignore SIGPIPE.
    signal(SIGPIPE, SIG_IGN);

    // add a signal handler to quit this program.
    signal(SIGINT, signal_handler_INT);

    /* Analyze command line options. */
    for (k = 1; k < argc; k ++)
    {
        if (strcmp(argv[k], "-h") == 0
                || strcmp(argv[k], "--help") == 0) {
            printf("%s key1 key2 ...\n"
                    "-h, --help\t\tdisplay this help and exit\n"
                    "key\t\tYMLDB key to monitor\n",
                    argv[0]);
            return 0;
        }
        ymldb_create(argv[k], YMLDB_FLAG_SUBSCRIBER);
    }
    int res;
    int cnt = 0;
    int max_fd = 0;
    fd_set read_set;
    struct timeval tv;

    do
    {
        FD_ZERO(&read_set);
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        max_fd = ymldb_distribution_set(&read_set);
        res = select(max_fd + 1, &read_set, NULL, NULL, &tv);
        if (res < 0)
        {
            fprintf(stderr, "select failed (%s)\n", strerror(errno));
            break;
        }
        ymldb_distribution_recv_and_dump(stdout, &read_set);
    } while (!done);
    ymldb_dump_all(stdout);
    ymldb_destroy_all();
    ymldb_dump_all(stdout);
    fprintf(stdout, "end.\n");
    return 0;
}