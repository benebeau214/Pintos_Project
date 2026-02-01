#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include <bitmap.h>
#include <stdio.h>

extern struct lock filesys_lock;

static struct list frame_table;
static struct lock frame_lock;
struct list_elem *clock_ptr;

void vm_frame_init (void) {
    list_init(&frame_table);
    lock_init(&frame_lock);
    clock_ptr = list_begin(&frame_table);
}


static void *try_to_free_pages (enum palloc_flags flags) {
    struct list_elem *e = clock_ptr;
    if (list_empty(&frame_table)) PANIC("Frame table is empty, but memory is full!");

    lock_acquire(&frame_lock);
    while (true) {
        if (e == list_end(&frame_table)) e = list_begin(&frame_table);
        struct frame *f = list_entry(e, struct frame, elem);

        if (f->vme == NULL) {
            e = list_next(e);
            continue;
        }

        struct thread *t = f->thread;
        
        /* 1. Second Chance (Accessed Bit) */
        if (pagedir_is_accessed(t->pagedir, f->vme->vaddr)) {
            pagedir_set_accessed(t->pagedir, f->vme->vaddr, false);
        } 
        else {
            /* 2. Eviction 및 Write-back 결정 */
            bool dirty = pagedir_is_dirty(t->pagedir, f->vme->vaddr);
            
            // 2-1. VM_FILE (mmap) 처리
            if (f->vme->type == VM_BIN && !dirty) {
                f->vme->is_loaded = false;
            } 
            else if (f->vme->type == VM_FILE) {             
             if (dirty) {
                 file_write_at(f->vme->file, f->kpage, 
                                f->vme->read_bytes, f->vme->offset);
             }

             f->vme->is_loaded = false; 
            }
            else {
                size_t swap_index = vm_swap_out(f->kpage);
                if (swap_index == BITMAP_ERROR) PANIC("Swap Disk is Full!");
                
                f->vme->swap_slot = swap_index;
                f->vme->type = VM_ANON; // 타입 변경 (Swap-backed)
                f->vme->is_loaded = false;
            }
                
            pagedir_clear_page(t->pagedir, f->vme->vaddr);
            palloc_free_page(f->kpage);
            
            if (clock_ptr == &f->elem) clock_ptr = list_next(e);
            list_remove(&f->elem);
            free(f);
            lock_release(&frame_lock);
            return palloc_get_page(flags); // 새 페이지 반환
        }
        e = list_next(e);
    }
}

struct page *alloc_page (enum palloc_flags flags) {
    void *kpage = palloc_get_page(flags);
    while (kpage == NULL) {
        kpage = try_to_free_pages(flags);
    }
    struct frame *f = malloc(sizeof(struct frame));
    if (f == NULL) { palloc_free_page(kpage); return NULL; }
    f->kpage = kpage;
    f->thread = thread_current();
    f->vme = NULL;
    lock_acquire(&frame_lock);
    list_push_back(&frame_table, &f->elem);
    if (clock_ptr == NULL || clock_ptr == list_end(&frame_table)) clock_ptr = &f->elem;
    lock_release(&frame_lock);
    return kpage;
}

void free_page (void *kpage) {
    struct list_elem *e;
    lock_acquire(&frame_lock);
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *tmp = list_entry(e, struct frame, elem);
        if (tmp->kpage == kpage) { __free_page(tmp); break; }
    }
    lock_release(&frame_lock);
}

void __free_page (struct frame *f) {
    if (clock_ptr == &f->elem) {
        clock_ptr = list_next(clock_ptr);
        if (clock_ptr == list_end(&frame_table)) clock_ptr = list_begin(&frame_table);
    }
    list_remove(&f->elem);
    palloc_free_page(f->kpage);
    free(f);
}

void add_page_to_frame (void *kpage, struct vm_entry *vme) {
    struct list_elem *e;
    lock_acquire(&frame_lock);
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *f = list_entry(e, struct frame, elem);
        if (f->kpage == kpage) { f->vme = vme; break; }
    }
    lock_release(&frame_lock);
}