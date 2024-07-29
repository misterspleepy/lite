#include "sys.h"
#define BUF_SIZE 32
static struct {
    buf_t bufs[BUF_SIZE];
    buf_t* head;
    spinlock_t lk;
} bcache;
void binit()
{
    lock_init(&bcache.lk, "bcache");
    bcache.bufs[0].prev = &bcache.bufs[BUF_SIZE - 1];
    for (int i = 0; i < BUF_SIZE; i++) {
        buf_t* cur = &bcache.bufs[i];
        buf_t* next = &bcache.bufs[(i+1) % BUF_SIZE];
        cur->next = next;
        next->prev = cur;
        cur->ref_count = 0;
        cur->dev = 0;
        cur->block_num = 0;
        sleeplock_init(&cur->slk, "buf_lock");
        cur->is_valid = 0;
        cur->disk = 0;
        memset(&cur->data, 0, BLOCK_SIZE);
    }
    bcache.head = &bcache.bufs[0];
}

buf_t* bget(int32 dev, int32 block_num)
{
    buf_t* cur = bcache.head;
    lock_acquire(&bcache.lk);
    buf_t* condi_buf = 0;
    while (1) {
        while (cur->next != bcache.head) {
            if (cur->dev == dev && cur->block_num == block_num) {
                lock_release(&bcache.lk);
                sleeplock_acquire(&cur->slk);
                return cur;
            }
            if (cur->ref_count == 0) {
                condi_buf = cur;
            }
            cur = cur->next;
        }
        if (condi_buf) {
            break;
        }
        sleep(&bcache, &bcache.lk);
    }
    
    condi_buf->dev = dev;
    condi_buf->block_num = block_num;
    condi_buf->ref_count = 1;
    lock_release(&bcache.lk);
    sleeplock_acquire(&condi_buf->slk);
    condi_buf->is_valid = 0;
    condi_buf->disk = 0;
    return condi_buf; 
}
// bread will lock the bufs sleeplock
buf_t* bread(int32 dev, int32 block_num)
{
    buf_t* buf = bget(dev, block_num);
    if (buf->is_valid == 0) {
        virtio_disk_rw(buf, 0);
        buf->is_valid = 1;
    }
    return buf;
}

void bwrite(buf_t* buf)
{
    if (!sleeplock_holding(&buf->slk)) {
        panic("bwrite\n");
    }
    virtio_disk_rw(buf, 1);
}

void brelease(buf_t* buf)
{
    if (!sleeplock_holding(&buf->slk)) {
        panic("brelease\n");
    }
    lock_acquire(&bcache.lk);
    sleeplock_release(&buf->slk);
    buf->ref_count--;
    if (buf->ref_count == 0) {
        buf->prev->next = buf->next;
        buf->next->prev = buf->prev;
        bcache.head->prev->next = buf;
        buf->prev = bcache.head->prev;
        bcache.head->prev = buf;
        buf->next = bcache.head;
        wakeup(&bcache);
    }
    lock_release(&bcache.lk);
}

void bpin(buf_t *b)
{
    lock_acquire(&bcache.lk);
    b->ref_count++;
    lock_release(&bcache.lk);
}

void bunpin(buf_t *b)
{
    lock_acquire(&bcache.lk);
    b->ref_count--;
    lock_release(&bcache.lk);
}
