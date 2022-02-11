#include "syscall.h"
#include "scheduler.h"
#include "ipc/spp.h"
#include "../mem/heap.h"
#include "../mem/paging.h"
#include "../mem/pfa.h"
#include "../utils/constants.h"
#include "../utils/macros.h"
#include "../utils/helpers/alloc.h"
#include "../sys/pci.h"
#include "../abi/vm-flags.h"
#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <math.h>

#define SYSCALL(name) sys_##name
#define DEFINE_SYSCALL(name) static int SYSCALL(name)(uint64_t* stack)
#define POP_STACK(T) *((T*) stack++)
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(uint64_t* stack_ptr);

extern void switch_to_kernel(cpu_state_t* state);

/*
static void* pmalloc(uint64_t size)
{
    return (void*) allocate_heap_memory(get_current_scheduled_process()->heap, size);
}

static void pfree(void* addr)
{
    free_heap_memory(get_current_scheduled_process()->heap, (uint64_t) addr);
}
*/

DEFINE_SYSCALL(exit)
{
    UNUSED(stack);
    switch_to_kernel(&(get_current_scheduled_process()->user_mode));
    terminate_process(get_current_scheduled_process());
    run_scheduler();
    return -1;
}

DEFINE_SYSCALL(execve)
{
    const char* path;
    process_t* current;
    process_t* new;

    path = POP_STACK(const char*);
    current = get_current_scheduled_process();
    new = create_replacement_process(current, NULL);

    UNUSED(path);
    UNUSED(current);
    UNUSED(new);

    return 0;
}

/*
static int sys_spp_expose(uint64_t* stack)
{
    uint64_t fn_addr = POP_STACK(uint64_t, stack);
    const char* fn_name = POP_STACK(const char*, stack);
    return expose_function(get_current_scheduled_process(), fn_addr, fn_name);
}

static int sys_spp_request(uint64_t* stack)
{
    const char* fn_name = POP_STACK(const char*, stack);
    uint64_t* out_addr = POP_STACK(uint64_t*, stack);
    *out_addr = request_func(get_current_scheduled_process(), fn_name);
    return -(out_addr == 0);
}

static int sys_pci_find(uint64_t* stack)
{
    pci_devices_list_entry_t* entry;
    pci_export_list_entry_t* new_entry;
    pci_header_common_t* header;
    uint64_t header_size, devices_found = 0;
    uint8_t class = POP_STACK(uint8_t, stack);
    uint8_t subclass = POP_STACK(uint8_t, stack);
    uint8_t program_interface = POP_STACK(uint8_t, stack);
    pci_export_list_t* new_list = POP_STACK(pci_export_list_t*, stack);
    pci_devices_list_t* list = find_pci_devices(class, subclass, program_interface);

    if (list == NULL)
        return 0;
    
    new_list->head = NULL;
    new_list->tail = NULL;

    entry = list->head;
    while (entry != NULL)
    {   
        // The problem with mapping only one page is if the header overlaps two pages (very unlikely scenario but still possible)
        header = (pci_header_common_t*) (kernel_map_temporary_page(entry->header_paddr, PAGE_ACCESS_RO, PL0) + GET_ADDR_OFFSET(entry->header_paddr));
        switch (GET_HEADER_TYPE(header->header_type))
        {
        case PCI_HEADER_0x0:
            header_size = sizeof(pci_header_0x0_t);
            break;
        case PCI_HEADER_0x1:
            header_size = sizeof(pci_header_0x1_t);
            break;
        case PCI_HEADER_0x2:
            header_size = sizeof(pci_header_0x2_t);
            break;
        }
        
        new_entry = pmalloc(header_size + sizeof(pci_devices_list_entry_t*));
        if (new_entry == NULL)
        {
            kernel_unmap_temporary_page((uint64_t) header);
            delete_devices_list(list);
            free(list);
            return 0;
        }
        new_entry->next = NULL;
        memcpy(&new_entry->header, header, header_size);
        if (new_list->tail == NULL)
            new_list->head = new_entry;
        else
            new_list->tail->next = new_entry;
        new_list->tail = new_entry;

        kernel_unmap_temporary_page((uint64_t) header);
        entry = entry->next;
        ++devices_found;
    }

    delete_devices_list(list);
    return devices_found;
}
*/

DEFINE_SYSCALL(vm_map)
{
    uint64_t hint = POP_STACK(uint64_t);
    uint64_t size = POP_STACK(uint64_t);
    PAGE_ACCESS_TYPE prot = POP_STACK(PAGE_ACCESS_TYPE);
    int flags = POP_STACK(int);
    int fd = POP_STACK(int);
    long offset = POP_STACK(long);
    uint64_t* window = POP_STACK(uint64_t*);
    process_t* ps = get_current_scheduled_process();
    uint64_t vaddr, paddr;

    *window = (uint64_t) -1;

    if 
    (
        hint & 0x0000000000000FFF ||
        size & 0x0000000000000FFF ||
        offset & 0x0000000000000FFF
    )
        return EINVAL;

    if ((flags & MAP_ANONYMOUS) && fd == -1 && offset == 0)
        paddr = request_pages(ceil((double) size / SIZE_4KB));
    else
    {
        /* TODO: Implement mapping fd */
        paddr = 0;
    }

    if (flags & (MAP_FIXED | MAP_FIXED_NOREPLACE))
    {
        if (flags & MAP_FIXED)
            pml4_unmap_memory(ps->pml4, hint, size);
        if (pml4_map_memory(ps->pml4, paddr, hint, size, prot, PL3) < size)
            return EEXIST;
    }
    else
    {
        if (hint < ps->mapping_start_vaddr)
            hint = ps->mapping_start_vaddr;
        if 
        (
            (flags & MAP_32BIT) &&
            (
                hint > alignd(VADDR_GET(0, 0, 2, 0) - size, SIZE_4KB) ||
                pml4_get_next_vaddr(ps->pml4, hint, size, &vaddr) < size
            )
        )
            return ENOMEM;
        else if (pml4_get_next_vaddr(ps->pml4, hint, size, &vaddr) < size)
            return ENOMEM;
        if (pml4_map_memory(ps->pml4, paddr, vaddr, size, prot, PL3) < size)
            return EEXIST;
    }

    if ((flags & MAP_ANONYMOUS) || !(flags & MAP_UNINITIALIZED))
        memset((void*) vaddr, 0, size);

    *window = vaddr;

    return 0;
}

DEFINE_SYSCALL(vm_unmap)
{
    uint64_t vaddr = POP_STACK(uint64_t);
    uint64_t size = POP_STACK(uint64_t);
    process_t* ps = get_current_scheduled_process();

    if (pml4_unmap_memory(ps->pml4, vaddr, size) < size)
        return -1;

    return 0;
}

static syscall_handler_t handlers[] = {
    SYSCALL(exit),
    SYSCALL(execve),
    SYSCALL(vm_map),
    SYSCALL(vm_unmap)
};

#define NUM_SYSCALLS (sizeof(handlers) / sizeof(uint64_t))

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num < NUM_SYSCALLS)
        return handlers[num](stack);
    return INVALID_SYSCALL;
}