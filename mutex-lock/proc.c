#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }

  if(proc->type)
   proc->parent->sz = sz;

  proc->sz = sz;
  
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(char *ustack, uint thrdattr, uint wrapperaddr, uint arg1, uint arg2)
{
  int i, j, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  if(proc->ctflag){

  	np->pgdir = proc->pgdir;
    	np->type = 1;
	np->threadattr = thrdattr;
	np->threadretval = -1;

  } else {

    	// Copy process state from p.
    	if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
      		kfree(np->kstack);
      		np->kstack = 0;
      		np->state = UNUSED;
      		return -1;
    	}

    	np->type = 0;
	np->tcount = 0;
  }

  for(i=0;i<NMUTX;i++){
    np->mutexlist[i].lockingthread = -1;
    np->mutexlist[i].lock.id = -1;	
  }

  for(i=0;i<NCONDVAR;i++){

    np->condvarlist[i].id = -1;

    for(j=0;j<MAXTHRDS;j++)
      np->condvarlist[i].waitingthreadlist[j] = -1;
  }

  np->sz = proc->sz;


//for threads invoking threads
  if(proc->type && proc->ctflag)
    np->parent = proc->parent;
  else
    np->parent = proc;

  *np->tf = *proc->tf;

if(proc->ctflag){

  np->tf->eip = wrapperaddr;

  proc->ustack = ustack;

  uint *sp = (uint*)proc->ustack;

  *(--sp) = arg2;
  *(--sp) = arg1;
  *(--sp) = 0;

  np->tf->esp = (uint)sp;
  proc->tcount+=1;
}

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;


  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i]){
	if(proc->ctflag)
		np->ofile[i] = proc->ofile[i];
	else
		np->ofile[i] = filedup(proc->ofile[i]);
  }

  if(proc->ctflag)
	np->cwd = proc->cwd;
  else
  	np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;

}

void cleanupzombiethreads(){

//	cprintf("tcount: %d\n", proc->tcount);	
//	cprintf("proc: %p\n", proc);
//	cprintf("proc pid: %d\n", proc->pid);
        struct proc *p;
	int i;

        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

                if(p->type && (p->state == ZOMBIE) && (p->parent == proc)){

		 	p->parent->tcount-=1;
                        p->state = UNUSED;
                        p->pid = 0;
                        p->parent = 0;
                        p->name[0] = 0;
                        p->killed = 0;
                        p->wrapper = 0;
                        p->ctflag = 0;
                        p->ustack = 0;
                        p->type = 0;
                        p->threadattr = 0;
                        p->threadretval = 0;
                        p->chan = 0;
                        kfree(p->kstack);
                        p->kstack = 0;

                        for(i=0;i<NMUTX;i++)
                             (p->mutexlist[i]).lock.id = -1;

                        for(i=0;i<NCONDVAR;i++)
                             (p->condvarlist[i]).id = -1;

                        break;

	//		release(&ptable.lock);
	//		int val = wait(p->pid);
	//		if(val != -1)
	//			getthreadretval(val);
	//		acquire(&ptable.lock);
                }
        }

        release(&ptable.lock);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{

  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      if(!proc->type){
        fileclose(proc->ofile[fd], 1);
	}
      proc->ofile[fd] = 0;
    }
  }

  if(!proc->type){
  begin_op();
  iput(proc->cwd);
  end_op();
}
  proc->cwd = 0;

  acquire(&ptable.lock);

