#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // starting virtual address
  uint64 v0;
  if (argaddr(0, &v0) < 0) {
    return -1;
  }

  // # of pages to check
  int n;
  if (argint(1, &n) < 0) {
    return -1;
  }

  // accessed pages bitmask return virtual address
  uint64 p;
  if (argaddr(2, &p) < 0) {
    return -1;
  }

  // get accessed tables
  uint64 bmask = 0;
  for (int i = 0; i < n; i++) {
    pte_t *pte = walk(myproc()->pagetable, v0+i*PGSIZE, 0);
    if (*pte & PTE_A){
      bmask |= (1L << i);
    }
    *pte &= ~PTE_A;
  }

  // copy back to userspace
  if (copyout(myproc()->pagetable, p, (char*)&bmask, sizeof(bmask)) < 0) {
    return -1;
  }

  return 0;
  
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
