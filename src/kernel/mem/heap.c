#include "heap.h"
#include "pfa.h"
#include "paging.h"
#include "../utils/constants.h"
#include "../utils/alloc.h"
#include <mem.h>
#include <math.h>
#include <stddef.h>

#define MIN_SIZE 16

static uint64_t heap_start, heap_end, heap_ceil;
static heap_segment_header_t* head;
static heap_segment_header_t* tail;

static uint64_t expand_kernel_heap(uint64_t size)
{
    uint64_t pages_paddr, pages_count, mapped_size;
    heap_segment_header_t* new = (heap_segment_header_t*) heap_end;
    
    size = alignu(size + sizeof(heap_segment_header_t), SIZE_4KB);
    if (heap_end + size > heap_ceil)
        size = heap_ceil - heap_end;
    
    pages_count = size / SIZE_4KB;
    pages_paddr = request_pages(pages_count);
    if (pages_paddr == 0)
        return 0;
    
    mapped_size = paging_map_memory(pages_paddr, heap_end, size, PAGE_ACCESS_RW, PL0);
    if (mapped_size < size)
    {
        free_pages(pages_paddr, pages_count);
        return 0;
    }
    
    heap_end += mapped_size;

    new->free = 1;
    new->size = size - sizeof(heap_segment_header_t*);
    new->data = (uint64_t)(new + 1);
    new->prev = tail;
    new->next = tail->next;
    tail->next = new;
    tail = new;

    return mapped_size;
}

static heap_segment_header_t* next_free_kernel_heap_segment(uint64_t size)
{
    heap_segment_header_t* current = head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    if (expand_kernel_heap(size) < size)
        return NULL;
    return next_free_kernel_heap_segment(size);
}

static void combine_kernel_heap_forward(heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == tail)
        tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void combine_kernel_heap_backward(heap_segment_header_t* seg)
{
    if (seg->prev != NULL)
        combine_kernel_heap_forward(seg->prev);
}

void free_kernel_heap_memory(uint64_t addr)
{
    heap_segment_header_t* seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    combine_kernel_heap_forward(seg);
    combine_kernel_heap_backward(seg);
}