//  // Parent might be sleeping in wait().
  //wakeup1(proc->parent);

  if(proc->type == 0){
  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if((p->type != 1) && (p->parent == proc)){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }
 }

  if(proc->type && proc->threadattr){
        proc->pgdir = 0;
  }

  while(proc->tcount != 0){
//	cprintf("tcount: %d\n", proc->tcount);
	release(&ptable.lock);
	cleanupzombiethreads();
	yield();
	acquire(&ptable.lock);

  }

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint tid) {
  struct proc *p;
  int havekids, pid, i;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(((p->parent != proc) && !proc->type) || (proc->type && (p->parent != proc->parent))) 
        continue;
      
      if(tid)
	if(tid != p->pid)
	  continue;
     
      havekids = 1;

      if(p->state == ZOMBIE){
        // Found one.

        pid = p->pid;

	if(!p->type){
           freevm(p->pgdir);
	   p->pgdir = 0;
           p->state = UNUSED;
	   p->pid = 0;
	   p->parent = 0;
           p->name[0] = 0;
           p->killed = 0;
	   p->tcount = 0;
	   p->wrapper = 0;
	   p->ctflag = 0;
	   p->ustack = 0;
	   p->type = 0;
	   p->threadattr = 0;
	   p->threadretval = 0;
	   p->chan = 0;
	   kfree(p->kstack);
           p->kstack = 0;

		
	   for(i=0;i<NMUTX;i++)
	     p->mutexlist[i].lock.id = -1;

	   for(i=0;i<NCONDVAR;i++)
	     p->condvarlist[i].id = -1;

	}

        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    if(tid){
	release(&ptable.lock);
	yield();
	acquire(&ptable.lock);
    }
    else
	sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE){
        continue;
	}

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  //cprintf("akkar bakkar");
  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  //cprintf("proc id: %d\n", proc->pid);
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    initlog();
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

void
modified_sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleeping on condvar without mutex");

    acquire(&ptable.lock);  //DOC: sleeplock1
    xchg(&lk->locked, 0);

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
    release(&ptable.lock);
    while(xchg(&lk->locked, 1) != 0);
}


//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


int getthreadretval(int tid){

	int retval = -1;
	struct proc *p;
	int i;

	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      		if(p->type && (p->pid == tid)){
			retval = p->threadretval;
			if(p->threadattr > 0){
				p->threadattr = 0;
				p->threadretval = 0;
				break;
			}

	                p->parent->tcount-=1;
	                p->pgdir = 0;
	                p->state = UNUSED;
	                p->pid = 0;
	                p->parent = 0;
	                p->name[0] = 0;
	                p->killed = 0;
	                p->wrapper = 0;
	                p->ctflag = 0;
	                p->ustack = 0;
	                p->type = 0;
	                p->threadattr = 0;
	                p->threadretval = 0;
	                p->chan = 0;
		        kfree(p->kstack);
		        p->kstack = 0;

	                for(i=0;i<NMUTX;i++)
	        	     (p->mutexlist[i]).lock.id = -1;

	                for(i=0;i<NCONDVAR;i++)
		             (p->condvarlist[i]).id = -1;

			break;
		}		
	}
	release(&ptable.lock);

	return retval;
}

int mutex_init(){

	int retval = -1;
	int i;

	acquire(&ptable.lock);
	for(i=0;i<NMUTX;i++){
		if(proc->mutexlist[i].lock.id == -1){
			proc->mutexlist[i].lock.id = (i+1);
			initlock(&proc->mutexlist[i].lock, (char*)(i+1));
			proc->mutexlist[i].lockingthread = 0;
			retval = (i+1);
			break;
		}
	}
	release(&ptable.lock);

	return retval;
}

int mutex_destroy(int mutex){

	int retval = -1;

	acquire(&ptable.lock);
	if((mutex > 0) && (mutex <= NMUTX) && (proc->mutexlist[mutex-1].lock.id == mutex)){
		proc->mutexlist[mutex-1].lock.id = -1;
		initlock(&proc->mutexlist[mutex-1].lock, (char*)-1);
		proc->mutexlist[mutex-1].lockingthread = -1;
		retval = mutex;
	}
	release(&ptable.lock);

	return retval;
}

int mutex_lock(int mutex){

	int retval = -1;

	if((proc->type) && (mutex > 0) && (mutex <= NMUTX) && (proc->parent->mutexlist[mutex-1].lock.id == mutex)){
		while(proc->parent->mutexlist[mutex-1].lock.locked);
		acquire(&ptable.lock);
		while(xchg(&proc->parent->mutexlist[mutex-1].lock.locked, 1) != 0);
		proc->parent->mutexlist[mutex-1].lockingthread = mutex;	
		release(&ptable.lock);
		retval = mutex;
	}

	return retval;

}

