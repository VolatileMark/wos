#ifndef __WOS_VM_FLAGS_H__
#define __WOS_VM_FLAGS_H__

#define EEXIST -1
#define EINVAL -2
#define ENOMEM -3

#define PROT_NONE 0
#define PROT_EXEC (1 << 1)
#define PROT_READ (1 << 2)
#define PROT_WRITE (1 << 0)

#define MAP_PRIVATE (1 << 0)
#define MAP_32BIT (1 << 1)
#define MAP_ANONYMOUS (1 << 2)
#define MAP_FIXED (1 << 6)
#define MAP_FIXED_NOREPLACE (1 << 7)
#define MAP_UNINITIALIZED (1 << 8)

#endif