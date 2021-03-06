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

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int pagecount[PHYSTOP/PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  memset(kmem.pagecount, 0, sizeof(kmem.pagecount)/sizeof(int));
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

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  // Decrease page counter and skip if page counter is not zero (COW page still in use)
  acquire(&kmem.lock);
  kmem.pagecount[PGCOUNT(pa)]--;
  if (kmem.pagecount[PGCOUNT(pa)] > 0) {
    release(&kmem.lock);
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.pagecount[PGCOUNT((void*)r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}

// increase the page counter for the specified physical address
void
increasepagecount(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.pagecount[PGCOUNT(pa)]++;
  release(&kmem.lock);
}

// decrease the page counter for the specified physical address
void
decreasepagecount(uint64 pa)
{
  kmem.pagecount[PGCOUNT(pa)]--;
  if (kmem.pagecount[PGCOUNT(pa)] < 0) {
    panic("cow page reference < 0");
  }
}

// count the number of page references to a specific physical address
int
countpagerefs(uint64 pa)
{
  return kmem.pagecount[PGCOUNT(pa)];
}