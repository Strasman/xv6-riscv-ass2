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


void chld(){
    printf("thank you kind sir\n");
exit(1);
}
void* thread(){
    int status;
    int pid;
    if((pid = fork()) ==0){
        fprintf(2,"child's PID: %d\n",getpid());
        chld();
    }else{
        if(pid == getpid()){
            fprintf(2,"[ERROR] the new process is the same as the other process\n");   
            exit(0x55); 
        }
        if(wait(&status) != -1 && status != 1){
                fprintf(2,"[ERROR] child exited abnormally\n");   
                kthread_exit(-1);         
        }
        kthread_exit(0);
    }
    return 0;
}
int main(int argc,char** argv){
    printf("This test will try to fork this process from a thread.\n\
    Expected Behaviuor:\n\
    the child prints a msg 'thank you kind sir' and exit\n\n");
    fprintf(2,"father PID: %d\n",getpid());
    void* stack = malloc(4000);
    int tid;
    if((tid = kthread_create(thread,stack,4000)) == -1){
        fprintf(2,"[ERROR] couldn't start a thread\n");
        return 1;
    }
    return 0;
}