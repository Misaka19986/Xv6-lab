#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
    int p_1[2], p_2[2];
    int pid;
    char buf[] = {'b'}; // message one byte

    pipe(p_1);
    pipe(p_2);

    /*
        parent send by p_1[1], child receives that through p_1[0]
        p_2 is similar to p_1
    */
    if(0 == fork()){
        pid = getpid();

        close(p_1[1]);  // close write side of p_1
        read(p_1[0], buf, 1);   // receive one byte from parent

        printf("%d:received ping\n", pid);   

        close(p_2[0]);  // close read side of p_2
        write(p_2[1], buf, 1);
        
        exit(0);
    }else{
        pid = getpid();

        close(p_1[0]);  // close read side of p_1
        write(p_1[1], buf, 1);  // send one byte to child through p_1

        close(p_2[1]);  // close write side of p_2
        read(p_2[0], buf, 1);   // receive one byte from child

        printf("%d: received pong\n", pid);

        exit(0);
    }

}