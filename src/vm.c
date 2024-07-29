#include "sys.h"
#include "conf.h"
pagetable_t kernel_page_table = 0;
void kvmmake(pagetable_t pagetable);
void mappages(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, uint8 flag);
pte_t* walk(pagetable_t pgtable, uint64 va, uint64 alloc);
uint64 walkaddr(pagetable_t pgtable, uint64 va);
void freewalk(pagetable_t pagetable);
void kvminit()
{
    if (kernel_page_table) {
        panic("kvminit\n");
    }
    kernel_page_table = kalloc();
    if (kernel_page_table == 0) {
        panic("kvminit failed, can't allocate memmory.\n");
    }
    kvmmake(kernel_page_table);
    sfence_vma();
    w_satp(MAKE_SAPT(kernel_page_table));
    sfence_vma();
}

void kvmmake(pagetable_t pagetable)
{
    mappages(pagetable, CLINI_BASE, CLINI_BASE, CLINI_SIZE, PTE_R | PTE_W | PTE_X);
    mappages(pagetable, PLIC_BASE, PLIC_BASE, PLIC_SIZE, PTE_W | PTE_X | PTE_R);
    mappages(pagetable, UART0_BASE, UART0_BASE, UART0_SIZE, PTE_W | PTE_X | PTE_R);
    mappages(pagetable, VIRTIO_BASE, VIRTIO_BASE, VIRTIO_SIZE, PTE_W | PTE_X | PTE_R);
    mappages(pagetable, (uint64)__TEXT_BEGIN, (uint64)__TEXT_BEGIN, (uint64)((void*)__TEXT_END - (void*)__TEXT_BEGIN), PTE_X | PTE_R);
    mappages(pagetable, (uint64)__DATA_BEGIN, (uint64)__DATA_BEGIN, (uint64)((void*)__DATA_END - (void*)__DATA_BEGIN), PTE_W | PTE_R);
    mappages(pagetable, (uint64)__DATA_END, __DATA_END, (uint64)(PHYSTOP - (uint64)__DATA_END), PTE_R | PTE_W | PTE_X);
    mappages(pagetable, (uint64)TRAMPOLINE, (uint64)__TRAMPOLINE, PAGE_SIZE, PTE_R | PTE_X);
}

// va and pa should be PAGE alignment
void mappages(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, uint8 flag)
{
    for (uint64 cur = va; cur < va + size; cur += PAGE_SIZE, pa += PAGE_SIZE) {
        pte_t* p = walk(pgtable, cur, 1);
        if (p == 0) {
            panic("mappages");
        }
        if (*p & PTE_V) {
            panic("remapping");
        }
        *p = PA2PTE(pa) | PTE_V | PTE_R | flag;
    }
}

pte_t* walk(pagetable_t pgtable, uint64 va, uint64 alloc)
{
    va = PAGE_ROUNDDOWN(va);
    for (int i = 2; i > 0; i--) {
        pte_t* p = ((pte_t*)pgtable + VPN(va, i));
        if (!(*p & PTE_V)) {
            if (alloc == 0) {
                return 0;
            }
            pgtable = kalloc();
            if (pgtable == 0) {
                return 0;
            }
            memset(pgtable, 0, PAGE_SIZE);
            *p = PA2PTE((uint64)pgtable) | PTE_V;
        } else {
            pgtable = PTE2PA(*p);
        }
        if ((*p & (PTE_R | PTE_W | PTE_X)) != 0) {
            /* 叶子结点 */
            return 0;
        }
    }
    return pgtable + VPN(va, 0);
}

// Return 0 if failed
uint64 walkaddr(pagetable_t pgtable, uint64 va)
{
    pte_t* p = walk(pgtable, va, 0);
    if (p == 0 || (*p & PTE_V) == 0) {
        return 0;
    }
    return PTE2PA(*p);
}

void freewalk(pagetable_t pagetable)
{
    for (pte_t* p = pagetable; (uint64)p < (uint64)pagetable + PAGE_SIZE; p++) {
        if ((*p & PTE_V) && (*p & (PTE_R|PTE_W|PTE_X)) == 0) {
            freewalk((pagetable_t)PTE2PA(*p));
            *p = 0;
        } else if(*p & PTE_V) {
            panic("freeewalk: leaf");
        }
    }
    kfree(pagetable);
}
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
    if(newsz >= oldsz)
        return oldsz;

    if(PAGE_ROUNDUP(newsz) < PAGE_ROUNDUP(oldsz)){
        int npages = (PAGE_ROUNDUP(oldsz) - PAGE_ROUNDUP(newsz)) / PAGE_SIZE;
        uvmunmap(pagetable, PAGE_ROUNDUP(newsz), npages, 1);
    }
    return newsz;
}

