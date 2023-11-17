//History function for xv6
//by Raein Soltani

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"


int main(int argc, char *argv[]){
    history(atoi(argv[1]));
    return 0;
}