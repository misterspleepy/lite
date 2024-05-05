SECTIONS
{
    ENTRY (_entry)
    . = 0x80000000; /* ram begin address */
    PROVIDE(__TEXT_BEGIN = .);
    .text : {
        *(.text)
        . = ALIGN(0x1000); /*  page size = 4k */
        __TRAMPOLINE = .;
        *(tramsec);
        . = ALIGN(0x1000);
        ASSERT(. - __TRAMPOLINE == 0x1000, "error: trampoline larger than one page");
    }
    PROVIDE(__TEXT_END = .);
    PROVIDE(__DATA_BEGIN = .);
    .data : {
        . = ALIGN(16);
        *(.data .sdata);
    }
    .bss : {
        *(.bss .sbss)
    }
    . = ALIGN(0x1000);
    PROVIDE(__DATA_END = .);
    PROVIDE(__KERNEL_END = .);
}