/* vm/swap.h */
#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>

void vm_swap_init (void);
void vm_swap_in (size_t swap_index, void *kpage);
size_t vm_swap_out (void *kpage);
void vm_swap_free (size_t swap_index);

#endif /* vm/swap.h */