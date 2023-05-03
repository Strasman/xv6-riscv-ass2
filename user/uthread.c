#include "kernel/types.h"
#include "uthread.h"
#include "user.h"

struct uthread threads[MAX_UTHREADS+1];

struct uthread *current_thread = &threads[MAX_UTHREADS];



int
uthread_create(void (*start_func)(), enum sched_priority priority){
    struct uthread *t;
    for(t = threads;t < &threads[MAX_UTHREADS];t++){
        if(t->state == FREE){
            t->priority = priority;
            t->state = RUNNABLE;
            t->context.sp = (uint64)t->ustack + STACK_SIZE - sizeof(uint64);
            t->context.ra = (uint64)start_func;
            // t->context.s0 = 0;
            // t->context.s1 = 0;
            // t->context.s2 = 0;
            // t->context.s3 = 0;
            // t->context.s4 = 0;
            // t->context.s5 = 0;
            // t->context.s6 = 0;
            // t->context.s7 = 0;
            
            if(current_thread == 0){
                current_thread = t;
            }
            return 0;            
        }
    }
    return -1;
}

void
uthread_yield(){
    enum sched_priority priority = LOW;
    struct uthread *t_yield = 0;
    
    struct uthread *t;
    for(t = threads;t < &threads[MAX_UTHREADS];t++){
        if(t->state == RUNNABLE && t->priority >= priority){
            t_yield = t;
            priority = t->priority;    
        }
    }

    if(t_yield != 0 && t_yield != current_thread){
            struct uthread *prev = current_thread;
            t_yield->state = RUNNING;
            current_thread = t_yield;
            uswtch(&prev->context,&t_yield->context);
    }
}

void
uthread_exit(){
    current_thread->state = FREE;
    struct uthread *t;
    int not_last = 0; // if the thread should be terminate

    for(t = threads; not_last == 0 && t < &threads[MAX_UTHREADS];t++){
        if(current_thread != t && t->state != FREE)
                not_last = 1;
    }

    if(not_last)
        uthread_yield();
    else
        exit(0);
}

enum sched_priority
uthread_set_priority(enum sched_priority priority){
    enum sched_priority old_priority = current_thread->priority;
    current_thread->priority = priority;
    return old_priority;
}

enum sched_priority
uthread_get_priority(){
    return current_thread->priority;
}

int
uthread_start_all(){
    static int first = 1; //variable to check if the function called only once (from the main thread)
    if(first == 1){
        first = 0;
        uthread_yield();
    }
    return -1;
}

struct uthread*
uthread_self(){
    return current_thread;
}