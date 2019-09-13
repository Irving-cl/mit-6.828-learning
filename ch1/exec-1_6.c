
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define PINGPONG_CNT 1000000

void foo(int in, int out)
{
    char buf[2];
    int n;
    for (int i = 0; i < PINGPONG_CNT; i++)
    {
        n = read(in, buf, sizeof(buf));
        if (n == 0)
        {
            break;
        }
        else if (n < 0)
        {
            fprintf(stderr, "read error\n");
            exit(-1);
        }
        if (write(out, buf, n) != n)
        {
            fprintf(stderr, "write error\n");
            exit(-1);
        }
    }
}

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    if (fork() == 0)
    {
        time_t begin;
        time_t end;
        time(&begin);

        foo(p[0], p[1]);
        wait(NULL);

        time(&end);
        double secs = difftime(end, begin);
        fprintf(stdout, "Performance - total:%d, time:%.2lfs, average:%.2lf/s\n", PINGPONG_CNT, secs, (double)(PINGPONG_CNT) / secs);
    }
    else
    {
        char a = 'a';
        if (write(p[1], &a, 1) != 1)
        {
            fprintf(stderr, "write error\n");
            exit(-1);
        }
        foo(p[0], p[1]);
    }
}

