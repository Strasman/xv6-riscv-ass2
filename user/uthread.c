#include "user/uthread.h"
#include "kernel/types.h"
#include "user/user.h"


// need to fill the include ??


struct uthread threads[MAX_UTHREADS];       //Task 1.1 an array for all the process threads
struct uthread* currnt_thread = threads;    //Task 1.1 a pointer for the current thread
static int inittable = 0;

//Task 1.2 
void initTableThreads(){
    struct uthread *newuthread;

    // for every thread in table
    for (newuthread = threads;    newuthread < &threads[MAX_UTHREADS];     newuthread++){
        //newuthread->utid = newUtid++;                                   // utid ??
        newuthread->state = FREE;                                       // state
        newuthread->priority = LOW;                                     // priority
        init_threads_fields(&newuthread->context, 0, sizeof(newuthread->context));   // context ??
        init_threads_fields(&newuthread->ustack, 0, STACK_SIZE);                     // state ??
    } 

    inittable = 1;
}



//Task 1.2 
int 
uthread_create(void (*start_func)() , enum sched_priority priority){
    
     if (!inittable) initTableThreads();

    struct uthread *t;
    //mabye need to put locks ??
    for(t = threads; t < &threads[MAX_UTHREADS]; t++) {// going over the thread array
        //mabye need to put locks ??
        if(t->state == FREE){// finding free thread
            // inishiliz 
            t->context.ra = (uint64)start_func; // this is the argument ??
            t->context.sp = (uint64)&t->ustack;
            t->priority = priority; 
            t->state = RUNNABLE;

            return 0;
        }
    }

    return -1;
}

//Task 1.2
void uthread_yield(){
    struct uthread *yield_thread; // the thread that we want to yiled
    yield_thread = min_priority_thread(); //find the min prority thread
    uswtch(currnt_thread , yield_thread);// restore the context of the thread with the lowest priority  
}

//Task 1.2
void uthread_exit(){
    struct uthread *switch_thread; // the thread that will take control
    switch_thread = min_priority_thread(); //find the min prority thread
    currnt_thread->state = FREE; 

    // When the last user thread in the process calls uthread_exit the process should terminate (i.e., exit(…) should be called). ??

    uswtch(currnt_thread , switch_thread);
}

//Task 1.2
enum sched_priority uthread_set_priority(enum sched_priority priority){
    enum sched_priority old_priority = currnt_thread -> priority; 
    currnt_thread -> priority = priority;
    return old_priority;
}

//Task 1.2
enum sched_priority
uthread_get_priority(){
    return currnt_thread -> priority;
}

//Task 1.2
int
uthread_start_all(){
    static int first = 1; //variable to check if the function called only once (from the main thread)
    if(first){
        first = 0;
        struct uthread *start_thread; // the thread that will take control
        start_thread = min_priority_thread(); //find the min prority thread
        uswtch(currnt_thread , start_thread);
    }
    return -1;
}

//Task 1.2
struct uthread*
uthread_self(){
    //need to complite

}



//Task 1.2 help function 

void*
init_threads_fields(void *field, int c, uint size)// initialize the threads fields
{
  char *cfield = (char *) field;
  int i;
  for(i = 0; i < size; i++){
    cfield[i] = c;
  }
  return field;
}


struct uthread*
min_priority_thread(){
    struct uthread * ret_thread = threads;
    struct uthread *t;
    int min_priority = 2; // max priority

    for (t = threads; t < &threads[MAX_UTHREADS]; t++ ){ // find the min thread in the thread table
        if (t->priority < min_priority){
            min_priority = t->priority;
            ret_thread = t;
        }
    }
    return ret_thread;
}
