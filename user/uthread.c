#include "kernel/types.h"
#include "uthread.h"
#include "user.h"

// Task 1.1 
// An array for all the process threads
struct uthread threads[MAX_UTHREADS+1];
// A pointer for the current thread
struct uthread *current_thread = &threads[MAX_UTHREADS];

// Create user thread if found anyone free and make it ruunable, save current if first initiallize
int
uthread_create(void (*start_func)(), enum sched_priority priority){
    struct uthread *t;
    for(t = threads;t < &threads[MAX_UTHREADS];t++){
        if(t->state == FREE){
            t->priority = priority;
            t->context.sp = (uint64)t->ustack + STACK_SIZE - sizeof(uint64);
            t->context.ra = (uint64)start_func;
            t->state = RUNNABLE;

            if(current_thread == 0){
                current_thread = t;
            }
            return 0;            
        }
    }
    return -1;
}

// This function picks up the next user thread from the user threads table, according to the scheduling policy
// High priority first to run
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

// Terminates the calling user thread and transfers control
void
uthread_exit(){
    current_thread->state = FREE;
    struct uthread *t;
    int not_last = 0; // if the process should be terminate

    for(t = threads; not_last == 0 && t < &threads[MAX_UTHREADS];t++){
        if(current_thread != t && t->state != FREE)
                not_last = 1;
    }

    if(not_last)
        uthread_yield();
    else
        exit(0);
}

// This function sets the priority of the calling user thread
enum sched_priority
uthread_set_priority(enum sched_priority priority){
    enum sched_priority old_priority = current_thread->priority;
    current_thread->priority = priority;
    return old_priority;
}

// This function returns the current priority of the calling user thread
enum sched_priority
uthread_get_priority(){
    return current_thread->priority;
}

// This function is called by the main user thread,
// picks the first user thread to run according to the scheduling policy and starts it.
int
uthread_start_all(){
    static int first = 1; //variable to check if the function called only once (from the main thread)
    if(first == 1){
        first = 0;
        uthread_yield();
    }
    return -1;
}

// Returns a pointer to the UTCB associated with the calling thread
struct uthread*
uthread_self(){
    return current_thread;
}