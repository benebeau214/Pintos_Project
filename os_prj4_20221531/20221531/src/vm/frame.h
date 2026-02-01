#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/synch.h"

extern struct list_elem *clock_ptr;

struct frame {
    void *kpage;
    struct vm_entry *vme;
    struct thread *thread;
    struct list_elem elem;
};

void vm_frame_init (void);
struct page *alloc_page (enum palloc_flags flags);
void free_page (void *kpage);
void __free_page (struct frame *f);
void add_page_to_frame (void *kpage, struct vm_entry *vme);

#endif /* vm/frame.h */