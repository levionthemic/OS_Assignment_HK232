/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee
 * a personal to use and modify the Licensed Source Code for
 * the sole purpose of studying during attending the course CO2018.
 */
// #ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc, struct memphy_struct *mp)
{
    /* TODO update all page table directory info
     *      in flush or wipe TLB (if needed)
     */

    return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct *mp)
{
    /* TODO flush tlb cached*/

    return 0;
}

/**
 * tlballoc - CPU TLB-based allocate a region memory
 * @param proc:  Process executing the instruction
 * @param size: allocated size
 * @param reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
    //printf("Start of tlballoc process %d\n", proc->pid);
    
    int addr, val;

    /* By default using vmaid = 0 */
    val = __alloc(proc, 0, reg_index, size, &addr);

    /* TODO update TLB CACHED frame num of the new allocated page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/

    int pgn = PAGING_PGN(addr);
    int fpn = PAGING_FPN(proc->mm->pgd[pgn]);

    add_row_tlb(proc->pid, pgn, fpn);
   
    //printf("End of tlballoc process %d\n", proc->pid);
    
    return val;
}

/**
 * tlbfree_data - CPU TLB-based free a region memory
 * @param proc: Process executing the instruction
 * @param size: allocated size
 * @param reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
    //printf("Start of tlbfree_data process %d\n", proc->pid);
    
    int val = __free(proc, 0, reg_index);
    if (val == -1)
        return -1;

    /* TODO update TLB CACHED frame num of freed page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/

    // Xoa dong co pgn trong tlb
    addr_t addr = proc->regs[reg_index];
    int pgn = PAGING_PGN(addr);
    int pid = proc->pid;
    remove_row(pid, pgn);
    //printf("End of tlbfree_data process %d\n", proc->pid);
    return 0;
}

/**
 * tlbread - CPU TLB-based read a region memory
 * @param proc: Process executing the instruction
 * @param source: index of source register
 * @param offset: source address = [source] + [offset]
 * @param destination: destination storage
 */
int tlbread(struct pcb_t *proc, uint32_t source, uint32_t offset, uint32_t destination)
{
    //printf("Start of tlbread process %d\n", proc->pid);
    BYTE data;
    int frmnum = -1;

    /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/
    /* frmnum is return value of tlb_cache_read/write value*/

    addr_t addr = proc->regs[source] + offset;
    int pid = proc->pid;
    int pgnum = PAGING_PGN(addr);
    tlb_cache_read(&proc, pid, pgnum, &frmnum);
    if (frmnum >= 0)
    {
        int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + offset;
        TLBMEMPHY_read(proc->tlb, phyaddr, &data);
        destination = (uint32_t)data;
        return 0;
    }
    
#ifdef IODUMP
    if (frmnum >= 0)
        printf("TLB hit at read region=%d offset=%d\n", source, offset);
    else
        printf("TLB miss at read region=%d offset=%d\n", source, offset);
#ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1); // print max TBL
#endif
    MEMPHY_dump(proc->mram);
#endif

    int val = __read(proc, 0, source, offset, &data);

    destination = (uint32_t)data;

    /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/

    struct mm_struct *mm = proc->mm;
    int fpn = PAGING_FPN(mm->pgd[pgnum]);
        // if (pg_getpage(mm, pgnum, &fpn, proc) != 0)
        //     return -1; /* invalid page access */
    tlb_cache_write(&proc, pid, pgnum, fpn);
    return val;
}

/**
 * tlbwrite - CPU TLB-based write a region memory
 * @param proc: Process executing the instruction
 * @param data: data to be wrttien into memory
 * @param destination: index of destination register
 * @param offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t *proc, BYTE data, uint32_t destination, uint32_t offset)
{
    //printf("Running tlbwrite process %d\n", proc->pid);
    int val;
    int frmnum = -1;

    /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
    /* by using tlb_cache_read()/tlb_cache_write()
    frmnum is return value of tlb_cache_read/write value*/
    
    addr_t addr = proc->regs[destination] + offset;
    
    int pid = proc->pid;
    int pgnum = PAGING_PGN(addr);
    
    tlb_cache_read(&proc, pid, pgnum, &frmnum);
    
    if (frmnum >= 0)
    {   
        int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + offset;
        TLBMEMPHY_write(proc->tlb, phyaddr, data);
    }
    else
    {
        struct mm_struct *mm = proc->mm;
        int fpn = PAGING_FPN(mm->pgd[pgnum]);
        // int fpn;
        // if (pg_getpage(mm, pgnum, &fpn, proc) != 0)
        //     return -1; /* invalid page access */
        int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + offset;
        TLBMEMPHY_write(proc->tlb, phyaddr, data);
        
        tlb_cache_write(&proc, pid, pgnum, fpn);
        
    }
#ifdef IODUMP
    if (frmnum >= 0)
        printf("TLB hit at write region=%d offset=%d value=%d\n",
               destination, offset, data);
    else
        printf("TLB miss at write region=%d offset=%d value=%d\n",
               destination, offset, data);
#ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1); // print max TBL
#endif
    MEMPHY_dump(proc->mram);
#endif
    
    val = __write(proc, 0, destination, offset, data);
    
    /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/

    return val;
}

// #endif
