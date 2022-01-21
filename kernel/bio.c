// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct bbucket {
  struct spinlock lock;
  struct buf *buf;
};

struct bbucket bhash[NBUCKETS];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  char sbuf[NBUCKETS][20];

  initlock(&bcache.lock, "bcache");

  // Initialize buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }

  // Initialize buffer hash table
  for(int i = 0; i < NBUCKETS; i++){
    snprintf(sbuf[i], 20, "bcache_%d", i);
    initlock(&bhash[i].lock, sbuf[i]);
  } 
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int n;

  n = hashfun(blockno);
  acquire(&bhash[n].lock);

  // Is the block already cached?
  for(b = bhash[n].buf; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bhash[n].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bhash[n].lock);
  acquire(&bcache.lock);
  uint i, lru;
  for(i = 0; i < NBUCKETS; i++)
    acquire(&bhash[i].lock);

  for(i = 0, b = 0, lru = -1; i < NBUF; i++){
    if(bcache.buf[i].dev == dev && bcache.buf[i].blockno == blockno && bcache.buf[i].refcnt > 0){
      b = &bcache.buf[i];
      b->refcnt++;
      for(int j = 0; j < NBUCKETS; j++)
        release(&bhash[j].lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
    if(bcache.buf[i].refcnt == 0 && (lru == -1 || b->stamp < lru)) {
      b = &bcache.buf[i]; 
      lru = b->stamp;
    }
  }

  if(b == 0)
    panic("bget: no buffers");

  // Remove buffer from old bucket
  int t = hashfun(b->blockno);
  if(bhash[t].buf == b){
    bhash[t].buf = b->next;
  } else {
    for(struct buf *bt = bhash[t].buf; bt != 0; bt = bt->next){
      if(bt->next == b){
        bt->next = b->next;
        break;
      }
    }
  }
  
  // Add buffer to current bucket
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->next = bhash[n].buf;
  bhash[n].buf = b;

  for(i = 0; i < NBUCKETS; i++)
    release(&bhash[i].lock);
  release(&bcache.lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int n = hashfun(b->blockno);

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bhash[n].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->stamp = ticks;
  }
  release(&bhash[n].lock);
}

void
bpin(struct buf *b) {
  acquire(&bhash[hashfun(b->blockno)].lock);
  b->refcnt++;
  release(&bhash[hashfun(b->blockno)].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bhash[hashfun(b->blockno)].lock);
  b->refcnt--;
  release(&bhash[hashfun(b->blockno)].lock);
}
