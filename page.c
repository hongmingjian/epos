/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#include "kernel.h"
#include "dosfs.h"

extern VOLINFO g_volinfo;

static char    *g_swapfile_name="epos.swp";

static struct swap_page {
  uint32_t state; //0 - free,
                  //1 - shadow, means that this page lives both memory
                  //    and swap file
                  //2 - used, means that this page only lives swap file
  uint32_t vaddr;
} g_swap_pages[4096];/*XXX - The size of swap file is 4096 pages(16MiB)*/

static uint32_t g_max_victim = USER_MIN_ADDR+0x1400000;
static uint32_t g_min_victim = USER_MIN_ADDR;
static uint32_t g_cur_victim = USER_MIN_ADDR+0x1400000;

static
uint32_t get_swap_page(uint32_t state)
{
  int i;
  for(i = 0; i < __countof(g_swap_pages); i++)
    if(g_swap_pages[i].state == state)
      return i;
  return -1;
}

static
uint32_t find_swap_page(uint32_t vaddr)
{
  int i;

  vaddr = PAGE_TRUNCATE(vaddr);

  for(i = 0; i < __countof(g_swap_pages); i++)
    if((g_swap_pages[i].state > 0) &&
       (g_swap_pages[i].vaddr == vaddr))
      return i;

  return -1;
}

static
int page_swap_out(uint32_t i)
{
  uint8_t scratch[SECTOR_SIZE];
  FILEINFO fi;
  uint32_t write;

  g_swap_pages[i].vaddr = PAGE_TRUNCATE(g_swap_pages[i].vaddr);

  if(DFS_OK != DFS_OpenFile(&g_volinfo, g_swapfile_name,
                            DFS_WRITE, scratch, &fi)) {
    return -1;
  }

  DFS_Seek(&fi, i*PAGE_SIZE, scratch);
  if(fi.pointer != i*PAGE_SIZE) {
    return -2;
  }

  DFS_WriteFile(&fi, scratch, (void *)(g_swap_pages[i].vaddr),
                &write, PAGE_SIZE);
  return write-PAGE_SIZE;
}

static
int page_swap_in(uint32_t i)
{
  uint8_t scratch[SECTOR_SIZE];
  FILEINFO fi;
  uint32_t read;

  g_swap_pages[i].vaddr = PAGE_TRUNCATE(g_swap_pages[i].vaddr);

  if(DFS_OK != DFS_OpenFile(&g_volinfo, g_swapfile_name,
                            DFS_READ, scratch, &fi)) {
    return -1;
  }

  DFS_Seek(&fi, i*PAGE_SIZE, scratch);
  if(fi.pointer != i*PAGE_SIZE) {
    return -2;
  }

  DFS_ReadFile(&fi, scratch, (void *)(g_swap_pages[i].vaddr),
               &read, PAGE_SIZE);
  return read-PAGE_SIZE;
}

