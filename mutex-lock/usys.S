#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(kthread_create)
SYSCALL(kthread_join)
SYSCALL(kthread_mutex_init)
SYSCALL(kthread_mutex_destroy)
SYSCALL(kthread_mutex_lock)
SYSCALL(kthread_mutex_unlock)
SYSCALL(kthread_cond_init)
SYSCALL(kthread_cond_destroy)
SYSCALL(kthread_cond_wait)
SYSCALL(kthread_cond_signal)
SYSCALL(kthread_cond_broadcast)
SYSCALL(kthread_exit)
SYSCALL(kthread_saveretval)
SYSCALL(kthread_fetchretval)
