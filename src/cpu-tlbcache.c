/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee
 * a personal to use and modify the Licensed Source Code for
 * the sole purpose of studying during attending the course CO2018.
 */
// #ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access
 * and runs at high speed
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#define init_tlbcache(mp, sz, ...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
#define TLB_SIZE 0x10000

typedef struct
{
    int pid;
    int pgn;
    int fpn;
} cache_line;
static cache_line tlb_cache[TLB_SIZE];
static int tlb_size = 0;

static pthread_mutex_t tlb_lock;

int print_tlb()
{  
    pthread_mutex_lock(&tlb_lock);
    for (int i = 0; i < tlb_size; i++)
        printf("pid = %d, pgn = %08ld\n" ,
            tlb_cache[i].pid, tlb_cache[i].pgn * sizeof (uint32_t));
    pthread_mutex_unlock(&tlb_lock);
    return 0;
}
int remove_row(int pid, int pgn)
{
    pthread_mutex_lock(&tlb_lock);
    for (int i = 0; i < tlb_size; i++)
    {
        if (tlb_cache[i].pid == pid && tlb_cache[i].pgn == pgn)
        {
            tlb_cache[i] = tlb_cache[tlb_size - 1];
            tlb_size--;
            break;
        }
    }
    pthread_mutex_unlock(&tlb_lock);
    return 0;
}
int add_row_tlb(int pid, int pgn, int fpn)
{
    pthread_mutex_lock(&tlb_lock);
    if (tlb_size == TLB_SIZE)
    {
        pthread_mutex_unlock(&tlb_lock);
        return -1;
    }
    tlb_cache[tlb_size].pid = pid;
    tlb_cache[tlb_size].pgn = pgn;
    tlb_cache[tlb_size].fpn = fpn;
    tlb_size++;
    pthread_mutex_unlock(&tlb_lock);
    return 0;
}

/**
 *  tlb_cache_read read TLB cache device
 *  @param mp: memphy struct
 *  @param pid: process id
 *  @param pgnum: page number
 *  @param value: obtained value
 */
int tlb_cache_read(struct pcb_t ** proc, int pid, int pgnum, int *value)
{
    /* TODO: the identify info is mapped to
     *      cache line by employing:
     *      direct mapped, associated mapping etc.
     */
    pthread_mutex_lock(&tlb_lock);
    //printf("Start of tlb_cache_read process %d\n", pid);
    //printf("pid = %d, pgn = %d\n", pid, pgnum);
    for (int i = 0; i < tlb_size; i++)
    {
        //printf("pid = %d, pgn = %d\n", tlb_cache[i].pid, tlb_cache[i].pgn);
        if (tlb_cache[i].pid == pid && tlb_cache[i].pgn == pgnum)
        {
            *value = tlb_cache[i].fpn;
            break;
        }
    }
    //printf("End of tlb_cache_read process %d\n", pid);
    pthread_mutex_unlock(&tlb_lock);
    return 0;
}

/**
 *  tlb_cache_write write TLB cache device
 *  @param mp: memphy struct
 *  @param pid: process id
 *  @param pgnum: page number
 *  @param value: obtained value
 */
int tlb_cache_write(struct pcb_t ** proc, int pid, int pgnum, int value)
{
    /* TODO: the identify info is mapped to
     *      cache line by employing:
     *      direct mapped, associated mapping etc.
     */
    pthread_mutex_lock(&tlb_lock);
    for (int i = 0; i < tlb_size; i++)
    {
        //printf("pid = %d, pgn = %d\n", tlb_cache[i].pid, tlb_cache[i].pgn);
        if (tlb_cache[i].pid == pid && tlb_cache[i].pgn == pgnum)
        {
            tlb_cache[i].fpn = value;
            break;
        }
    }
    pthread_mutex_unlock(&tlb_lock);
    //printf("End of tlb_cache_read process %d\n", pid);
    return 0;
}

/**
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @param mp: memphy struct
 *  @param addr: address
 *  @param value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value)
{
    //printf("Start of TLBMEMPHY_read\n");
    if (mp == NULL)
        return -1;
    
    /* TLB cached is random access by native */
    //printf("state 1\n");
    BYTE data = mp->storage[addr];
    *value = data;
    //printf("state 2\n");
    //printf("End of TLBMEMPHY_read\n");
    return 0;
}

/**
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @param mp: memphy struct
 *  @param addr: address
 *  @param data: written data
 */
int TLBMEMPHY_write(struct memphy_struct *mp, int addr, BYTE data)
{
    if (mp == NULL)
        return -1;

    /* TLB cached is random access by native */
    mp->storage[addr] = data;

    return 0;
}

/**
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @param mp: memphy struct
 */

int TLBMEMPHY_dump(struct memphy_struct *mp)
{
    /*TODO dump memphy contnt mp->storage
     *     for tracing the memory content
     */
    char result[100];
    strcpy(result, "Memory content-[pos, content]: ");
    char temp[100];
    if (mp && mp->storage)
    {
        for (int i = 0; i < mp->maxsz; i++)
        {
            if (mp->storage[i] != (char)0)
            {
                sprintf(temp, "[%d, %d]", i, mp->storage[i]);
                strcat(result, temp);
            }
        }
        strcat(result, "\n\0");
    }
    printf("%s", result);
    return 0;
}

/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
    mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
    mp->maxsz = max_size;

    mp->rdmflg = 1;

    return 0;
}

// #endif
