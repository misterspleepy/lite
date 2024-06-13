#include "sys.h"
void sleeplock_init(sleeplock_t* lk, const char* name)
{
    lk->locked = 0;
    lk->myproc = 0;
    lk->name = name;
    lock_init(&lk->spinlock, name);
}

void sleeplock_acquire(sleeplock_t* lk)
{
    lock_acquire(&lk->spinlock);
    if (sleeplock_holding(lk) == 1) {
        panic("sleeplock_acquire\n");
    }
    while (lk->locked == 1) {
        sleep(lk, &lk->spinlock);
    }
    lk->locked = 1;
    lk->myproc = myproc();
    lock_release(&lk->spinlock);
}

void sleeplock_release(sleeplock_t* lk)
{
    lock_acquire(&lk->spinlock);
    if (!sleeplock_holding(lk)) {
        panic("sleeplock release\n");
    }
    lk->locked = 0;
    lk->myproc = 0;
    lock_release(&lk->spinlock);
    wakeup(lk);
}

// need holding lk->spinlock
int32 sleeplock_holding(sleeplock_t* lk)
{
    return lk->myproc == myproc();
}