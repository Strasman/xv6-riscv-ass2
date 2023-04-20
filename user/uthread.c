#include "user/uthread.h"
#include "kernel/types.h"
#include "user/user.h"

// need to fill the include ??


struct uthread threads[MAX_UTHREADS];       // Task 1.1 an array for all the process threads
struct uthread* currnt_thread = threads;    // Task 1.1 a pointer for the current thread

//Task 1.2 
int 
uthread_create(void (*start_func)() , enum sched_priority priority){
    struct uthread *t;
    //mabye need to put locks ??
    for(t = threads; t < &threads[MAX_UTHREADS]; t++) {// going over the thread array
        //mabye need to put locks ??
        if(t->state == FREE){// finding free thread
            // inishiliz 
            char stack[STACK_SIZE] = "";
            *t->ustack = stack;
            t->context.ra = start_func;
            t->context.sp = t->ustack;
            t->priority = priority; 
            t->state = RUNNABLE;

            return 0;
        }
    }

    return -1;
}



