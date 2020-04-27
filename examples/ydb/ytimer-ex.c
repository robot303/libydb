// ytimer-ex.c

#include <stdio.h>
#include <ylog.h>
#include <ytimer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static bool done;
void HANDLER_SIGINT(int signal)
{
    done = true;
}

int timer_func2(unsigned int timer_id, void *cookie)
{
    fprintf(stdout, "%s\n", __func__);
    return 0;
}

int timer_func1(unsigned int timer_id, void *cookie)
{
    ytimer *timer;
    timer = (ytimer *) cookie;
    ytimer_set_msec(timer, 100, false, timer_func2, NULL);
    fprintf(stdout, "%s\n", __func__);
    return 0;
}


int main(int argc, char *argv[])
{
    int count = 0;
    int fd, ret;
    fd_set read_set;
    ytimer *timer;
    ylog_severity = YLOG_DEBUG;
    timer = ytimer_create();
    if (timer == NULL)
    {
        fprintf(stderr, "timer createion failed.\n");
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, HANDLER_SIGINT);

    unsigned int timerid = ytimer_set_msec(timer, 666, true, timer_func1, timer);

    while (!done)
    {
        fd = ytimer_fd(timer);
        FD_ZERO(&read_set);
        if (fd < 0)
        {
            fprintf(stderr, "timer fd failed.\n");
            goto failed;
        }
        FD_SET(fd, &read_set);
        ret = select(fd + 1, &read_set, NULL, NULL, NULL);
        if (ret < 0)
        {
            done = 1;
        }
        else if (ret == 0)
        {
            // timeout
        }
        else
        {
            if (FD_ISSET(fd, &read_set))
            {
                FD_CLR(fd, &read_set);
                ytimer_handle(timer);
                count++;
            }
        }
        if (count == 10)
            ytimer_delete(timer, timerid);
    }
failed:
    ytimer_destroy(timer);
    return 0;
}