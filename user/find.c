#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void make_null_str(char arr[100]) {
    for (int i = 0; i < 100; i++) {
        arr[i] = '\0';
    }
}

char*
fmtname(char *path)
{
    int i = strlen(path);
    for (; i >= 0; i--)
        if (path[i] == '/')
            break;
    return path+i+1;
}

void find(char *path, char *aim) {
    int fd;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    struct dirent de;

    switch(st.type) {
        case T_FILE: {
            if (strcmp(fmtname(path), aim) == 0) {
                printf(path);
                printf("\n");
            }
            break;
        }
        case T_DIR: {
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0) continue;
                char next_path[100], *p = next_path;
                make_null_str(next_path);
                strcpy(next_path, path);
                p += strlen(next_path);
                *p++ = '/';
                strcpy(p, de.name);
                find(next_path, aim);
            }
        }
    }

    close(fd);
}

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "invalid args.\n");
        exit(1);
    }
    char *path = argv[1], *aim = argv[2];
    find(path, aim);
    exit(0);
}
