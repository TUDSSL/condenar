/******************************************************************************
 *
 * linker_script.ld - Linker script for applications using startup_gnu.c
 *
 *****************************************************************************/
ENTRY(Reset_Handler)

MEMORY
{
    MCU_MRAM     (rx)  : ORIGIN = 0x00018000, LENGTH = 1300K
    MRAM_APPSTORAGE    :    ORIGIN = 0x00018000 + 1M, LENGTH = 100K /*Start + position should be 16 byte aligned*/
    MRAM_SAVESTATE     :    ORIGIN = 0x00018000 + 1M + 100K, LENGTH = 500K
    
    MCU_TCM      (rwx) : ORIGIN = 0x10000000, LENGTH = 393216
    SHARED_SRAM  (rwx) : ORIGIN = 0x10060000, LENGTH = 1048576
}

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        KEEP(*(.patch))
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > MCU_MRAM

    /* User stack section initialized by startup code. */
    .stack (NOLOAD):
    {
        . = ALIGN(8);
        *(.stack)
        *(.stack*)
        . = ALIGN(8);
    } > MCU_TCM

    .heap : {
        __heap_start__ = .;
        end = __heap_start__;
        _end = end;
        __end = end;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
    } > MCU_TCM

    .data :
    {
        . = ALIGN(4);
        _sdata = .;     

        *k_*(.data)
        *k_*(.data*)
        *.a(.data) /*DONT checkpoint apollo libs*/
        *.a(.data*) 
        *am_*(.data) /*DONT checkpoint apollo libs*/
        *am_*(.data*)
        *(.checkpoint_exclude_data)

        . = ALIGN(4);
        _scheckpointedData = .;         
        *(.data)
        *(.data*)
        *(.checkpoint_include_data)
        . = ALIGN(4);
        _echeckpointedData = .;   
        _edata = .;
    } > MCU_TCM AT>MCU_MRAM

    _sizecheckpointedData = _echeckpointedData - _scheckpointedData;

    /* used by startup to initialize data */
    _init_data = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *k_*(.bss)
        *k_*(.bss*) 
        *.a(.bss) /*DONT checkpoint apollo libs*/
        *.a(.bss*)
        *am_*(.bss) /*DONT checkpoint apollo libs*/
        *am_*(.bss*)
        *(.checkpoint_exclude_bss)

        . = ALIGN(4);        
        _scheckpointedBss = .; 
        *(.bss)
        *(.bss*)
        *(.checkpoint_include_bss)
        *(COMMON)
        . = ALIGN(4);
        _echeckpointedBss = .;
        _ebss = .;
    } > MCU_TCM

    _sizecheckpointedBss = _echeckpointedBss - _scheckpointedBss;

    .shared :
    {
        . = ALIGN(4);
        KEEP(*(.resource_table))
        KEEP(*(.shared))
        . = ALIGN(4);
    } > SHARED_SRAM AT>MCU_MRAM
    .ARM.attributes 0 : { *(.ARM.attributes) }


    .appStorage :
    {
        . = ALIGN(16); /*MRAM is easier to used if aligned to 16 bytes blocks*/
        __appStorageStart = .;
        . = . + LENGTH(MRAM_APPSTORAGE); /*It will always say 100% usage*/
        . = ALIGN(16); /*This will complain if the start/size are not aligned by overflowing*/
        __appStorageEnd = .;       
    } > MRAM_APPSTORAGE

    .saveState :
    {
        . = ALIGN(16); /*MRAM is easier to used if aligned to 16 bytes blocks*/
        __saveStateStart = .;
        *(.SaveState) /*A pretty big struct that contains known things at compile time, suck as checkpoint headers, heap, stack and registers*/
        . = ALIGN(16); /*MRAM is easier to used if aligned to 16 bytes blocks*/
        _mramCheckpointedDataStart = .;
        . = . + (_sizecheckpointedData); /*Reserve enough space for everything from the checkpointed data section to be copied here*/
        . = ALIGN(16); /*This will complain if the start/size are not aligned by overflowing*/
        _mramCheckpointedDataEnd = .;
        _mramCheckpointedBssStart = .;
        . = . + (_sizecheckpointedBss); /*Reserve enough space for everything from the checkpointed bss section to be copied here*/
        . = ALIGN(16); /*This will complain if the start/size are not aligned by overflowing*/
        _mramCheckpointedBssEnd = .;
        
         . = ALIGN(16); /*MRAM is easier to used if aligned to 16 bytes blocks*/
        _mramCheckpointedDataStart2 = .;
        . = . + (_sizecheckpointedData); /*Reserve enough space for everything from the checkpointed data section to be copied here*/
        . = ALIGN(16); /*This will complain if the start/size are not aligned by overflowing*/
        _mramCheckpointedDataEnd2 = .;
        _mramCheckpointedBssStart2 = .;
        . = . + (_sizecheckpointedBss); /*Reserve enough space for everything from the checkpointed bss section to be copied here*/
        . = ALIGN(16); /*This will complain if the start/size are not aligned by overflowing*/
        _mramCheckpointedBssEnd2 = .;
        __saveStateEnd = .;       
    } > MRAM_SAVESTATE
}