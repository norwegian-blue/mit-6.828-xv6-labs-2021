// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);
static void* steal(int);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct cpufreelist {
  struct spinlock lock;
  struct run *freelist;
};

struct cpufreelist kmem;

struct cpufreelist cpufreemem[NCPU];

void
kinit()
{
  //initlock(&kmem.lock, "kmem");
  //freerange(end, (void*)PHYSTOP);
  char buf[NCPU][20];

  int i;
  for(i = 0; i < NCPU; i++) {
    snprintf(buf[i], 20, "kmem_cpu%d", i);
    initlock(&cpufreemem[i].lock, buf[i]);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  push_off();
  int n = cpuid();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&cpufreemem[n].lock);
  r->next = cpufreemem[n].freelist;
  cpufreemem[n].freelist = r;
  release(&cpufreemem[n].lock);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();

  int n = cpuid();

  acquire(&cpufreemem[n].lock);
  r = cpufreemem[n].freelist;
  if(r == 0)
    r = steal(n);
  if(r)
    cpufreemem[n].freelist = r->next;
  release(&cpufreemem[n].lock);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Steal free memory from another CPU.
// Search through list of free memory
// structures and steal entire free memory.
static void*
steal(int n)
{
  int i;
  for (i = 0; i < NCPU; i++) {
    if(i == n)
      continue;
    acquire(&cpufreemem[i].lock);
    if (cpufreemem[i].freelist){
      cpufreemem[n].freelist = cpufreemem[i].freelist;
      cpufreemem[i].freelist = 0;
      release(&cpufreemem[i].lock);
      break;
    }
    release(&cpufreemem[i].lock);
  }
  return cpufreemem[n].freelist; 
}