uint64_t allocate_kernel_heap_memory(uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = alignu(size, MIN_SIZE) + sizeof(heap_segment_header_t);

    seg = next_free_kernel_heap_segment(size);
    if (seg == NULL)
        return 0;
    
    new = (heap_segment_header_t*)(seg->data + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    new->data = (uint64_t)(new + 1);
    if (seg == tail)
        tail = new;
    
    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

uint64_t init_kernel_heap(uint64_t start_addr, uint64_t end_addr, uint64_t initial_pages)
{
    uint64_t initial_size, pages_paddr;
    
    heap_ceil = alignd(end_addr, SIZE_4KB);
    initial_size = initial_pages * SIZE_4KB;
    if (start_addr + initial_size > heap_ceil)
    {
        initial_size = heap_ceil - start_addr;
        initial_pages = ceil((double) initial_size / SIZE_4KB);
        end_addr = start_addr + initial_size;
    }

    pages_paddr = request_pages(initial_pages);
    if (pages_paddr == 0)
        return 0;

    if (paging_map_memory(pages_paddr, start_addr, initial_size, PAGE_ACCESS_RW, PL0) < initial_size)
    {
        paging_unmap_memory(start_addr, initial_size);
        free_pages(pages_paddr, initial_pages);
        return 0;
    }
    
    heap_start = start_addr;
    heap_end = start_addr + initial_size;
    
    head = (heap_segment_header_t*) heap_start;
    head->next = NULL;
    head->prev = NULL;
    head->data = (uint64_t)(head + 1);
    head->size = initial_size - sizeof(heap_segment_header_t);
    head->free = 1;

    tail = head;

    return initial_size;
}

static void record_segment(segment_list_t* sl, uint64_t paddr, uint64_t pages)
{
    segment_list_entry_t* entry = kmalloc(sizeof(segment_list_entry_t));
    
    entry->paddr = paddr;
    entry->pages = pages;
    entry->next = NULL;

    if (sl->head == NULL)
        sl->head = entry;
    else
        sl->tail->next = entry;
    sl->tail = entry;
}

static uint64_t expand_process_heap(uint64_t size, process_t* ps)
{
    uint64_t pages_paddr, pages_count, mapped_size;
    heap_segment_header_t* new = (heap_segment_header_t*) ps->heap.end_vaddr;
    
    size = alignu(size + sizeof(heap_segment_header_t), SIZE_4KB);
    if (ps->heap.end_vaddr + size > ps->heap.ceil_vaddr)
        size = ps->heap.ceil_vaddr - ps->heap.end_vaddr;
    
    pages_count = size / SIZE_4KB;
    pages_paddr = request_pages(pages_count);
    if (pages_paddr == 0)
        return 0;
    
    mapped_size = pml4_map_memory(ps->pml4, pages_paddr, ps->heap.end_vaddr, size, PAGE_ACCESS_RW, PL3);
    if (mapped_size < size)
    {
        free_pages(pages_paddr, pages_count);
        return 0;
    }

    record_segment(&ps->heap.segments, pages_paddr, pages_count);
    
    ps->heap.end_vaddr += mapped_size;

    new->free = 1;
    new->size = size - sizeof(heap_segment_header_t*);
    new->data = (uint64_t)(new + 1);
    new->prev = ps->heap.tail;
    new->next = ps->heap.tail->next;
    ps->heap.tail->next = new;
    ps->heap.tail = new;

    return mapped_size;
}

static heap_segment_header_t* next_free_process_heap_segment(uint64_t size, process_t* ps)
{
    heap_segment_header_t* current = ps->heap.head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    if (expand_process_heap(size, ps) < size)
        return NULL;
    return next_free_process_heap_segment(size, ps);
}

static void combine_process_heap_forward(heap_segment_header_t* seg, process_t* ps)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == ps->heap.tail)
        ps->heap.tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void combine_process_heap_backward(heap_segment_header_t* seg, process_t* ps)
{
    if (seg->prev != NULL)
        combine_process_heap_forward(seg->prev, ps);
}

void free_process_heap_memory(uint64_t addr, process_t* ps)
{
    heap_segment_header_t* seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    combine_process_heap_forward(seg, ps);
    combine_process_heap_backward(seg, ps);
}

uint64_t allocate_process_heap_memory(uint64_t size, process_t* ps)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = alignu(size, MIN_SIZE) + sizeof(heap_segment_header_t);

    seg = next_free_process_heap_segment(size, ps);
    if (seg == NULL)
        return 0;
    
    new = (heap_segment_header_t*)(seg->data + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    new->data = (uint64_t)(new + 1);
    if (seg == ps->heap.tail)
        ps->heap.tail = new;
    
    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

uint64_t init_process_heap(uint64_t start_addr, uint64_t end_addr, uint64_t initial_pages, process_t* ps)
{
    uint64_t initial_size, pages_paddr;

    ps->heap.ceil_vaddr = alignd(end_addr, SIZE_4KB);
    initial_size = initial_pages * SIZE_4KB;
    if (start_addr + initial_size > ps->heap.ceil_vaddr)
    {
        initial_size = ps->heap.ceil_vaddr - start_addr;
        initial_pages = ceil((double) initial_size / SIZE_4KB);
        end_addr = start_addr + initial_size;
    }
    
    pages_paddr = request_pages(initial_pages);
    if (pages_paddr == 0)
        return 0;
    
    if 
    (
        pml4_map_memory(ps->pml4, pages_paddr, start_addr, initial_size, PAGE_ACCESS_RW, PL3) < initial_size ||
        paging_map_memory(pages_paddr, start_addr, initial_size, PAGE_ACCESS_RW, PL0) < initial_size
    )
    {
        pml4_unmap_memory(ps->pml4, start_addr, initial_size);
        paging_unmap_memory(start_addr, initial_size);
        free_pages(pages_paddr, initial_pages);
        return 0;
    }

    record_segment(&ps->heap.segments, pages_paddr, initial_pages);

    ps->heap.start_vaddr = start_addr;
    ps->heap.end_vaddr = end_addr;
    
    ps->heap.head = (heap_segment_header_t*) ps->heap.start_vaddr;
    ps->heap.head->next = NULL;
    ps->heap.head->prev = NULL;
    ps->heap.head->data = (uint64_t)(ps->heap.head + 1);
    ps->heap.head->size = initial_size - sizeof(heap_segment_header_t);
    ps->heap.head->free = 1;

    ps->heap.tail = ps->heap.head;

    paging_unmap_memory(start_addr, initial_size);

    return initial_size;
}