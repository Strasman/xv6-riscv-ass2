#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "uthread.h"

void* func(){
    kthread_exit(0);
    return 0;
}

int main(int argc, char** argv){
    printf("This test will try to call for kthread_exit from 2 threads thread\nExpected Behvaiour: sucessfull exit and return to the shell\n");
    void* memory = malloc(4000);
    void* memory2 = malloc(4000);
    int tid = kthread_create(func,memory,4000);
    fprintf(2,"[main] created %d\n",tid);
    tid = kthread_create(func,memory2,4000);
    fprintf(2,"[main] created %d\n",tid);
    kthread_exit(0);
    return 1;
}