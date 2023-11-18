#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

// just like ls.c

char* 
getLastString(char* path){
    char* p;
    for(p = path + strlen(path);p >= path && *p != '/';p--);
    p++;
    return p;
}

void 
find(char* path, char* target){
    char buf[512], *p;
    int fd;

    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:;
            char* name = getLastString(path);
            int catch = 1;
            if(strcmp(name, target) || name == 0){   // full comparsion
                catch = 0;
            }
            if(catch){
                printf("%s\n", path);
            }
            close(fd);
            break;
        
        case T_DIR:;
            // make new path
            uint path_len = strlen(path);
            memset(buf, 0, sizeof(buf));
            memcpy(buf, path, path_len);

            buf[path_len] = '/';
            p = buf + path_len + 1;
            while(read(fd, &de, sizeof(de)) == sizeof(de)){ // recurse every sub-directories of path
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){   // sub-directories includes "." and ".."
                    continue;
                }
                memcpy(p, de.name, DIRSIZ); // connect sub-directories name to new path
                p[DIRSIZ] = 0;
                find(buf, target);
            }
            close(fd);
            break;
    }
}

int
main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(2, "usage: find [directory] [target filename]\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}