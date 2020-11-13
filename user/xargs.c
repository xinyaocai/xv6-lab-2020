#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argn, char *argv[]) {
    char temp[MAXARG][100];
    int i = 1;
    for (; i < argn; i++) {
        memcpy(temp[i-1], argv[i], strlen(argv[i])+1);
    }
    char c;
    int cnt = 0;
    i--;
    while (read(0, &c, 1) != 0) {
        if (c == '\n') {
            temp[i][cnt] = '\0';
            i++;
            cnt = 0;
        } else {
            temp[i][cnt++] = c;
        }
    }
    char *exec_argv[i+1];
    for (int j = 0; j < i; j++) {
        exec_argv[j] = temp[j];
    }
    exec_argv[i] = 0;
    exec(argv[1], exec_argv);
    exit(0);
}