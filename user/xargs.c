#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define BUF_SIZE 512

int
main(int argc, char *argv[]){
    char buf[BUF_SIZE + 1] = {0};   // stdin buffer
    char *xargsv[MAXARG] = {0}; // command and arguements after xargs
    uint occupied = 0;  // occupied bytes of buffer
    int stdin_ended = 0;    // stdin ended or not

    for(int i = 1;i < argc;i++){
        xargsv[i - 1] = argv[i]; 
    }

    while(!stdin_ended || occupied != 0){
        if(!stdin_ended){
            int remain_size = BUF_SIZE - occupied;
            int bytes = read(0, buf + occupied, remain_size);   // read from stdin
            if(bytes < 0){
                fprintf(2, "xargs: read returns an error\n");
            }else if(bytes == 0){
                close(0);
                stdin_ended = 1;
            }
            
            occupied += bytes;
        }

        char *line_end = strchr(buf, '\n'); // read a line from buf
        while(line_end){
            char xbuf[BUF_SIZE + 1] = {0};
            memcpy(xbuf, buf, line_end - buf);
            xargsv[argc - 1] = xbuf;    // connect a line of stdin to the end of arguements

            // so now xargsv is full for specific line
            if(0 == fork()){
                // child
                if(!stdin_ended){
                    close(0);
                }
                if(exec(argv[1], xargsv) < 0){  // noted that argv includes command itself
                    fprintf(2, "xargs: exec failed\n");
                    exit(1); 
                }
            }else{
                // parent
                memmove(buf, line_end + 1, occupied - (line_end - buf) - 1);    // remove the line from buf
                occupied -= line_end - buf + 1;
                memset(buf + occupied, 0, BUF_SIZE - occupied); // refresh
                int pid;
                wait(&pid); //  wait for last line to exec

                line_end = strchr(buf, '\n');
            }
        }
    }
    exit(0);
}