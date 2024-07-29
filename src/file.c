#include "sys.h"
#include "file.h"
#define FILE_TABLE_SIZE 120
devrw_t devrw[NDEV];

static struct {
    spinlock_t lock;
    file_t files[FILE_TABLE_SIZE];
} file_table;

void fileinit()
{
    file_t* file;
    lock_init(&file_table.lock, "file table lock");
    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        file = &(file_table.files[i]);
        file->type = F_NONE;
        file->refcount = 0;
    }
}

file_t* filealloc()
{
    file_t* file = 0;
    lock_acquire(&file_table.lock);
    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        file = &(file_table.files[i]);
        if (file->refcount == 0) {
            file->refcount = 1;
            lock_release(&file_table.lock);
            return file;
        }
    }
    lock_release(&file_table.lock);
    return 0;
}

void fileclose(file_t* file)
{
    file_t ff;
    lock_acquire(&file_table.lock);
    if (file->refcount < 1) {
        panic("fileclose");
    }
    file->refcount--;
    if (file->refcount > 0) {
        lock_release(&file_table.lock);
        return;
    }
    ff = *file;
    file->type = F_NONE;
    file->inode = 0;
    lock_release(&file_table.lock);
    if (ff.type == F_INODE && ff.inode) {
        iput(ff.inode);
        file->inode = 0;
    }
    return;
}

file_t* filedup(file_t* file)
{
    lock_acquire(&file_table.lock);
    if (file->refcount < 1) {
        panic("filedup");
    }
    file->refcount += 1;
    lock_release(&file_table.lock);
    return file;
}

// buf is from user virtual memory
int fileread(file_t* file, uint64 buf_uvm, uint32 size)
{
    if (file->refcount < 1) {
        panic("fileread");
    }
    if (file->type == F_DEVICE) {
        if (file->major < 0 || file->major >= NDEV || devrw[file->major].read == 0) {
            return -1;
        }
        return devrw[file->major].read(1, buf_uvm, size);
    }
    return -1;
}

// buf is from user virtual memory
int filewrite(file_t* file, uint64 buf_uvm, uint32 size)
{
    if (file->refcount < 1) {
        panic("filewrite");
    }
    if (file->type == F_DEVICE) {
        if (file->major < 0 || file->major >= NDEV || (uint64)devrw[file->major].write == 0) {
            return -1;
        }
        return devrw[file->major].write(1, buf_uvm, size);
    }
    return -1;
}
