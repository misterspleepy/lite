#include "sys.h"

void lock_init(spinlock_t* lk, const char* name)
{
    lk->cpu = 0;
    lk->locked = 0;
    lk->name = name;
}

void lock_acquire(spinlock_t* lk)
{
    push_off();
    while(__sync_lock_test_and_set(&lk->locked, 1)) {
        ;
    }
    __sync_synchronize();
    lk->cpu = (uint64)mycpu();
}

void lock_release(spinlock_t* lk)
{
    if (lock_holding(lk) == 0) {
        panic("lock_release\n");
    }
    lk->cpu = 0;
    __sync_synchronize();
    __sync_lock_release(&lk->locked);
    pop_off();
}

uint64 lock_holding(spinlock_t* lk)
{
    return lk->cpu == (uint64)mycpu();
}

void push_off()
{
    if (mycpu() == 0) {
        panic("push_off");
    }
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