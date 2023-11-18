#include "kernel/types.h"
#include "user/user.h"

void
process(int fd){
    int num = 0;    // current proc has a prime
    int forked = 0; // has forked a child or not
    int pass = 0;   // received a num from pipe
    int p[2];

    while(1){
        int bytes = read(fd, &pass, 4); // read a num from pipe

        // if no more nums
        if(0 == bytes){
            close(fd);
            if(forked){ // has a child
                close(p[1]);    // no more write
                int cid;
                wait(&cid);
            }
            exit(0);
        }

        // had not got a prime
        if(0 == num){   
            num = pass;
            printf("prime %d\n", num);
        }

        // sieve
        if(pass % num != 0){    
            // is prime
            if(!forked){   
                // has no child
                pipe(p);
                forked = 1;
                if(0 == fork()){
                    // child
                    close(p[1]);    // close write
                    close(fd);
                    process(p[0]);
                }else{
                    // parent
                    close(p[0]);    // close read
                }
            }

            write(p[1], &pass, 4);  // pass the num
        }
    }
}

int
main(int argc, char *argv[]){
    int p[2];
    pipe(p);
    for(int i = 2;i <= 35;i++){
        write(p[1], &i, 4);
    }

    close(p[1]);
    process(p[0]);
    exit(0);
}