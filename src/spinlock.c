#include "sys.h"

void lock_init(spinlock* lk, char* name)
{
    lk->cpu = 0;
    lk->locked = 0;
    lk->name = name;
}

void lock_acquire(spinlock* lk)
{
    push_off();
    while(__sync_lock_test_and_set(&lk->locked, 1)) {
        ;
    }
    __sync_synchronize();
    lk->cpu = (uint64)mycpu();
}

void lock_release(spinlock* lk)
{
    if (lock_holding(lk) == 0) {
        panic("lock_release\n");
    }
    lk->cpu = 0;
    __sync_synchronize();
    __sync_lock_release(&lk->locked);
    pop_off();
}

uint64 lock_holding(spinlock* lk)
{
    return lk->cpu == (uint64)mycpu();
}

void push_off()
{
    if (mycpu()->noff == 0) {
        mycpu()->sie = r_sstatus() & SSTATUS_SIE;
        dis_intr();
    }
    mycpu()->noff++;
}

void pop_off()
{
    if (mycpu()->noff < 0 || (r_sstatus() & SSTATUS_SIE)) {
        panic("pop_off\n");
    }
    if (--mycpu()->noff == 0) {
        w_sstatus(r_sstatus() | mycpu()->sie);
    }
}