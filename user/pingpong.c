#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *args[]) {
    int p[2];
    char c;
    pipe(p);

    close(p[0]);
    write(p[1], &c, 1);

    if (fork() == 0) {
        close(p[1]);
        int nread = read(p[0], &c, 1);
        if (nread) {
            printf("%d: received ping\n", getpid());
        }
        close(p[0]);
        write(p[1], &c, 1);
        exit(0);
    } else {
        wait(0);
        close(p[1]);
        int nread = read(p[0], &c, 1);
        if (nread) {
            printf("%d: received pong\n", getpid());
        }
        exit(0);
    }
}