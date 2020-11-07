#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
 
#define R 0
#define W 1
 
int
main(int argc, char *argv[])
{
    int numbers[100], cnt = 0, i;
    int fd[2];
    for (i = 2; i <= 35; i++) {
        numbers[cnt++] = i;
    }
    // 注意fork是在这个循环内进行的
    while (cnt > 0) {
        pipe(fd);
        if (fork() == 0) {
            int prime, this_prime = 0;
            close(fd[W]);
            cnt = -1;
            // 读的时候，如果父亲还没写，就会block
            while (read(fd[R], &prime, sizeof(prime)) != 0) {
                // 设置当前进程代表的素数，然后筛掉能被当前素数整除的数
                if (cnt == -1) {
                    this_prime = prime;
                    cnt = 0;
                } else {
                    // 把筛出来的接着放在number数组里？不对，这里cnt是重新从0开始计数的
                    if (prime % this_prime != 0) numbers[cnt++] = prime;
                }
            }
            printf("prime %d\n",this_prime);
            close(fd[R]);
        } else {
            close(fd[R]);
            for (i = 0; i < cnt; i++) {
                write(fd[W], &numbers[i], sizeof(numbers[0]));
            }
            close(fd[W]);
            wait(0);
            break;
        }
    }
    exit(0);
}