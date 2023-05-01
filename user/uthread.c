// #include "user/uthread.h"
// #include "kernel/types.h"
// #include "user/user.h"


// struct uthread threads[MAX_UTHREADS+1];       //Task 1.1 an array for all the process threads
// struct uthread* current_thread = &threads[MAX_UTHREADS];    //Task 1.1 a pointer for the current thread


// //Task 1.2 
// int 
// uthread_create(void (*start_func)() , enum sched_priority priority){
    
    
//     struct uthread *t;
//     for(t = threads; t < &threads[MAX_UTHREADS]; t++) {// going over the thread array
//         if(t->state == FREE){// finding free thread
//             // init
//             t->context.ra = (uint64)start_func; // this is the argument ??
//             t->context.sp = (uint64)t->ustack;
//             t->priority = priority; 
//             t->state = RUNNABLE;
//             t->context.s0 = 0;
//             t->context.s1 = 0;
//             t->context.s2 = 0;
//             t->context.s3 = 0;
//             t->context.s4 = 0;
//             t->context.s5 = 0;
//             t->context.s6 = 0;
//             t->context.s7 = 0;

//             if(current_thread == 0){
//                 current_thread = t;
//             }
//             return 0; // end
//         }
//     }

//     return -1;
// }

// //Task 1.2
// void uthread_yield(){// אם אין לך טרד להחליף איתו ??
//     struct uthread *yield_thread; // the thread that we want to yiled
//     yield_thread = max_priority_thread(); //find the min prority thread
//     if (yield_thread != 0 && yield_thread != current_thread){
//         struct uthread *p = current_thread;
//         yield_thread->state = RUNNING;
//         current_thread = yield_thread;
//         uswtch(&p->context , &yield_thread->context);// restore the context of the thread with the lowest priority  
//     }
//     return;
// }

// //Task 1.2
// void uthread_exit(){
//     current_thread->state = FREE;
//     struct uthread *t;
//     int shouldTerminate = 0; 

//     for(t = threads;t < &threads[MAX_UTHREADS];t++){
//         if(current_thread != t && t->state != FREE){
//                 shouldTerminate = 1;
//                 break;
//         }
//     }

//     if(shouldTerminate)
//         uthread_yield();
//     else
//         exit(0);
// }

// //Task 1.2
// enum sched_priority uthread_set_priority(enum sched_priority priority){
//     enum sched_priority old_priority = current_thread -> priority; 
//     current_thread -> priority = priority;
//     return old_priority;
// }

// //Task 1.2
// enum sched_priority
// uthread_get_priority(){
//     return current_thread -> priority;
// }

// //Task 1.2
// int
// uthread_start_all(){
//     static int first = 1; //variable to check if the function called only once (from the main thread)
//     if(first == 1){
//         first = 0;
//         uthread_yield();
//     }
//     return -1;
// }

// //Task 1.2
// struct uthread*
// uthread_self(){
//     return current_thread;
// }



// //Task 1.2 help function 

// int
// sched_priority_num(enum sched_priority priority)
// {
//     if (priority == LOW){
//         return 0;
//     }
//     if (priority == MEDIUM){
//         return 1;
//     }
//     if (priority == HIGH){ 
//         return 2;
//     }
    
//     return -1;
// }

// struct uthread*
// max_priority_thread(){
//     struct uthread * ret_thread = 0;
//     struct uthread *t;
//     int max_priority = 0; // max priority

//     for (t = threads; t < &threads[MAX_UTHREADS]; t++ ){ // find the min thread in the thread table
//         int priority = sched_priority_num(t->priority);
//         if(t->state == RUNNABLE && priority > max_priority && t != current_thread){
//             max_priority = priority;
//             ret_thread = t;
//         }
//     }
//     return ret_thread;// there is a situation when there are no threads but the function return the first thread in the table ??
// }

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
            t->context.s0 = 0;
            t->context.s1 = 0;
            t->context.s2 = 0;
            t->context.s3 = 0;
            t->context.s4 = 0;
            t->context.s5 = 0;
            t->context.s6 = 0;
            t->context.s7 = 0;
            
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