int mutex_unlock(int mutex){

	int retval = -1;

	if((proc->type)&& (mutex > 0) && (mutex <= NMUTX) && (proc->parent->mutexlist[mutex-1].lock.id == mutex)){
		acquire(&ptable.lock);
		xchg(&proc->parent->mutexlist[mutex-1].lock.locked, 0);		
		proc->parent->mutexlist[mutex-1].lockingthread = 0;
		release(&ptable.lock);
		retval = mutex;
	}

	return retval;
}

int condvar_init(){

        int i, flag = -1;
	
	acquire(&ptable.lock);
        for(i=0;i<NCONDVAR;i++){
                if(proc->condvarlist[i].id == -1){
                        flag = (i+1);
                        proc->condvarlist[i].id = (i+1);
                        int j;
                        for(j=0;j<MAXTHRDS;j++)
                                proc->condvarlist[i].waitingthreadlist[j] = 0;
                        break;
                }
        }
	release(&ptable.lock);

        return flag;
}

int condvar_destroy(int condvar){

	int retval = -1;

	acquire(&ptable.lock);
	if((condvar > 0) && (condvar <= NCONDVAR) && (proc->condvarlist[condvar-1].id == condvar)){
              
	        proc->condvarlist[condvar-1].id = -1;
        	int j;
	        for(j=0;j<MAXTHRDS;j++)
        	        proc->condvarlist[condvar-1].waitingthreadlist[j] = -1;
		retval = condvar;
	}
	release(&ptable.lock);

	return retval;
}

int condvar_wait(int condvar, int mutex){

	int retval = -1;

	if((condvar > 0) && (condvar <= NMUTX) && (mutex > 0) && (condvar <= NCONDVAR)){
                int i;
                for(i=0;i<MAXTHRDS;i++){
                        if(proc->parent->condvarlist[condvar-1].waitingthreadlist[i] == 0){
				acquire(&ptable.lock);
                                proc->parent->condvarlist[condvar-1].waitingthreadlist[i] = proc->pid;
				release(&ptable.lock);
                                modified_sleep((void*)proc->pid, &proc->parent->mutexlist[mutex-1].lock);
                                retval = 0;
				break;
                        }
                }
        }

        return retval;
}

int condvar_signal(int condvar){

	int i;

        if((condvar < 1) || (condvar > NCONDVAR))
                return -1;

        for(i=0;i<MAXTHRDS;i++){

                if(proc->type){
                        if(proc->parent->condvarlist[condvar-1].waitingthreadlist[i] > 0){
                                wakeup((void*)proc->parent->condvarlist[condvar-1].waitingthreadlist[i]);
				acquire(&ptable.lock);
                                proc->parent->condvarlist[condvar-1].waitingthreadlist[i] = 0;
                                release(&ptable.lock);
                                break;
                        }
                } else {

                        if(proc->condvarlist[condvar-1].waitingthreadlist[i] > 0){
                                wakeup((void*)proc->condvarlist[condvar-1].waitingthreadlist[i]);
                                acquire(&ptable.lock);
                                proc->condvarlist[condvar-1].waitingthreadlist[i] = 0;
                                release(&ptable.lock);
                                break;
                        }
                }
        }

        return 0;
}

int condvar_broadcast(int condvar){

	int i;

        if((condvar < 1) || (condvar > NCONDVAR))
             return -1;

        for(i=0;i<MAXTHRDS;i++){

                if(proc->type){
                        if(proc->parent->condvarlist[condvar-1].waitingthreadlist[i] > 0){
                                wakeup((void*)proc->parent->condvarlist[condvar-1].waitingthreadlist[i]);
                                acquire(&ptable.lock);
                                proc->parent->condvarlist[condvar-1].waitingthreadlist[i] = 0;
                                release(&ptable.lock);
                        }
                } else {

                        if(proc->condvarlist[condvar-1].waitingthreadlist[i] > 0){
                                wakeup((void*)proc->condvarlist[condvar-1].waitingthreadlist[i]);
                                acquire(&ptable.lock);
                                proc->condvarlist[condvar-1].waitingthreadlist[i] = 0;
                                release(&ptable.lock);
                        }
                }
        }

        return 0;
}