int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code)
{
  uint32_t flags;

#if VERBOSE
  printk("PF:0x%08x(0x%01x)", vaddr, code);
#endif

  if((code&PTE_V) == 0) {
    uint32_t paddr;
    int i, found = 0;

    if(code&PTE_U) {
      if ((vaddr <  USER_MIN_ADDR) ||
          (vaddr >= USER_MAX_ADDR)) {
        printk("PF:Invalid memory access: 0x%08x(0x%01x)\r\n", vaddr, code);
        return -1;
      }
    } else {
      if ((vaddr >= USER_MIN_ADDR) &&
          (vaddr <  USER_MAX_ADDR))
        code |= PTE_U;

      if((vaddr >= (uint32_t)vtopte(USER_MIN_ADDR)) &&
         (vaddr <  (uint32_t)vtopte(USER_MAX_ADDR)))
        code |= PTE_U;
    }

    /* Search for a free frame */
    save_flags_cli(flags);
    for(i = 0; i < g_frame_count; i++) {
      if(g_frame_freemap[i] == 0) {
        found = 1;
        g_frame_freemap[i] = 1;
        break;
      }
    }
    restore_flags(flags);

    if(found) {

      /* Got one :), clear its data before returning */
      paddr = g_ram_zone[0/*XXX*/]+(i<<PAGE_SHIFT);
      *vtopte(vaddr)=paddr|PTE_V|PTE_W|(code&PTE_U);
      memset(PAGE_TRUNCATE(vaddr), 0, PAGE_SIZE);
      invlpg(vaddr);

#if VERBOSE
      printk("->0x%08x\r\n", *vtopte(vaddr));
#endif

      return 0;

    } else {
      uint32_t *pte, victim;

      /*
       * No free frame :(, search for a victim page using second-chance
       * algorithm
       */
      save_flags_cli(flags);
      do {
        if(g_cur_victim < g_min_victim)
          g_cur_victim = g_max_victim;

        g_cur_victim -= PAGE_SIZE;

        pte = vtopte(g_cur_victim);
        if((*pte) & PTE_V) {
          if((*pte) & PTE_A)
            *pte = (*pte) & (~PTE_A);
          else {
            break;
          }
        }
      } while(1);
      victim = g_cur_victim;
      restore_flags(flags);

      pte = vtopte(victim);

#if VERBOSE
      printk("[0x%08x(0x%08x)", victim, *pte);
#endif

      /* Does the victim page have a shadow in the swap file? */
      save_flags_cli(flags);
      i = find_swap_page(victim);
      if(i != (uint32_t)-1) {

        /* Yes, mark it as used */
        g_swap_pages[i].state = 2/*used*/;
        restore_flags(flags);

        /* Is the victim page a dirty one? */
        if((*pte) & PTE_M) {

          /* Yes, page it out */
          page_swap_out(i);
#if VERBOSE
          printk("*>0x%08x]", i*PAGE_SIZE);
#endif
        }
#if VERBOSE
        else
          printk("]");
#endif
      } else {

        /*
         * No. Try to find an available page in the swap file to accommodate
         * the victim page.
         *
         * Here, we search for a shadow page first because the size of swap
         * file is very limited. If no shadow page is found, we try again to
         * find a free page.
         */
        i = get_swap_page(1/*shadow*/);
        if(i == (uint32_t)-1)
          i = get_swap_page(0/*free*/);

        if(i != (uint32_t)-1) {

          /* Got it. Mark it as used */
          g_swap_pages[i].vaddr = victim;
          g_swap_pages[i].state = 2/*used*/;
          restore_flags(flags);

          page_swap_out(i);
#if VERBOSE
          printk("+>0x%08x]", i*PAGE_SIZE);
#endif
        } else {
          restore_flags(flags);
          printk("No free swap space!\r\n");
          goto failed;
        }
      }

      /*
       * The physical address of the victim page is extracted
       * and we map the fault page to it
       */
      paddr = PAGE_TRUNCATE(*pte);

      save_flags_cli(flags);

      *pte = 0;
      invlpg(victim);

      *vtopte(vaddr)=paddr|PTE_V|PTE_W|(code&PTE_U);
      invlpg(vaddr);

      /* Does the fault page have a shadow in the swap file? */
      i = find_swap_page(vaddr);

#if VERBOSE
      printk("->0x%08x", *vtopte(vaddr));
#endif

      if(i != (uint32_t)-1) {

        /* Yes, mark it as a shadow and page it in */
        g_swap_pages[i].state = 1/*shadow*/;
        restore_flags(flags);
        page_swap_in(i);

#if VERBOSE
        printk("<-0x%08x\r\n", i*PAGE_SIZE);
#endif
      } else {

        /*
         * No, we must clear the old data of the page before handing over
         * it to the fault page
         */
        restore_flags(flags);
        memset(PAGE_TRUNCATE(vaddr), 0, PAGE_SIZE);
#if VERBOSE
        printk("\r\n");
#endif
      }
    }

    return 0;
  }

failed:
  printk("->  ????????\r\n");
  return -1;
}

