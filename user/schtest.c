// multi-level queue scheduler test
//      by: raein soltani

#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"

#define timer_tick 1000000000
#define number_process 10

int
main(int argc, char *argv[]) {
    int pid;
    for (int i = 0; i < number_process; i++)
    {
        pid = fork();
        if (i == 6 || i == 7){
            setprio(7);
        }
        if (pid == 0)
        {
            for (int i = 0; i < timer_tick; i++){}
            printf("Proccess %d exited, pid: %d \n", i, getpid());
            exit(0);
        }
        
    }

    exit(0);
}