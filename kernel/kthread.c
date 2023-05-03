#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];
//extern struct spinlock wait_lock; // ??
extern void forkret(void);


void 
kthreadinit(struct proc *p)
{
  //acquire(&wait_lock); // ?? --
  //acquire(&p->lock);
  initlock(&p->ktidlock, "ktid_counter");

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++){
    initlock(&kt->ktlock, "kthread_lock");//initializes for every thread in the process table its lock,

    //acquire(&kt->ktlock); 

    kt->ktstate = KTUNUSED;
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));

    //release(&kt->ktlock);
  }

  //release(&p->lock);
  //release(&wait_lock); // ??
}

struct kthread *
mykthread() 
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kthread;
  pop_off();
  return kt;
}

int
alloctid(struct proc *p){
  int ktpid;
  acquire(&p->ktidlock);
  ktpid = p->ktidcounter;
  p->ktidcounter++;
  release(&p->ktidlock);
  return ktpid;
}

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
  printf("allocthread didnt find nada\n");
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

void
freekthread(struct kthread *kt){

  // if(kt->trapframe)
  //   kfree((void*)kt->trapframe);
  kt->ktid = 0;
  kt->ktchan = 0;
  kt->ktkilled = 0;
  kt->ktxstate = 0;
  //kt->proc = 0; 
  kt->trapframe = 0; 
  kt->ktstate = KTUNUSED;
  // what about the context?
 }

struct trapframe *
get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}


//Task 2.3

int 
the_only_one(struct kthread * t){
  struct proc *p = t->proc;
  struct kthread *kt;
  //acquire(&p->lock); // ??
  for(kt=p->kthread; kt<&p->kthread[NKT]; kt++){
    //acquire(&kt->ktlock);
    if(kt != t && (kt->ktstate != KTUNUSED || kt->ktstate != KTZOMBIE)){
      return 0;
    }
    //release(&kt->ktlock);
  }
  //release(&p->lock); // ??
  return 1;
}

int 
kthread_create(uint64 start_func , uint64 stack, uint stack_size){
  struct proc* p = myproc();
  struct kthread* kt = allockthread(p);
  if(kt == 0){  
    printf("create kt=0 alloc thread fails");
    return -1;
  }
  *(kt->trapframe) = *(mykthread()->trapframe);
  kt->trapframe->epc = start_func;
  kt->trapframe->sp = stack + stack_size;
  kt->ktstate = KTRUNNABLE;
  release(&kt->ktlock);
  return kt->ktid;
}

int 
kthread_id(){ 
  return mykthread()->ktid;
}

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

  if(num_threads == 1){
    exit(status);
  }
  else{
    acquire(&kt->ktlock);
    kt->ktstate=KTZOMBIE;
    kt->ktxstate=status;
    release(&kt->ktlock);
    wakeup(kt);
    
    acquire(&kt->ktlock);
    // Jump into the scheduler, never to return.
    sched();

    panic("zombie exit - kthread");
  }
}

int 
kthread_join(int ktid, uint64 status){
  struct kthread *tt;
  struct kthread *waitingfor = 0;
  int otherthreads;
  struct proc *p = myproc();

  acquire(&p->lock);

  for(;;){
    // Scan through table looking for exited children.
    otherthreads = 0;
    for(tt = p->kthread; tt < &p->kthread[NKT]; tt++){
      acquire(&tt->ktlock);
      if(tt->ktid == ktid && tt->ktstate != KTUNUSED){
        waitingfor = tt;
        otherthreads = 1;
        if(tt->ktstate == KTZOMBIE){
          if(status != 0 && copyout(p->pagetable, status, (char *)&tt->ktxstate,
                                  sizeof(tt->ktxstate)) < 0) {
            release(&tt->ktlock);
            release(&p->lock);
            return -1;
          }
          freekthread(tt);
          release(&tt->ktlock);
          release(&p->lock);
          return 0; 
        }
    }
    release(&tt->ktlock);
  }
    // No point waiting if we don't have any children.
    if(!otherthreads || mykthread()->ktkilled ==1){
      release(&p->lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(waitingfor, &p->lock);  //DOC: wait-sleep
  }
}


