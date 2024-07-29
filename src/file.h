enum file_type {
    F_NONE,
    F_INODE,
    F_PIPE,
    F_DEVICE
};

typedef struct {
    enum file_type type;
    uint16 refcount;
    uint8 readable;
    uint8 writeable;
    inode_t* inode;
    uint16 major;
    uint32 offset;
} file_t;
void fileinit();
file_t* filealloc();
file_t* filedup(file_t* file);
void fileclose(file_t* file);
int fileread(file_t* file, uint64 buf, uint32 size);
int filewrite(file_t* file, uint64 buf, uint32 size);
// void filestat(filt_t* file);


typedef struct {
    int (*read)(int user, uint64 buf, uint32 size);
    int (*write)(int user, uint64 buf, uint32 size);
} devrw_t;

#define NDEV 10
extern devrw_t devrw[];
#define CONSOLE 1