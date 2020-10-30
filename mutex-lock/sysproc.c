#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "proc.h"
int
sys_fork(void)
{
  if(proc->type || proc->tcount)
	return -1;

  proc->ctflag = 0;
  return fork((char*)0, 0, 0, 0, 0);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait(0);
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_getppid(void) {

	if(proc->parent == 0)
		return 0;
	return proc->parent->pid;
}

int sys_kthread_create(void){

	int ustack = 0;
	int thrdattr = 0;
	int wrapper = 0;
	int arg1 = 0;
	int arg2 = 0;

	argint(0, &ustack);
	argint(1, &thrdattr);
	argint(2, &wrapper);
	argint(3, &arg1);
	argint(4, &arg2);

	proc->ctflag = 1;
	return fork((char*)ustack, thrdattr, (uint)wrapper, (uint)arg1, (uint)arg2);
}

int sys_kthread_join(void){

	int tid = 0;

	argint(0, &tid);

	return wait(tid);
}


int sys_kthread_saveretval(void){

	int retval;
	argint(0, &retval);
	proc->threadretval = retval;
	return 0;
}


int sys_kthread_fetchretval(void){

	int thread = 0;

	argint(0, &thread);

	return getthreadretval(thread);
}

int sys_kthread_mutex_init(void){

	return mutex_init();
}

int sys_kthread_mutex_destroy(void){

	int mutex;
	argint(0, &mutex);

	return mutex_destroy(mutex);		
}

int sys_kthread_mutex_lock(void){

	int mutex = 0;
	argint(0, &mutex);

	return mutex_lock(mutex);
}

int sys_kthread_mutex_unlock(void){

	int mutex = 0;
	argint(0, &mutex);

	return mutex_unlock(mutex);
}

int sys_kthread_cond_init(void){

	return condvar_init();
}

int sys_kthread_cond_destroy(void){

	int condvar = 0;
	
	argint(0, &condvar);

	return condvar_destroy(condvar);	
}

int sys_kthread_cond_wait(void){

	int mutex = 0 , condvar = 0; 

	argint(0, &condvar);
	argint(1, &mutex);

	return condvar_wait(condvar, mutex);
}

int sys_kthread_cond_signal(void){

	int condvar = 0;

	argint(0, &condvar);

	return condvar_signal(condvar);
}

int sys_kthread_cond_broadcast(void){

	int  condvar = 0;
	
	argint(0, &condvar);
	
	return condvar_broadcast(condvar);
}

int sys_kthread_exit(void){
	int retval = 0;
	argint(0, &retval);
	proc->threadretval = retval;
	exit();
	return 0; //not reached
}
