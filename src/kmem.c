#include "memlayout.h"
#include "conf.h"
#include "sys.h"
struct run {
    struct run* next;
};
typedef struct run run;
struct {
    spinlock_t lk;
    run* head;
} kmempool;

// this func execute in main.c, single thread env
void kinit()
{
    kmempool.head = 0;
    lock_init(&kmempool.lk,"kernel memory lock");
    for (void* cur = (void*)__KERNEL_END; cur < (void*)PHYSTOP; cur += PAGE_SIZE) {
        kfree(cur);
    }
}

void* kalloc()
{
    lock_acquire(&kmempool.lk);
    if (kmempool.head == 0) {
        lock_release(&kmempool.lk);
        return 0;
    }
    run* p = kmempool.head;
    kmempool.head = p->next;
    lock_release(&kmempool.lk);
    memset(p, 0, PAGE_SIZE);
    return (void*)p;
}

void kfree(void* p)
{
    lock_acquire(&kmempool.lk);
    run* page = (run*)p;
    page->next = kmempool.head;
    kmempool.head = page;
    lock_release(&kmempool.lk);
}