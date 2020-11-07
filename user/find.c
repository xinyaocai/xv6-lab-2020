#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void find(char *path, char *aim, char *ans[], int *cnt) {
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

    switch(st.type) {
        case T_FILE:
            if (strcmp(fmtname(path), aim) == 0)
                ans[(*cnt)++] = path;
            break;
        case T_DIR:
            struct dirent dt;
            while (read(fd, &dt, sizeof(dt)) == sizeof(dt)) {
                if (dt.inum == 0) continue;
                
            }
    }
}

int
main(int argc, char *args[]) {
    if (argc < 3) {
        fprintf(2, "invalid args.\n");
        exit(1);
    }

    char *path = args[1], *aim = args[2];
    char *ans[100];
    int cnt = 0;
    find(path, aim, ans, &cnt);

    for (int i = 0; i < cnt; i++) {
        printf(ans[i]);
        printf('\n');
    }
    

}