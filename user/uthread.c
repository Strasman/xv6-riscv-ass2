#include "user/uthread.h"
#include "kernel/types.h"
#include "user/user.h"
// need to fill the include ??

struct uthread threads[MAX_UTHREADS];       //Task 1.1 an array for all the process threads
struct uthread* current_thread = threads;    //Task 1.1 a pointer for the current thread
static int inittable = 0;

//Task 1.2 
void 
initTableThreads(){
    struct uthread *newuthread;

    // for every thread in table
    for (newuthread = threads; newuthread < &threads[MAX_UTHREADS]; newuthread++){
        //newuthread->utid = newUtid++;                                   // utid ??
        newuthread->state = FREE;                                       // state
        newuthread->priority = LOW;                                     // priority
        //init_threads_fields(&newuthread->context, 0, sizeof(newuthread->context));   // context ??
        //init_threads_fields(&newuthread->ustack, 0, STACK_SIZE);                     // state ??
        memset(&newuthread->context, 0, sizeof(struct context));
        memset(&newuthread->ustack, 0, sizeof(STACK_SIZE));        
    } 
    inittable = 1;
}

//Task 1.2 
int 
uthread_create(void (*start_func)() , enum sched_priority priority){
    
    if (inittable == 0) initTableThreads();
    
    struct uthread *t;
    //mabye need to put locks ??
    for(t = threads; t < &threads[MAX_UTHREADS]; t++) {// going over the thread array
        //mabye need to put locks ??
        if(t->state == FREE){// finding free thread
            // inishiliz 
            t->context.ra = (uint64)start_func; // this is the argument ??
            t->context.sp = (uint64)t->ustack;
            t->priority = priority; 
            t->state = RUNNABLE;

            return 0; // end
        }
    }

    return -1;
}

//Task 1.2
void uthread_yield(){// אם אין לך טרד להחליף איתו ??
    struct uthread *yield_thread; // the thread that we want to yiled
    yield_thread = max_priority_thread(); //find the min prority thread
    current_thread->state = RUNNABLE; // we chaenge from FREE to RUNNABLE ??
    yield_thread->state = RUNNING;
    uswtch(&current_thread->context , &yield_thread->context);// restore the context of the thread with the lowest priority  
}

//Task 1.2
void uthread_exit(){
    current_thread->state = FREE; 

    if (runnable_exists() == 0){ // When the last user thread in the process calls uthread_exit the process should terminate 
        exit(0);
    }

    struct uthread *switch_thread; // the thread that will take control
    switch_thread = max_priority_thread(); //find the min priority thread
    
    switch_thread ->state = RUNNING;
    uswtch(&current_thread->context , &switch_thread->context);
}

//Task 1.2
enum sched_priority uthread_set_priority(enum sched_priority priority){
    enum sched_priority old_priority = current_thread -> priority; 
    current_thread -> priority = priority;
    return old_priority;
}

//Task 1.2
enum sched_priority
uthread_get_priority(){
    return current_thread -> priority;
}

//Task 1.2
int
uthread_start_all(){
    static int first = 1; //variable to check if the function called only once (from the main thread)
    if(first == 1){
        first = 0;
        struct uthread *start_thread; // the thread that will take control
        start_thread = max_priority_thread(); //find the min prority thread
        uswtch(&current_thread->context , &start_thread->context);
    }
    return -1;
}

//Task 1.2
struct uthread*
uthread_self(){
    return current_thread;
}





//Task 1.2 help function 

int
sched_priority_num(enum sched_priority priority)
{
    if (priority == LOW){
        return 0;
    }
    if (priority == MEDIUM){
        return 1;
    }
    if (priority == HIGH){ 
        return 2;
    }
    
    return -1;
}

struct uthread*
max_priority_thread(){
    struct uthread * ret_thread = threads;
    struct uthread *t;
    int max_priority = 0; // max priority

    for (t = threads; t < &threads[MAX_UTHREADS]; t++ ){ // find the min thread in the thread table
        int priority = sched_priority_num(t->priority);
        if(t->state == RUNNABLE && priority > max_priority && t != current_thread){
            max_priority = priority;
            ret_thread = t;
        }
    }
    return ret_thread;// there is a situation when there are no threads but the function return the first thread in the table ??
}

//Task 1.2 - uthread_exit() - find if there are thread with runnable state
int 
runnable_exists(){
    int exists = 0;
    struct uthread *t;
    for (t = threads; t < &threads[MAX_UTHREADS]; t++){
        if(t->state == RUNNABLE ){
            exists = 1;
            break;
        }  
    }
    return exists;
}

