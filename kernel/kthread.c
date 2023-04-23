#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;
extern void forkret(void);


void 
kthreadinit(struct proc *p)
{


  acquire(&wait_lock);
  acquire(&p->lock);

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++){
    initlock(&kt->ktlock, "kthread_lock");

    acquire(&kt->ktlock); 

    kt->ktstate = UNUSED;
    kt->proc = p;
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));

    release(&kt->ktlock);
  }

  release(&p->lock);
  release(&wait_lock);
}


struct kthread *
mykthread() // ??
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
    ktpid = p->ktidcounter ;
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
    if(kt->ktstate == UNUSED){
      goto found;
    }
    else{
      release(&kt->ktlock);
    }
  }

  return 0;  
  found:
  kt->ktid = alloctid(p);

  kt->ktstate = USED;

  kt->trapframe = get_kthread_trapframe(p, kt);

  memset(&kt->ktcontext, 0, sizeof(kt->ktcontext));
  kt->ktcontext.ra = (uint64)forkret;
  kt->ktcontext.sp = kt->kstack + PGSIZE;


  return kt;

  }

void
 freekthread(struct kthread *kt){

  kt->ktid = 0;
  kt->ktchan = 0;
  kt->ktkilled = 0;
  kt->ktxstate = 0;
  kt->proc = 0; // ??
  kt->trapframe = 0; // ??
  kt->kstack = 0; // do we need to free the stack?
  kt->ktstate = UNUSED;
  // what about the trapframe and the context?

 }

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}