SECTIONS
{
    ENTRY (_entry)
    . = 0x80000000; /* ram begin address */
    .text : {
        *(.text)
        . = ALIGN(0x1000); /*  page size = 4k */
    }
    .data : {
        . = ALIGN(16);
        *(.data);
    }
    .bss : {
        *(.bss)
    }
    . = ALIGN(0x1000);
}