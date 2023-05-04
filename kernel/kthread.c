#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// Task 2.1
extern struct proc proc[NPROC];

extern void forkret(void);

// Initializes locks and states for threads and for process counter
void 
kthreadinit(struct proc *p)
{
  // Initiallize lock for process threads counter
  initlock(&p->ktidlock, "ktid_counter");
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++){
    // Initiallize lock for every thread in the process table
    initlock(&kt->ktlock, "kthread_lock"); 
    kt->ktstate = KTUNUSED;
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

// Return the current running thread
struct kthread *
mykthread() 
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kthread;
  pop_off();
  return kt;
}

// Allocate id for process threads
int
alloctid(struct proc *p){
  int ktpid;
  acquire(&p->ktidlock);
  ktpid = p->ktidcounter;
  p->ktidcounter++;
  release(&p->ktidlock);
  return ktpid;
}

// Allocate thread, if found an unused one in the process
struct kthread*
allockthread(struct proc *p){
  struct kthread *kt ;

  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->ktlock);
    if(kt->ktstate == KTUNUSED){
      goto found;
    }
    else{
      release(&kt->ktlock);
    }
  }
  return 0;  

  found:
  kt->ktid = alloctid(p);
  kt->ktstate = KTUSED;

  // Allocate a trapframe page.
  if ((kt->trapframe = get_kthread_trapframe(p, kt)) == 0){
    freekthread(kt);
    release(&kt->ktlock);
    return 0;
  }

  memset(&kt->ktcontext, 0, sizeof(kt->ktcontext));
  kt->ktcontext.ra = (uint64)forkret;
  kt->ktcontext.sp = kt->kstack + PGSIZE;
  kt->proc = p;

  return kt;
}

// Free the Kernal thread
void
freekthread(struct kthread *kt){
  kt->ktid = 0;
  kt->ktchan = 0;
  kt->ktkilled = 0;
  kt->ktxstate = 0;
  kt->proc = 0; 
  kt->trapframe = 0; 
  kt->ktstate = KTUNUSED;
 }

// Get the kthread kernal trapframe
struct trapframe *
get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}

// Task 2.3

// We changed the signature of this function to (uint64, uint64, uint) for  (virtual?) address of the funcion 
int 
kthread_create(uint64 start_func , uint64 stack, uint stack_size){
  struct proc* p = myproc();
  struct kthread* kt = allockthread(p);
  if(kt == 0) // Allocate failed -> create failed
    return -1;

  *(kt->trapframe) = *(mykthread()->trapframe);
  kt->trapframe->epc = start_func;
  kt->trapframe->sp = stack + stack_size;
  kt->ktstate = KTRUNNABLE;
  release(&kt->ktlock); // Release the lock acquired in allocthread
  return kt->ktid;
}

// Return the current thread id
int 
kthread_id(){ 
  return mykthread()->ktid;
}

// Kill the thread of the given id, if sleeping -> make runnable, change ktkilled (flag) to 1
int 
kthread_kill(int ktid){
  struct kthread* kt;
  struct proc* p= myproc();
  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->ktlock);
    if(kt -> ktid == ktid )  {
      kt -> ktkilled = 1;
      if(kt -> ktstate == KTSLEEPING){
          kt -> ktstate = KTRUNNABLE;
      }
      release(&kt->ktlock);
      return 0;
    } 
    release(&kt->ktlock);
  }
    return -1;
}

// Exit the thread, if the only one close the process, else make zombie, wake up an dsend to sched
void 
kthread_exit(int status){
  struct kthread *kt = mykthread();
  int num_threads = 0;
  struct proc *p = myproc();
  struct kthread *t;

  acquire(&p->lock);
  for(t=p->kthread; t<&p->kthread[NKT]; t++){
    acquire(&t->ktlock);
    if((t->ktstate != KTUNUSED && t->ktstate != KTZOMBIE)){
      num_threads += 1;
    }
    release(&t->ktlock);
  }
  release(&p->lock);

  if(num_threads == 1)
    exit(status);
  else{
    acquire(&kt->ktlock);
    kt->ktstate=KTZOMBIE;
    kt->ktxstate=status;
    release(&kt->ktlock);
    // When send to wakeup needs to be un acquired
    wakeup(kt);
    acquire(&kt->ktlock);
    // Jump into the scheduler, never to return -  thread should be acquired
    sched();
    panic("zombie exit - kthread");
  }
}

// This function suspends the execution of the calling thread until the target thread terminates
int 
kthread_join(int ktid, uint64 status){
  struct kthread *kt;
  struct kthread *target_thread = 0;
  int otherthreads;
  struct proc *p = myproc();

  acquire(&p->lock);
  for(;;){
    // Scan through process threads looking for target
    otherthreads = 0;
    for(kt = p->kthread; kt < &p->kthread[NKT]; kt++){
      acquire(&kt->ktlock);
      if(kt -> ktid == ktid && kt -> ktstate != KTUNUSED){
        target_thread = kt;
        otherthreads = 1;
        if(kt -> ktstate == KTZOMBIE){
          // If copyout and status fail, the function fail
          if(status != 0 && copyout(p->pagetable, status, (char *)&kt -> ktxstate,
                                  sizeof(kt -> ktxstate)) < 0) {
            release(&kt->ktlock);
            release(&p->lock);
            return -1;
          }
          // If found and all good, free thread
          freekthread(kt);
          release(&kt->ktlock);
          release(&p->lock);
          return 0; 
        }
      }
      release(&kt->ktlock);
    }

    // No point waiting if we don't have any other threads.
    if(!otherthreads || mykthread()->ktkilled == 1){
      release(&p->lock);
      return -1;
    }
    
    // Wait for a thread to exit
    sleep(target_thread, &p->lock);  
  }
}


