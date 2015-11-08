#include "kernel.h"
#include "bitmap.h"

static struct pmzone {
 uint32_t base;
 uint32_t limit;
 struct bitmap *bitmap;
} pmzone[RAM_ZONE_LEN/2];

void init_frame()
{
    int i, z = 0;

    for(i = 0; i < RAM_ZONE_LEN; i += 2) {
        if(g_ram_zone[i+1] - g_ram_zone[i] == 0)
            break;

        pmzone[z].base = g_ram_zone[i];
        pmzone[z].limit = g_ram_zone[i+1]-g_ram_zone[i];
        uint32_t bit_cnt = pmzone[z].limit/PAGE_SIZE;
        uint32_t size = PAGE_ROUNDUP(bitmap_buf_size(bit_cnt));
        uint32_t paddr = pmzone[z].base;
        pmzone[z].base += size;
        pmzone[z].limit -= size;
        if(pmzone[z].limit == 0)
            continue;

        printk("RAM: 0x%08x - 0x%08x (%d frames)\r\n",
                pmzone[z].base, pmzone[z].base+pmzone[z].limit, pmzone[z].limit/PAGE_SIZE);

        uint32_t vaddr=page_alloc(size / PAGE_SIZE, 0);
        page_map(vaddr, paddr, size/PAGE_SIZE, PTE_V|PTE_W);
        pmzone[z].bitmap=bitmap_create_in_buf(bit_cnt, (void *)vaddr, 0);
        z++;
    }
}

uint32_t frame_alloc_in_addr(uint32_t pa, uint32_t npages)
{
    int z;
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pa >= pmzone[z].base &&
           pa <  pmzone[z].base + pmzone[z].limit) {
            uint32_t idx = (pa - pmzone[z].base) / PAGE_SIZE;
            if(bitmap_none(pmzone[z].bitmap, idx, npages)) {
                bitmap_set_multiple(pmzone[z].bitmap, idx, npages, 1);
                return pa;
            }
        }
    }
    return BITMAP_ERROR;
}

uint32_t frame_alloc(uint32_t npages)
{
    int z;
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pmzone[z].limit == 0)
            break;
        uint32_t idx = bitmap_scan(pmzone[z].bitmap, 0, npages, 0);
        if(idx != BITMAP_ERROR) {
            bitmap_set_multiple(pmzone[z].bitmap, idx, npages, 1);
            return pmzone[z].base + idx * PAGE_SIZE;
        }
    }

    return BITMAP_ERROR;
}

void frame_free(uint32_t paddr, uint32_t npages)
{
    uint32_t z;
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pmzone[z].limit == 0)
            break;
        if(paddr >= pmzone[z].base &&
           paddr <  pmzone[z].base+pmzone[z].limit) {
            uint32_t idx = (paddr - pmzone[z].base) / PAGE_SIZE;
            /* XXX - 确认之前不是空闲的
            if(bitmap_any(pmzone[z].bitmap, idx, npages))
                return;*/
            bitmap_set_multiple(pmzone[z].bitmap, idx, npages, 0);
            return;
        }
    }
}


