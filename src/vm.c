#include "sys.h"
#include "conf.h"
pagetable_t kernel_page_table = 0;
void kvmmake(pagetable_t pagetable);
void mappages(pagetable_t pgtable, uint64 va, uint64 pa, uint64 size, uint8 flag);
pte* walk(pagetable_t pgtable, uint64 va, uint64 alloc);
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
        pte* p = walk(pgtable, cur, 1);
        if (p == 0) {
            panic("mappages");
        }
        if (*p & PTE_V) {
            panic("remapping");
        }
        *p = PA2PTE(pa) | PTE_V | flag;
    }
}

pte* walk(pagetable_t pgtable, uint64 va, uint64 alloc)
{
    va = PAGE_ROUNDDOWN(va);
    for (int i = 2; i > 0; i--) {
        pte* p = ((pte*)pgtable + VPN(va, i));
        if (!(*p & PTE_V)) {
            if (alloc == 0) {
                return 0;
            }
            pgtable = kalloc();
            if (pgtable == 0) {
                return 0;
            }
            memset(pgtable, 0, PAGE_SIZE);
            *p = PA2PTE((int64)pgtable) | PTE_V;
        } else {
            pgtable = PTE2PA(*p);
        }
        if ((*p & 0xf) != 1) {
            /* 叶子结点 */
            return 0;
        }
    }
    return pgtable + VPN(va, 0);
}

uint64 walkaddr(pagetable_t pgtable, uint64 va)
{
    pte* p = walk(pgtable, va, 0);
    if (p == 0 || (*p & PTE_V) == 0) {
        return 0;
    }
    return PTE2PA(*p);
}

void freewalk(pagetable_t pagetable)
{
    for (pte* p = pagetable; p < pagetable + PAGE_SIZE; p++) {
        if (!(*p & PTE_V) || (*p & 0xf) != 1) {
            continue;
        }
        freewalk((pagetable_t)PTE2PA(*p));
    }
    kfree(pagetable);
}
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
    uint64 a;
    pte* _pte;
    if (newsz >= oldsz) {
        return oldsz; 
    }
    a = PAGE_ROUNDDOWN(oldsz);
    for (; a >= newsz; a -= PAGE_SIZE) {
        _pte = walk(pagetable, a, 0);
        if (_pte == 0 || !((*_pte )& PTE_V)) {
            panic("uvmdealloc\n");
        }
        kfree(PTE2PA(*_pte));
    }
    return newsz;
}

// 如果newsize小于old，do nothing and return oldsize
// if PAGE_ROUNDUP(old) >= newsize, return newsize
// else, do the normal thing
uint64 uvmalloc(pagetable_t pgtable, uint64 oldsz, uint64 newsz, uint8 xperm)
{
    pte* _pte;
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
        *_pte = PA2PTE(mem) | PTE_V | PTE_U | xperm;
    }
    return newsz;
bad:
    return uvmdealloc(pgtable, a, oldsz);
}

uint64 copyout(pagetable_t pagetable, uint64 dstva, uint64 src, uint64 size)
{
    uint64 dstpa;
    uint64 n = 0;
    uint64 offset = dstva & (PAGE_SIZE - 1);
    n = MIN(PAGE_SIZE - offset, size);
    dstpa = walkaddr(pagetable, dstva);
    if (dstpa == 0) {
        panic("copyout\n");
    }
    dstpa += offset;
    while (size) {
        memmove(dstpa, src, n);
        size -= n;
        dstva += n;
        dstpa = walkaddr(pagetable, dstva);
        if (dstpa == 0) {
            panic("copyout\n");
        }
        src += n;
        n = MIN(PAGE_SIZE, size);
    }
    return 0;
}
// todo
// void uvmcopy(old, new, size);
// void uvmfree(pagetable, size)
// {
//     uvmdealloc(pagetable, size, 0)
//     freewalk(pagetable)
// }

// void copyin(pagetable, dst, srcva , size)

// void copyinstr(pagetable, dst, srcva)
// void uvmunmap(pagetable, va, pages, dofree);
// void uvmclear(pagetable, va);
