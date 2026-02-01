#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"

static unsigned vm_hash_func (const struct hash_elem *e, void *aux UNUSED) {
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    return hash_bytes(&vme->vaddr, sizeof(vme->vaddr));
}

static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    struct vm_entry *vme_a = hash_entry(a, struct vm_entry, elem);
    struct vm_entry *vme_b = hash_entry(b, struct vm_entry, elem);
    return vme_a->vaddr < vme_b->vaddr;
}

static void vm_destroy_func (struct hash_elem *e, void *aux UNUSED) {
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);

    /* 1. 메모리에 로드된 상태라면 물리 프레임 해제 */
    if (vme->is_loaded) {
        struct thread *t = thread_current();
        void *kpage = pagedir_get_page(t->pagedir, vme->vaddr);
        if (kpage != NULL) {
            free_page(kpage);
            pagedir_clear_page(t->pagedir, vme->vaddr);
        }
    }
    /* 2. 메모리에 없고 스왑 영역에 있다면, 스왑 슬롯 해제 */
    else if (vme->type == VM_ANON) {
        vm_swap_free(vme->swap_slot);
    }

    free(vme);
}

void vm_init (struct hash *vm) {
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

void vm_destroy (struct hash *vm) {
    hash_destroy(vm, vm_destroy_func);
}

struct vm_entry *find_vme (void *vaddr) {
    struct vm_entry vme;
    vme.vaddr = pg_round_down(vaddr);
    struct hash_elem *e = hash_find(&thread_current()->vm, &vme.elem);
    return e ? hash_entry(e, struct vm_entry, elem) : NULL;
}

bool insert_vme (struct hash *vm, struct vm_entry *vme) {
    return hash_insert(vm, &vme->elem) == NULL;
}

bool delete_vme (struct hash *vm, struct vm_entry *vme) {
    if (hash_delete(vm, &vme->elem) != NULL) {
        free(vme);
        return true;
    }
    return false;
}