// 如果newsize小于old，do nothing and return oldsize
// if PAGE_ROUNDUP(old) >= newsize, return newsize
// else, do the normal thing
uint64 uvmalloc(pagetable_t pgtable, uint64 oldsz, uint64 newsz, uint8 xperm)
{
    pte_t* _pte;
    uint64 mem;
    uint64 a;
    a = PAGE_ROUNDUP(oldsz);
    if (newsz <= a) {
        return oldsz;
    }
    for (; a < newsz; a += PAGE_SIZE) {
        _pte = walk(pgtable, a, 1);
        if (_pte == 0) {
            goto bad;
        }
        mem = (uint64)kalloc();
        if (mem == 0) {
            goto bad;
        }
        *_pte = PA2PTE(mem) | PTE_V | PTE_U | PTE_R | xperm;
    }
    return newsz;
bad:
    return uvmdealloc(pgtable, a, oldsz);
}

// Return 0 on success, -1 on error.
uint64 copyout(pagetable_t pagetable, uint64 dstva, uint64 src, uint64 size)
{
    uint64 dstpa;
    uint64 n = 0;
    uint64 offset = dstva & (PAGE_SIZE - 1);
    n = MIN(PAGE_SIZE - offset, size);
    dstpa = walkaddr(pagetable, dstva);
    if (dstpa == 0) {
        return -1;
    }
    dstpa += offset;
    while (size) {
        memmove(dstpa, src, n);
        size -= n;
        dstva += n;
        dstpa = walkaddr(pagetable, dstva);
        if (dstpa == 0) {
            return -1;
        }
        src += n;
        n = MIN(PAGE_SIZE, size);
    }
    return 0;
}

// 0 success, -1 failed
uint64 copyinstr(pagetable_t pg, uint64 dst, uint64 srcva, uint64 max)
{
    uint64 copysize = 0;
    uint64 offset = srcva & (PAGE_SIZE - 1);
    uint64 sz = MIN(PAGE_SIZE - offset, max);
    uint64 i;
    char* begin = (char*)walkaddr(pg, srcva) + offset;
    while (1) {
        for (i = 0; i < sz; i++) {
            if (*(begin + i) == 0) {
                i++;
                break;
            }
        }
        memmove((void*)dst, begin, i);
        copysize += i;
        if (i != sz) {
            return copysize;
        }
        dst += i;
        srcva += i;
        max -= i;
        begin = (char*)walkaddr(pg, srcva);
        if (begin == 0) {
            return -1;
        }
        sz = MIN(PAGE_SIZE, max);
    }
    return -1;
}

uint64 copyin(pagetable_t pg, uint64 dst, uint64 srcva, uint64 size)
{
    uint64 offset = srcva & (PAGE_SIZE - 1);
    uint64 n = MIN(PAGE_SIZE - offset, size);
    uint64 srcpa = walkaddr(pg, srcva);
    if (srcpa == 0) {
        return -1;
    }
    while (size) {
        memmove((void*)dst, (void*)srcpa + offset, n);
        size -= n;
        dst += n;
        srcva += n;
        srcpa = walkaddr(pg, srcva);
        if (srcpa == 0) {
            return -1;
        }
        n = MIN(PAGE_SIZE, size);
    }
    return 0;
}

uint64 eithercopyout(short user, uint64 dst, uint64 src, uint64 size)
{
    if (user == 1) {
        return copyout(myproc()->pagetable, dst, src, size);
    } else {
        memmove((void*)dst, (void*)src, size);
    }
    return 0;
}

uint64 eithercopyin(short user, uint64 dst, uint64 src, uint64 size)
{
    if (user == 1) {
        return copyin(myproc()->pagetable, dst, src, size);
    }
    memmove((void*)dst, (void*)src, size);
    return 0;
}

void uvmfree(pagetable_t ptb, uint64 size)
{
    if (uvmdealloc(ptb, size, 0) != 0) {
        panic("uvmfree");
    }
    uvmunmap(ptb, TRAMPOLINE, 1, 0);
    uvmunmap(ptb, TRAP_FRAME, 1, 0);
    freewalk(ptb);
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
    uint64 a;
    pte_t* pte;

    if((va % PAGE_SIZE) != 0) {
        panic("uvmunmap: not aligned");
    }
    for(a = va; a < va + npages * PAGE_SIZE; a += PAGE_SIZE){
        if((pte = walk(pagetable, a, 0)) == 0) {
            panic("uvmunmap: walk");
        }
        if((*pte & PTE_V) == 0) {
            panic("uvmunmap: not mapped");
        }
        if(PTE_FLAGS(*pte) == PTE_V) {
            panic("uvmunmap: not a leaf");
        }
        if(do_free){
            uint64 pa = PTE2PA(*pte);
            kfree((void*)pa);
        }
        *pte = 0;
    }
}
// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void uvmclear(pagetable_t pagetable, uint64 va)
{
    pte_t *pte;
    
    pte = walk(pagetable, va, 0);
    if(pte == 0) {
        panic("uvmclear");
    }
    *pte &= ~PTE_U;
}
// todo
// void uvmcopy(old, new, size);
