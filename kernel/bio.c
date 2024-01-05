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

#define NBUCKET 7

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

} bcache[NBUCKET];

uint get_index(uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0;i < NBUCKET;i++){
    initlock(&bcache[i].lock, "bcache.bucket");
    for(b = bcache[i].buf;b < bcache[i].buf + NBUF;b++){
      initsleeplock(&b->lock, "buffer");
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b = 0;
  struct buf *LRU_b = 0;

  uint index = get_index(blockno);
  uint min_timestamp = -1;  // max value of uint

  acquire(&bcache[index].lock);

  // Is the block already cached?
  for(b = bcache[index].buf;b < bcache[index].buf + NBUF;b++){
    if(b->dev == dev && b->blockno == blockno){ // hit
      b->refcnt++;
      release(&bcache[index].lock);
      acquiresleep(&b->lock);
      return b;
    }
    // find LRU unused block in this bucket
    if(b->refcnt == 0 && b->timestamp < min_timestamp){
      min_timestamp = b->timestamp;
      LRU_b = b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  if(LRU_b != 0){
    LRU_b->dev = dev;
    LRU_b->blockno = blockno;
    LRU_b->valid = 0;
    LRU_b->refcnt = 1;
    release(&bcache[index].lock);
    acquiresleep(&LRU_b->lock);
    return LRU_b;
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
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint index = get_index(b->blockno);

  acquire(&bcache[index].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->timestamp = ticks;
  }
  
  release(&bcache[index].lock);
}

void
bpin(struct buf *b) {
  uint index = get_index(b->blockno);
  acquire(&bcache[index].lock);
  b->refcnt++;
  release(&bcache[index].lock);
}

void
bunpin(struct buf *b) {
  uint index = get_index(b->blockno);
  acquire(&bcache[index].lock);
  b->refcnt--;
  release(&bcache[index].lock);
}


