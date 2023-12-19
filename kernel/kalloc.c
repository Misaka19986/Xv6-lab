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
} kmem;

struct {
  struct spinlock lock;
  uint counter[(PHYSTOP - KERNBASE) / PGSIZE];
} parefcnt; // cow counter

/*
  operations to parefcnt
*/
void
ref_lock_init()
{
  initlock(&parefcnt.lock, "parefcnt");
}

uint64
get_pa_index(uint64 pa)
{
  return (pa - KERNBASE) / PGSIZE;
}

void
acquire_ref_lock()
{
  acquire(&parefcnt.lock);
}

void
release_ref_lock()
{
  release(&parefcnt.lock);
}

uint
get_ref_cnt(uint64 pa)
{
  return parefcnt.counter[get_pa_index(pa)];
}

// it must be used when holding the lock
void
set_ref_cnt(uint64 pa, uint n)
{
  parefcnt.counter[get_pa_index(pa)] = n;
}

//  add given n to counter
//  it must be used when holding the lock
//  input should be positive
int
add_ref_cnt(uint64 pa, uint n)
{
  uint num = get_ref_cnt(pa) + n;
  if(num < 0){
    printf("parefcnt: wrong count\n");
    return -1;
  }
  parefcnt.counter[get_pa_index(pa)] += n;
  return 0;
}

void
kinit()
{
  ref_lock_init();
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    set_ref_cnt((uint64)p, 1);
    kfree(p);
  }
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

  // when the ref > 1, pa should not be freed
  acquire_ref_lock();
  uint ref = get_ref_cnt((uint64)pa);
  if(ref > 1){
    set_ref_cnt((uint64)pa, ref - 1);
    release_ref_lock();
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  set_ref_cnt((uint64)pa, 0);
  release_ref_lock();

  r = (struct run*)pa;

  acquire(&kmem.lock);
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
  if(r){
    kmem.freelist = r->next;
  }
    
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
  }

  if(r){
    set_ref_cnt((uint64)r, 1);
  }
  
  return (void*)r;
}
