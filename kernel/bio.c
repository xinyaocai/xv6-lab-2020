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

#define NBUCKET 13

uint
bhash(uint dev, uint blockno) {
  return (dev + blockno) % NBUCKET;
}

struct {
  struct spinlock locks[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head[i].next is most recent, head[i].prev is least.
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.locks[i], "bcache");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  for (int i = 0; i < NBUF; i++) {
    int bkt = i % NBUCKET;
    acquire(&bcache.locks[bkt]);
    b = &bcache.buf[i];
    b->next = bcache.head[bkt].next;
    b->prev = &bcache.head[bkt];
    initsleeplock(&b->lock, "buffer");
    bcache.head[bkt].next->prev = b;
    bcache.head[bkt].next = b;
    release(&bcache.locks[bkt]);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *empty = 0;

  int bkt = bhash(dev, blockno);

  acquire(&bcache.locks[bkt]);

  // Is the block already cached?
  for(b = bcache.head[bkt].next; b != &bcache.head[bkt]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
    if (b->refcnt == 0 && empty == 0) {
      empty = b;
    }
  }

  if (empty != 0) {
    empty->dev = dev;
    empty->blockno = blockno;
    empty->valid = 0;
    empty->refcnt = 1;
    release(&bcache.locks[bkt]);
    acquiresleep(&empty->lock);
    return empty;
  }

  for (int i = bhash(dev, bkt+1); i != bkt; i = bhash(dev, i+1)) {
    acquire(&bcache.locks[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.locks[i]);
        b->next = bcache.head[bkt].next;
        b->prev = &bcache.head[bkt];
        bcache.head[bkt].next->prev = b;
        bcache.head[bkt].next = b;
        release(&bcache.locks[bkt]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.locks[i]);
  }
  
  panic("bget: no buffers");
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
// Move to the head[i] of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bkt = bhash(b->dev, b->blockno);

  acquire(&bcache.locks[bkt]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bkt].next;
    b->prev = &bcache.head[bkt];
    bcache.head[bkt].next->prev = b;
    bcache.head[bkt].next = b;
  }
  
  release(&bcache.locks[bkt]);
}

void
bpin(struct buf *b) {
  int bkt = bhash(b->dev, b->blockno);
  acquire(&bcache.locks[bkt]);
  b->refcnt++;
  release(&bcache.locks[bkt]);
}

void
bunpin(struct buf *b) {
  int bkt = bhash(b->dev, b->blockno);
  acquire(&bcache.locks[bkt]);
  b->refcnt--;
  release(&bcache.locks[bkt]);
}


