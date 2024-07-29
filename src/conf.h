#define NCPU         1
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define NPROC        100
#define MAXFILE      128
#define FSSIZE       2000  // size of file system in blocks
#define PATHMAX      128
#define ARGSMAX      32